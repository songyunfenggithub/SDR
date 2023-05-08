
#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "CSoundCard.h"
#include "CData.h"
#include "CFilter.h"
#include "CWinFFT.h"
#include "CWinSDR.h"
#include "CToolsWin.h"

#include "CWinSDRScan.h"

using namespace std;
using namespace WINS; 
//using namespace DEVICES;

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define BRUSH_BACKGROUND		BLACK_BRUSH

#define COLOR_BORDER_THICK		RGB(64, 64, 64)
#define COLOR_BORDER_THIN		RGB(128, 128, 128)
#define COLOR_BACKGROUND		RGB(128, 128, 128)

#define COLOR_ORIGNAL_FFT		RGB(0, 255, 0)  
#define COLOR_ORIGNAL_FFT_LOG	RGB(0, 255, 255)  
#define COLOR_FILTTED_FFT		RGB(0, 0, 255)  
#define COLOR_FILTTED_FFT_LOG	RGB(255, 255, 0)  

#define COLOR_TEXT_BACKGOUND	RGB(0, 0, 0)
#define COLOR_TEXT				RGB(255, 255, 255)

#define DIVLONG		10
#define DIVSHORT	5

#define TIMEOUT		100

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define DIVLONG		10
#define DIVSHORT	5

CWinSDRScan clsWinOneFFT;

CWinSDRScan::CWinSDRScan()
{
	OPENCONSOLE;

	hMutexUseBuff = CreateMutex(NULL, false, "CWinScanhMutexUseBuff");

	RegisterWindowsClass();

}

CWinSDRScan::~CWinSDRScan()
{
	//CLOSECONSOLE;
}

void CWinSDRScan::RegisterWindowsClass(void)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinSDRScan::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = SDR_SCAN_ONE_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

