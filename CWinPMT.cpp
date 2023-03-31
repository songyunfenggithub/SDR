
#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "CSoundCard.h"
#include "CWaveData.h"
#include "CWaveFFT.h"
#include "CWaveFilter.h"
#include "CWinSpectrum.h"
#include "CPMT.h"

#include "CWinPMT.h"

using namespace std;

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define WAVE_RECT_HEIGHT				0X200
#define WAVE_RECT_BORDER_TOP		100
#define WAVE_RECT_BORDER_LEFT		50
#define WAVE_RECT_BORDER_RIGHT		60
#define WAVE_RECT_BORDER_BOTTON		0

#define DIVLONG		10
#define DIVSHORT	5

#define TIMEOUT		100

#define BRUSH_BACKGROUND		BLACK_BRUSH

#define COLOR_BORDER_THICK		RGB(64, 64, 64)
#define COLOR_BORDER_THIN		RGB(128, 128, 128)
#define COLOR_BACKGROUND		RGB(128, 128, 128)

#define COLOR_ORIGNAL_PMT		RGB(0, 255, 0)  
#define COLOR_ORIGNAL_PMT_LOG	RGB(0, 255, 255)  

#define COLOR_TEXT_BACKGOUND	RGB(0, 0, 0)
#define COLOR_TEXT				RGB(255, 255, 255)

CWinPMT clsWinPMT;

CWinPMT::CWinPMT()
{
	hMutexUseBuff = CreateMutex(NULL, false, "CWinPMThMutexUseBuff");
	RegisterWindowsClass();
	Init();
}

CWinPMT::~CWinPMT()
{

}

void CWinPMT::RegisterWindowsClass(void)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinPMT::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = PMT_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinPMT::OnMouse(HWND hWnd)
{
	HDC hDC = GetDC(hWnd);
	PAINTSTRUCT ps;
	RECT r, rt;
	SelectObject(hDC, CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));
	HPEN hPen = CreatePen(PS_DOT, 0, RGB(64, 64, 64));
	GetClientRect(hWnd, &rt);
	SetBkColor(hDC, COLOR_TEXT_BACKGOUND);
	SetBkMode(hDC, TRANSPARENT);
	//	SetBkMode(hdc, OPAQUE); 
	//	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);
	r.top = WAVE_RECT_BORDER_TOP - DIVLONG - 40;
	r.left = WAVE_RECT_BORDER_LEFT;
	r.right = r.left + 700;
	r.bottom = r.top + 20;
	FillRect(hDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));

	int i = 0;
	char s[500], t[100];
	FILTERCOREDATATYPE* pFilterCore = clsWaveFilter.pCurrentFilterInfo->FilterCore;
	int FilterLength = clsWaveFilter.pCurrentFilterInfo->CoreLength;
	int X = (HScrollPos + MouseX - WAVE_RECT_BORDER_LEFT) / HScrollZoom;
	X = BOUND(X, 0, (HScrollPos + rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / HScrollZoom);
	FILTERCOREDATATYPE Y = X > FilterLength ? 0 : pFilterCore[X];
	sprintf(s, "X: %d, core V: %lf", X, Y);
	SetTextColor(hDC, COLOR_ORIGNAL_PMT);
	DrawText(hDC, s, strlen(s), &r, NULL);

	r.left += 200;
	SetTextColor(hDC, COLOR_ORIGNAL_PMT);
	double Hz = (double)X * clsWaveData.AdcSampleRate / clsWaveFFT.FFTSize;
	Y = X > clsWaveFFT.FFTSize / 2 ? 0 : clsPMT.PMTBuff[X];
	sprintf(s, "Hz: %.03f, FFT: %s", Hz, formatKDouble(Y, 0.001, "", t));
	DrawText(hDC, s, strlen(s), &r, NULL);

	r.left += 250;
	SetTextColor(hDC, COLOR_ORIGNAL_PMT);
	int Ypos = BOUND(MouseY - WAVE_RECT_BORDER_TOP, 0, WAVE_RECT_HEIGHT);
	double Ylog = 20 * (X > clsWaveFFT.FFTSize / 2 ? 0 : clsPMT.PMTLogBuff[X]);
	sprintf(s, "Y: %.03fdb, logFFT: %fdb", -(double)Ypos / 64 * 20, Ylog);
	DrawText(hDC, s, strlen(s), &r, NULL);

	DeleteObject(SelectObject(hDC, GetStockObject(SYSTEM_FONT)));
	DeletePen(hPen);
	ReleaseDC(hWnd, hDC);
}