LRESULT CALLBACK CWinSDRScan::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinOneFFT.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinSDRScan::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rt;

	//ProcessKey(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_CREATE:

		OPENCONSOLE;
		{
			clsWinOneFFT.hWnd = hWnd;

			uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
			//KillTimer(hWnd, uTimerId);
		}
		break;

	case WM_CHAR:
		printf("WinOneSDRScan WM_CHAR\r\n");
		//PostMessage(clsWinSpect.hWnd, message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		//clsWinSpect.ActivehWnd = NULL;
		break;
	case WM_MOUSEMOVE:
		MouseX = GET_X_LPARAM(lParam);
		MouseY = GET_Y_LPARAM(lParam);
		//Hz = ((double)MouseY / clsWinSpect.WinOneSpectrumVScrollZoom + clsWinSpect.WinOneSpectrumVScrollPos) * AdcData->SampleRate / clsWaveFFT.FFTSize;
		OnMouse(hWnd);
		break;

	case WM_TIMER:
		if (bFFTHold == false && FFTNeedReDraw == false) {
			FFTNeedReDraw = true;
		}
		//InvalidateRect(hWnd, NULL, TRUE);
		//UpdateWindow(hWnd);
		break;

	case WM_SIZE:
		GetRealClientRect(hWnd, &rt);
		WinWidth = rt.right;
		break;

	case WM_COMMAND:
		return OnCommand(hWnd, message, wParam, lParam);
		break;

	case WM_SYSCOMMAND:
		if (LOWORD(wParam) == SC_CLOSE)
		{
			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;

	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;
	case WM_PAINT:
		Paint(hWnd);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		KeyAndScroll(hWnd, message, wParam, lParam);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

BOOL CWinSDRScan::OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
		//Setting COMMANDS-----------------------
	case IDM_SPECTRUM_PAUSE_BREAK:
		break;

	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

void CWinSDRScan::OnMouse(HWND hWnd)
{
	/*
	RECT rt;
	GetClientRect(hWnd, &rt);

	int i = 0;
	char s[500], t[100];
	FILTER_CORE_DATA_TYPE* pFilterCore = clsFilter.pCurrentFilterInfo == NULL ? clsFilter.FilterCore : clsFilter.pCurrentFilterInfo->FilterCore;
	int FilterLength = clsFilter.pCurrentFilterInfo == NULL ? clsFilter.FilterCoreLength : clsFilter.pCurrentFilterInfo->CoreLength;
	int X = (clsWinSpect.HScrollPos + MouseX - WAVE_RECT_BORDER_LEFT) / clsWinSpect.HScrollZoom;
	X = BOUND(X, 0, (clsWinSpect.HScrollPos + rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / clsWinSpect.HScrollZoom);
	double Y = X > FilterLength ? 0 : pFilterCore[X];
	int n = 0;
	n += sprintf(strMouse + n, "X: %d, core V: %lf", X, Y);

	double Hz = (double)X * AdcData->SampleRate / clsWaveFFT.FFTSize;
	Y = X > clsWaveFFT.FFTSize / 2 ? 0 : clsWinSpect.OrignalFFTBuff[X];
	n += sprintf(strMouse + n, " | ");
	n += sprintf(strMouse + n, "Hz: %.03f, FFT: %s", Hz, formatKDouble(Y, 0.001, "", t));

	int Ypos = BOUND(MouseY - WAVE_RECT_BORDER_TOP, 0, WAVE_RECT_HEIGHT);
	double Ylog = 20 * (X > clsWaveFFT.FFTSize / 2 ? 0 : clsWinSpect.FilttedFFTBuffLog[X]);
	n += sprintf(strMouse + n, " | ");
	n += sprintf(strMouse + n, "Y: %.03fdb, logFFT: %fdb", -(double)Ypos / 64 * 20, Ylog);
	*/
}

VOID CWinSDRScan::Paint(HWND hWnd)
{
	HDC		hDC;
	PAINTSTRUCT ps;
	RECT	rt, r;
	HPEN	hPen, hPenLighter;

	hDC = BeginPaint(hWnd, &ps);

	//EndPaint(hWnd, &ps);
	//return;

	GetClientRect(hWnd, &rt);

	HDC		hdc = CreateCompatibleDC(hDC);
	HBITMAP hbmp = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hdc, hbmp);

	SelectObject(hdc, CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));

	FillRect(hdc, &rt, (HBRUSH)GetStockObject(BRUSH_BACKGROUND));

	int	x, y, pos, i;
	TCHAR	s[200];
	int		zoomX, zoomY;
	zoomX = 1;
	zoomY = 1;
	pos = 0;
	s[0] = 0;

	hPen = CreatePen(PS_DOT, 0, COLOR_BORDER_THICK);
	hPenLighter = CreatePen(PS_DOT, 0, COLOR_BORDER_THIN);
	SetBkColor(hdc, COLOR_BACKGROUND);
	r.top = WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP + DIVLONG;
	r.right = rt.right;
	r.bottom = rt.bottom;

	SetBkMode(hdc, TRANSPARENT);
	//	SetBkMode(hdc, OPAQUE); 
	//	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);

	for (x = WAVE_RECT_BORDER_LEFT, i = 0; x < rt.right - WAVE_RECT_BORDER_RIGHT; x += 8)
	{
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP - ((x - WAVE_RECT_BORDER_LEFT) % 32 ? DIVSHORT : DIVLONG), NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP);
		if (!((x - WAVE_RECT_BORDER_LEFT) % 32))
		{
			if (!(i % 5))
			{
				sprintf(s, "%.02fhz", (double)(i * 32 + HScrollPos) / HScrollZoom * AdcData->SampleRate / FFTInfo_Signal.FFTSize);
				r.top = WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT + DIVLONG;
				r.left = x;
				SetTextColor(hdc, COLOR_ORIGNAL_FFT);
				DrawText(hdc, s, strlen(s), &r, NULL);
				sprintf(s, "%d", (int)((i * 32 + HScrollPos) / HScrollZoom));
				r.top = WAVE_RECT_BORDER_TOP - DIVLONG - 16;
				r.left = x;
				SetTextColor(hdc, COLOR_ORIGNAL_FFT);
				DrawText(hdc, s, strlen(s), &r, NULL);
			}
			SelectObject(hdc, i % 10 ? hPen : hPenLighter);
			LineTo(hdc, x, WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP);
			i++;
		}
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP, NULL);
		LineTo(hdc, x, WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP + ((x - WAVE_RECT_BORDER_LEFT) % 32 ? DIVSHORT : DIVLONG));
	}
	SelectObject(hdc, hPen);
	MoveToEx(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, WAVE_RECT_BORDER_TOP, NULL);
	LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP);

	for (y = WAVE_RECT_BORDER_TOP, i = 0; y <= WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP; y += 8)
	{
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, WAVE_RECT_BORDER_LEFT - ((y - WAVE_RECT_BORDER_TOP) % 32 ? DIVSHORT : DIVLONG), y, NULL);
		LineTo(hdc, WAVE_RECT_BORDER_LEFT, y);
		if (!((y - WAVE_RECT_BORDER_TOP) % 32))
		{
			SelectObject(hdc, i % 4 ? hPen : hPenLighter);
			LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, y);

			if (!((y - WAVE_RECT_BORDER_TOP) % 64))
			{
				sprintf(s, "%ddb", -i / 2 * 20);
				r.top = y - 8;
				r.left = rt.right - WAVE_RECT_BORDER_RIGHT + DIVLONG + 1;
				r.right = rt.right;
				r.bottom = r.top + 20;
				SetTextColor(hdc, COLOR_FILTTED_FFT_LOG);
				DrawText(hdc, s, strlen(s), &r, NULL);
			}
			i++;
		}
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, y, NULL);
		LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT + ((y - WAVE_RECT_BORDER_TOP) % 32 ? DIVSHORT : DIVLONG), y);
	}
	DeleteObject(hPen);
	DeleteObject(hPenLighter);

	double Y = 0;
	int X;
	double CoreCenter = 256;
	int FFTLength = FFTInfo_Signal.HalfFFTSize;

	WaitForSingleObject(hMutexUseBuff, INFINITE);

	//绘制滤波 FFT 信号--------------------------------------------------
	double* pOriBuf = OrignalScanBuff;
	double* pFilBuf = FilttedScanBuff;
	double fftvmax;
	double fftvmin;
	if (pOriBuf && pFilBuf) fftvmax = max(pOriBuf[FFTLength], pFilBuf[FFTLength]);
	double scale = 2 * CoreCenter / fftvmax;
	int Xstep = HScrollZoom > 1.0 ? HScrollZoom : 1;
	int istep = HScrollZoom < 1.0 ? ((double)1.0 / HScrollZoom) : 1;
	if (bFFTOrignalShow && pOriBuf)
	{
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength) Y = 2 * CoreCenter - pOriBuf[i] * scale;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = 2 * CoreCenter - pOriBuf[i] * scale;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}
	if (bFFTFilttedShow && pFilBuf)
	{
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength) Y = 2 * CoreCenter - pFilBuf[i] * scale;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = 2 * CoreCenter - pFilBuf[i] * scale;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pOriLogBuf = OrignalScanBuffLog;
	if (bFFTOrignalLogShow && pOriLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		fftvmax = pOriLogBuf[FFTLength];
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength)
			Y = pOriLogBuf[i] * -64;
		Y = BOUND(Y, 0, 64 * 12);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pOriLogBuf[i] * -64;
			Y = BOUND(Y, 0, 64 * 12);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pFilLogBuf = FilttedScanBuffLog;
	if (bFFTFilttedLogShow && pFilLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength)
			Y = pFilLogBuf[i] * -64;
		Y = BOUND(Y, 0, 64 * 12);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pFilLogBuf[i] * -64;
			Y = BOUND(Y, 0, 64 * 12);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	{
		//绘制图列-------------------------------------------------------
		r.top = WAVE_RECT_BORDER_TOP + DIVLONG + 20;
		r.left = rt.right - WAVE_RECT_BORDER_RIGHT - 100;
		r.right = rt.right;
		r.bottom = rt.bottom;
		SetBkMode(hdc, TRANSPARENT);

		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT);
		SelectObject(hdc, hPen);
		MoveToEx(hdc, r.left - 20, r.top + 10, NULL);
		LineTo(hdc, r.left - 5, r.top + 10);
		DeleteObject(hPen);
		SetTextColor(hdc, COLOR_ORIGNAL_FFT);
		sprintf(s, "orignal fft");
		DrawText(hdc, s, strlen(s), &r, NULL);
		r.top += 20;
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT_LOG);
		SelectObject(hdc, hPen);
		MoveToEx(hdc, r.left - 20, r.top + 10, NULL);
		LineTo(hdc, r.left - 5, r.top + 10);
		DeleteObject(hPen);
		SetTextColor(hdc, COLOR_ORIGNAL_FFT_LOG);
		sprintf(s, "orignal fft log");
		DrawText(hdc, s, strlen(s), &r, NULL);
		r.top += 20;
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT);
		SelectObject(hdc, hPen);
		MoveToEx(hdc, r.left - 20, r.top + 10, NULL);
		LineTo(hdc, r.left - 5, r.top + 10);
		DeleteObject(hPen);
		SetTextColor(hdc, COLOR_FILTTED_FFT);
		sprintf(s, "filtted fft");
		DrawText(hdc, s, strlen(s), &r, NULL);
		r.top += 20;
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT_LOG);
		SelectObject(hdc, hPen);
		MoveToEx(hdc, r.left - 20, r.top + 10, NULL);
		LineTo(hdc, r.left - 5, r.top + 10);
		DeleteObject(hPen);
		SetTextColor(hdc, COLOR_FILTTED_FFT_LOG);
		sprintf(s, "filtted fft log");
		DrawText(hdc, s, strlen(s), &r, NULL);
	}

	//---------------------------------------
	{
		//绘制
#define DRAW_TEXT_X		(WAVE_RECT_BORDER_LEFT + 10)	
#define DRAW_TEXT_Y		(WAVE_RECT_BORDER_TOP + DIVLONG + 20)
		double FullVotage = 5.0;
		double VotagePerDIV = (FullVotage / (unsigned __int64)((UINT64)1 << (sizeof(ADC_DATA_TYPE) * 8)));
		char tstr1[100], tstr2[100];
		r.top = DRAW_TEXT_Y;
		r.left = DRAW_TEXT_X;
		r.right = rt.right;
		r.bottom = rt.bottom;
		sprintf(s, "32pix / DIV\r\n"\
			"Core Length: %d\r\n"\
			"AdcSampleRate: %d    Real AdcSampleRate: %d\r\n"\
			"FFT Size: %d      FFT Step: %d\r\n"\
			"Sepctrum Hz：%.03f",
			CoreLength,
			AdcData->SampleRate, AdcData->NumPerSec,
			FFTInfo_Signal.FFTSize, FFTInfo_Signal.FFTStep,
			clsWinOneSpectrum.Hz
		);
		SetBkMode(hdc, TRANSPARENT);
		//	SetBkMode(hdc, OPAQUE); 
		//	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);

		SetTextColor(hdc, COLOR_TEXT);
		DrawText(hdc, s, strlen(s), &r, NULL);

		sprintf(s, "HZoom = %f", HScrollZoom);
		r.top += 80;
		DrawText(hdc, s, strlen(s), &r, NULL);

		//sprintf(s, "vzoom %.40f", vzoom);
		r.top += 40;
		//DrawText(hdc, s, strlen(s), &r, NULL);
	}

	{
		//绘制鼠标提示消息
		r.top = WAVE_RECT_BORDER_TOP + DIVLONG + 5;
		r.left = WAVE_RECT_BORDER_LEFT + 10;
		r.right = r.left + 700;
		r.bottom = r.top + 20;
		SetTextColor(hdc, COLOR_ORIGNAL_FFT);
		DrawText(hdc, strMouse, strlen(strMouse), &r, NULL);
	}

	DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

	BitBlt(hDC,
		0, 0,
		rt.right, rt.bottom,
		hdc,
		0, 0,
		SRCCOPY);
	DeleteObject(hdc);
	DeleteObject(hbmp);

	ReleaseMutex(hMutexUseBuff);

	EndPaint(hWnd, &ps);
}