LRESULT CALLBACK CWinPMT::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinPMT.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinPMT::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rt;

	switch (message)
	{
	case WM_LBUTTONDOWN:
		//clsWinSpect.ActivehWnd = hWnd;
		break;
	case WM_MOUSEMOVE:
		MouseX = GET_X_LPARAM(lParam);
		MouseY = GET_Y_LPARAM(lParam);
		//OnMouse(hWnd);
		break;
	case WM_CREATE:
	{
		this->hWnd = hWnd;

		hMenu = GetMenu(hWnd);

		GetRealClientRect(hWnd, &rt);
		HScrollWidth = clsWaveFFT.FFTSize / 2 - (rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		if (HScrollWidth < 0) HScrollWidth = 0;
		SetScrollRange(hWnd, SB_HORZ, 0, HScrollWidth, TRUE);

		UINT uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
	}
	break;

	case WM_TIMER:
		break;

	case WM_SIZE:
		GetClientRect(hWnd, &rt);
		WinWidth = rt.right;
		WinHeight = rt.bottom;
		HScrollWidth = (clsWaveFFT.FFTSize / 2) * HScrollZoom - (rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		if (HScrollWidth < 0) HScrollWidth = 0;
		SetScrollRange(hWnd, SB_HORZ, 0, HScrollWidth, TRUE);
		break;

	case WM_COMMAND:
		OnCommand(hWnd, message, wParam, lParam);
		break;

	case WM_ERASEBKGND:
		//����������Ϣ��Ļˢ�»���˸
		break;

	case WM_PAINT:
		Paint(hWnd);
		OnMouse(hWnd);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		KeyAndScroll(hWnd, message, wParam, lParam);
		break;

	case WM_DESTROY:
		KillTimer(hWnd, 0);
		//PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

BOOL CWinPMT::OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);
	RECT rc;

	switch (wmId)
	{
		//Setting COMMANDS-----------------------
	case IDM_SPECTRUM_PAUSE_BREAK:
		break;

	case IDM_FFT_HORZ_ZOOM_INCREASE:
		if (HScrollZoom < 16) HScrollZoom *= 2;
		GetRealClientRect(hWnd, &rc);
		SetScrollRange(hWnd, SB_HORZ, 0, (HScrollWidth = HScrollZoom * clsWaveFFT.FFTSize / 2 - rc.right) > 0 ? HScrollWidth : 0, TRUE);
		InvalidateRect(hWnd, NULL, true);
		break;
	case IDM_FFT_HORZ_ZOOM_DECREASE:
		GetRealClientRect(hWnd, &rc);
		if (HScrollZoom * clsWaveFFT.FFTSize / 2 > rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_LEFT) HScrollZoom /= 2;
		SetScrollRange(hWnd, SB_HORZ, 0, (HScrollWidth = HScrollZoom * clsWaveFFT.FFTSize / 2 - rc.right) > 0 ? HScrollWidth : 0, TRUE);
		InvalidateRect(hWnd, NULL, true);
		break;
	case IDM_FFT_HORZ_ZOOM_HOME:
		HScrollZoom = 1.0;
		GetRealClientRect(hWnd, &rc);
		SetScrollRange(hWnd, SB_HORZ, 0, (HScrollWidth = HScrollZoom * clsWaveFFT.FFTSize / 2 - rc.right) > 0 ? HScrollWidth : 0, TRUE);
		InvalidateRect(hWnd, NULL, true);
		break;

	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

VOID CWinPMT::Paint(HWND hWnd)
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
				sprintf(s, "%.02fhz", (double)(i * 32 + HScrollPos) / HScrollZoom * clsWaveData.AdcSampleRate / clsWaveFFT.FFTSize);
				r.top = WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT + DIVLONG;
				r.left = x;
				SetTextColor(hdc, COLOR_ORIGNAL_PMT);
				DrawText(hdc, s, strlen(s), &r, NULL);
				sprintf(s, "%d", (int)((i * 32 + HScrollPos) / HScrollZoom));
				r.top = WAVE_RECT_BORDER_TOP - DIVLONG - 16;
				r.left = x;
				SetTextColor(hdc, COLOR_ORIGNAL_PMT);
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
				SetTextColor(hdc, COLOR_ORIGNAL_PMT_LOG);
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
	int FFTLength = clsWaveFFT.FFTSize / 2;

	WaitForSingleObject(hMutexUseBuff, INFINITE);

	//�����˲� FFT �ź�--------------------------------------------------
	UINT32* pPMTBuf = clsPMT.PMTBuff;
	double scale = 2 * CoreCenter / PMTMax;
	int Xstep = HScrollZoom > 1 ? HScrollZoom : 1;
	int istep = HScrollZoom < 1 ? 1 / HScrollZoom : 1;
	if (bPMTShow && pPMTBuf)
	{
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength) Y = 2 * CoreCenter - pPMTBuf[i] * scale;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_PMT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = 2 * CoreCenter - pPMTBuf[i] * scale;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pPMTLogBuf = clsPMT.PMTLogBuff;
	if (bPMTLogShow && pPMTLogBuf) {
		//�����˲� PMT Log10 �ź�--------------------------------------------------
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength)
			Y = pPMTLogBuf[i] * -64;
		Y = BOUND(Y, 0, 64 * 12);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_PMT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pPMTLogBuf[i] * -64;
			Y = BOUND(Y, 0, 64 * 12);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	{
		//����ͼ��-------------------------------------------------------
		r.top = WAVE_RECT_BORDER_TOP + DIVLONG + 20;
		r.left = rt.right - WAVE_RECT_BORDER_RIGHT - 100;
		r.right = rt.right;
		r.bottom = rt.bottom;
		SetBkMode(hdc, TRANSPARENT);

		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_PMT);
		SelectObject(hdc, hPen);
		MoveToEx(hdc, r.left - 20, r.top + 10, NULL);
		LineTo(hdc, r.left - 5, r.top + 10);
		DeleteObject(hPen);
		SetTextColor(hdc, COLOR_ORIGNAL_PMT);
		sprintf(s, "orignal fft");
		DrawText(hdc, s, strlen(s), &r, NULL);
		r.top += 20;
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_PMT_LOG);
		SelectObject(hdc, hPen);
		MoveToEx(hdc, r.left - 20, r.top + 10, NULL);
		LineTo(hdc, r.left - 5, r.top + 10);
		DeleteObject(hPen);
		SetTextColor(hdc, COLOR_ORIGNAL_PMT_LOG);
		sprintf(s, "orignal fft log");
		DrawText(hdc, s, strlen(s), &r, NULL);
		r.top += 20;
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_PMT);
		SelectObject(hdc, hPen);
		MoveToEx(hdc, r.left - 20, r.top + 10, NULL);
		LineTo(hdc, r.left - 5, r.top + 10);
		DeleteObject(hPen);
		SetTextColor(hdc, COLOR_ORIGNAL_PMT);
		sprintf(s, "filtted fft");
		DrawText(hdc, s, strlen(s), &r, NULL);
		r.top += 20;
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_PMT_LOG);
		SelectObject(hdc, hPen);
		MoveToEx(hdc, r.left - 20, r.top + 10, NULL);
		LineTo(hdc, r.left - 5, r.top + 10);
		DeleteObject(hPen);
		SetTextColor(hdc, COLOR_ORIGNAL_PMT_LOG);
		sprintf(s, "filtted fft log");
		DrawText(hdc, s, strlen(s), &r, NULL);
	}

	//---------------------------------------

	double FullVotage = 5.0;
	double VotagePerDIV = (FullVotage / (unsigned __int64)((UINT64)1 << (sizeof(ADCDATATYPE) * 8)));
	double z = DrawInfo.iVZoom >= 0 ? (1.0 / ((UINT64)1 << DrawInfo.iVZoom)) : ((UINT64)1 << -DrawInfo.iVZoom);
	char tstr1[100], tstr2[100];
	//clsWaveData.NumPerSec = 38400;
	double TimePreDiv = 32 * 1.0 / clsWaveData.NumPerSec *
		(DrawInfo.iHZoom >= 0 ? 1.0 / ((UINT64)1 << DrawInfo.iHZoom) : ((UINT64)1 << -DrawInfo.iHZoom));
	r.top = WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP + DIVLONG + 20;
	r.left = WAVE_RECT_BORDER_TOP;
	r.right = rt.right;
	r.bottom = rt.bottom;
	sprintf(s, "32pix / DIV\r\n"\
		"PMT Length: %d\r\n"\
		"AdcSampleRate: %d    Real AdcSampleRate: %d\r\n"\
		"FFT Size: %d      FFT Step: %d\r\n"\
		"Sepctrum Hz��%.03f",
		PMT_LENGTH,
		clsWaveData.AdcSampleRate, clsWaveData.NumPerSec,
		clsWaveFFT.FFTSize, clsWaveFFT.FFTStep,
		clsWinOneSpectrum.Hz
	);
	SetBkMode(hdc, TRANSPARENT);
	//	SetBkMode(hdc, OPAQUE); 
	//	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);

	SetTextColor(hdc, COLOR_TEXT);
	DrawText(hdc, s, strlen(s), &r, NULL);

	sprintf(s, "HZoom: %f", HScrollZoom);
	r.top += 80;
	DrawText(hdc, s, strlen(s), &r, NULL);

	//sprintf(s, "vzoom %.40f", vzoom);
	r.top += 40;
	//DrawText(hdc, s, strlen(s), &r, NULL);

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


VOID CWinPMT::GetRealClientRect(HWND hWnd, PRECT lprc)
{
	DWORD dwStyle;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	GetClientRect(hWnd, lprc);
	if (dwStyle & WS_HSCROLL)
		lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (dwStyle & WS_VSCROLL)
		lprc->right += GetSystemMetrics(SM_CXVSCROLL);
}

void CWinPMT::KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
			printf("HScrollPos: %d, HScrollWidth: %d.\r\n", HScrollPos, HScrollWidth);
		}
		break;
	}
}

void CWinPMT::Init(void)
{

	//��������ʼ��
	RECT rt;
	GetRealClientRect(hWnd, &rt);
	HScrollWidth = clsWaveFFT.FFTSize / 2 - (rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
	if (HScrollWidth < 0) HScrollWidth = 0;
	SetScrollRange(hWnd, SB_HORZ, 0, HScrollWidth, TRUE);

}


void CWinPMT::OpenWindow(void)
{
	if (hWnd) {
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
	}
}