void CWinSDRScan::KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT     iMax, iMin, iPos;
	int		dn = 0, tbdn = 0;
	RECT    rc;

	switch (message)
	{
	case WM_KEYDOWN:
		printf("win fft WM_KEYDOWN\r\n");
		/* Translate keyboard messages to scroll commands */
		switch (wParam)
		{
		case VK_UP:
			PostMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0L);
			break;
		case VK_DOWN:
			PostMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0L);
			break;
		case VK_PRIOR:
			PostMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0L);
			break;
		case VK_NEXT:
			PostMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
			break;
		case VK_HOME:
			PostMessage(hWnd, WM_HSCROLL, SB_PAGEUP, 0L);
			break;
		case VK_END:
			PostMessage(hWnd, WM_HSCROLL, SB_PAGEDOWN, 0L);
			break;
		case VK_LEFT:
			PostMessage(hWnd, WM_HSCROLL, SB_LINEUP, 0L);
			break;
		case VK_RIGHT:
			PostMessage(hWnd, WM_HSCROLL, SB_LINEDOWN, 0L);
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_UP:
		case VK_DOWN:
		case VK_PRIOR:
		case VK_NEXT:
			PostMessage(hWnd, WM_VSCROLL, SB_ENDSCROLL, 0L);
			break;

		case VK_HOME:
		case VK_END:
		case VK_LEFT:
		case VK_RIGHT:
			PostMessage(hWnd, WM_HSCROLL, SB_ENDSCROLL, 0L);
			break;
		}
		break;

	case WM_VSCROLL:
		//Calculate new vertical scroll position
		GetScrollRange(hWnd, SB_VERT, &iMin, &iMax);
		iPos = GetScrollPos(hWnd, SB_VERT);
		GetClientRect(hWnd, &rc);
		switch (GET_WM_VSCROLL_CODE(wParam, lParam))
		{
		case SB_LINEDOWN:
			dn = rc.bottom / 16;
			break;
		case SB_LINEUP:
			dn = -rc.bottom / 16;
			break;
		case SB_PAGEDOWN:
			dn = rc.bottom / 2;
			break;
		case SB_PAGEUP:
			dn = -rc.bottom / 2;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			break;
		}

	case WM_HSCROLL:
		//Calculate new horizontal scroll position
		GetScrollRange(hWnd, SB_HORZ, &iMin, &iMax);
		iPos = GetScrollPos(hWnd, SB_HORZ);
		GetClientRect(hWnd, &rc);

		switch (GET_WM_HSCROLL_CODE(wParam, lParam))
		{
		case SB_LINEDOWN:
			dn = 1;// (rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / 16;
			break;
		case SB_LINEUP:
			dn = -1;// -(rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / 16;
			break;
		case SB_PAGEDOWN:
			dn = (rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / 2;
			break;
		case SB_PAGEUP:
			dn = -(rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / 2;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			tbdn = GET_WM_HSCROLL_POS(wParam, lParam);
			break;
		default:
			dn = 0;
			break;
		}
		if (dn != 0)
		{
			HScrollPos = BOUND(HScrollPos + dn, 0, HScrollWidth);
		}
		if (tbdn != 0)
		{
			HScrollPos = BOUND(tbdn, 0, HScrollWidth);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
			printf("CWinSDRScan HScrollPos: %d, HScrollWidth: %d.\r\n", HScrollPos, HScrollWidth);
		}
		break;
	}
}

VOID CWinSDRScan::GetRealClientRect(HWND hWnd, PRECT lprc)
{
	DWORD dwStyle;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	GetClientRect(hWnd, lprc);
	if (dwStyle & WS_HSCROLL)
		lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (dwStyle & WS_VSCROLL)
		lprc->right += GetSystemMetrics(SM_CXVSCROLL);
}


