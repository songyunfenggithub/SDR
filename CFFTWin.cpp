
#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "myDebug.h"
#include "CSoundCard.h"
#include "CWaveData.h"
#include "CWaveFFT.h"
#include "CWaveFilter.h"
#include "CWinSpectrum.h"
#include "CDataFromSDR.h"
#include "CWaveAnalyze.h"
#include "CFFT.h"

#include "CFFTWin.h"

#define WAVE_RECT_HEIGHT			0X200
#define WAVE_RECT_BORDER_TOP		25
#define WAVE_RECT_BORDER_LEFT		20
#define WAVE_RECT_BORDER_RIGHT		60
#define WAVE_RECT_BORDER_BOTTON		25


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

#define FONT_HEIGHT	14

#define TIMEOUT		100

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define UP_TO_ZERO(x) (x = x < 0 ? 0 : x)

#define DIVLONG		10
#define DIVSHORT	5

#define FFT_ZOOM_MAX		16

CFFTWin::CFFTWin()
{
	OPENCONSOLE;

	Init();

	RegisterWindowsClass();

}

CFFTWin::~CFFTWin()
{
	UnInit();
	CLOSECONSOLE;
}

void CFFTWin::Init(void)
{
	hDrawMutex = CreateMutex(NULL, false, "CFFTWinhDrawMutex");
}

void CFFTWin::UnInit(void)
{

}

void CFFTWin::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CFFTWin::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = FFT_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

LRESULT CALLBACK CFFTWin::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CFFTWin* me;
	HGLOBAL hMemProp;
	void* lpMem;
	hMemProp = (HGLOBAL)GetProp(hWnd, "H");
	if (hMemProp) {
		lpMem = GlobalLock(hMemProp);
		memcpy(&me, lpMem, sizeof(UINT64));
		GlobalUnlock(hMemProp);
	}

	switch (message)
	{
	case WM_CREATE:
	{
		OPENCONSOLE;

		me = (CFFTWin*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
		HGLOBAL hMemProp;
		void* lpMem;
		hMemProp = GlobalAlloc(GPTR, sizeof(INT64));
		lpMem = GlobalLock(hMemProp);
		memcpy(lpMem, &me, sizeof(UINT64));
		GlobalUnlock(hMemProp);
		SetProp(hWnd, "H", hMemProp);

		me->hWnd = hWnd;
		me->uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		//KillTimer(hWnd, uTimerId);
		me->hWndVScroll = CreateWindow(TEXT("scrollbar"), NULL,
			WS_CHILD | WS_VISIBLE |
			WS_TABSTOP | SBS_VERT,
			20, 20, 30, 100,
			hWnd, 0, hInst, NULL);
		SetScrollRange(me->hWndVScroll, SB_CTL, 0, 300, FALSE);
		SetScrollPos(me->hWndVScroll, SB_CTL, 0, FALSE);

		me->fft = new CFFT();
		me->fft->Init(me);
	}
	break;
	case WM_CHAR:
		DbgMsg("CFFTWin WM_CHAR\r\n");
		PostMessage(clsWinSpect.hWnd, message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		//clsWinSpect.ActivehWnd = NULL;
		break;
	case WM_LBUTTONUP:
		break;
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		me->MouseX = GET_X_LPARAM(lParam);
		me->MouseY = GET_Y_LPARAM(lParam);
		me->OnMouse();
		break;
	case WM_TIMER:
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		me->fft->FFTNext = true;
		break;
	case WM_SIZE:
	{
		me->GetRealClientRect(&me->WinRect);
		me->HScrollWidth = me->HalfFFTSize / me->HScrollZoom - (me->WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		UP_TO_ZERO(me->HScrollWidth);
		SetScrollRange(hWnd, SB_HORZ, 0, me->HScrollWidth, TRUE);
		int w = GetSystemMetrics(SM_CXVSCROLL);
		MoveWindow(me->hWndVScroll, me->WinRect.right - w, WAVE_RECT_BORDER_TOP, w, me->FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON, true);
		me->VScrollHeight = (me->DBMin * -64) / me->VScrollZoom - (me->FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON);
		UP_TO_ZERO(me->VScrollHeight);
		SetScrollRange(me->hWndVScroll, SB_CTL, 0, me->VScrollHeight, TRUE);
		me->InitDrawBuff();
	}
	break;
	case WM_COMMAND:
		return me->OnCommand(message, wParam, lParam);
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
		me->Paint();
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		me->KeyAndScroll(message, wParam, lParam);
		break;

	case WM_DESTROY:
		//PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

bool CFFTWin::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDM_FFT_HORZ_ZOOM_INCREASE:
		if (HScrollZoom < FFT_ZOOM_MAX) {
			HScrollZoom *= 2.0;
			HScrollWidth = HalfFFTSize / HScrollZoom - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			SetScrollRange(hWnd, SB_HORZ, 0, HScrollWidth, TRUE);
			HScrollPos *= 2.0;
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_DECREASE:
		if (HScrollZoom * HalfFFTSize > WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) {
			HScrollZoom /= 2.0;
			HScrollWidth = HalfFFTSize / HScrollZoom - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			SetScrollRange(hWnd, SB_HORZ, 0, HScrollWidth, TRUE);
			HScrollPos /= 2.0;
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_HOME:
		HScrollPos /= HScrollZoom;
		HScrollWidth = HalfFFTSize - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		SetScrollRange(hWnd, SB_HORZ, 0, HScrollWidth, TRUE);
		SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
		HScrollZoom = 1.0;
		break;

	case IDM_FFT_VERT_ZOOM_INCREASE:
		if (VScrollZoom < FFT_ZOOM_MAX) {
			VScrollZoom *= 2.0;
			VScrollHeight = (DBMin * -64) * VScrollZoom - (FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON);
			SetScrollRange(hWndVScroll, SB_CTL, 0, VScrollHeight, TRUE);
			VScrollPos *= 2.0;
			SetScrollPos(hWndVScroll, SB_CTL, VScrollPos, TRUE);
		}
		break;
	case IDM_FFT_VERT_ZOOM_DECREASE:
		if ((VScrollZoom * DBMin * -64.0) > FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON) {
			VScrollZoom /= 2.0;
			VScrollHeight = (DBMin * -64) * VScrollZoom - (FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON);
			SetScrollRange(hWndVScroll, SB_CTL, 0, VScrollHeight, TRUE);
			VScrollPos /= 2.0;
			SetScrollPos(hWndVScroll, SB_CTL, VScrollPos, TRUE);
		}
		break;
	case IDM_FFT_VERT_ZOOM_HOME:
		VScrollPos /= VScrollZoom;
		VScrollHeight = (DBMin * -64) - (FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON);
		SetScrollRange(hWndVScroll, SB_CTL, 0, VScrollHeight, TRUE);
		SetScrollPos(hWndVScroll, SB_CTL, VScrollPos, TRUE);
		VScrollZoom = 1.0;
		break;
		//Setting COMMANDS-----------------------
	case IDM_SPECTRUM_PAUSE_BREAK:
		break;
	case IDM_EXIT:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

void CFFTWin::OnMouse(void)
{
	int i = 0;
	char s[500], t[100];
	FILTERCOREDATATYPE* pFilterCore = clsWaveFilter.pCurrentFilterInfo->FilterCore;
	int FilterLength = clsWaveFilter.pCurrentFilterInfo->CoreLength;
	int X = (HScrollPos + MouseX - WAVE_RECT_BORDER_LEFT) / HScrollZoom;
	X = BOUND(X, 0, (HScrollPos + this->WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / HScrollZoom);
	double Y = X > FilterLength ? 0 : pFilterCore[X];
	int n = 0;
	n += sprintf(strMouse + n, "X: %d, core V: %lf", X, Y);

	double Hz = (double)X * clsWaveData.AdcSampleRate / FFTSize;
	Y = X > FFTSize / 2 ? 0 : clsWinSpect.OrignalFFTBuff[X];
	n += sprintf(strMouse + n, " | ");
	n += sprintf(strMouse + n, "Hz: %.03f, FFT: %s", Hz, formatKDouble(Y, 0.001, "", t));

	int Ypos = BOUND(MouseY - WAVE_RECT_BORDER_TOP, 0, WAVE_RECT_HEIGHT);
	double Ylog = 20 * (X > clsWaveFFT.FFTSize / 2 ? 0 : clsWinSpect.FilttedFFTBuffLog[X]);
	n += sprintf(strMouse + n, " | ");
	n += sprintf(strMouse + n, "Y: %.03fdb, logFFT: %fdb", -(double)Ypos / 64 * 20, Ylog);

}

void CFFTWin::Paint(void)
{
	HDC		hDC;
	PAINTSTRUCT ps;
	RECT	rt, r;
	HPEN	hPen, hPenLighter;

	hDC = BeginPaint(hWnd, &ps);

	//EndPaint(hWnd, &ps);
	//return;

	GetClientRect(hWnd, &rt);

	rt.right = WinRect.right;
	rt.bottom = FFTHeight;

	HDC		hdc = CreateCompatibleDC(hDC);
	HBITMAP hbmp = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hdc, hbmp);

	SelectObject(hdc, CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));

	FillRect(hdc, &rt, (HBRUSH)GetStockObject(BRUSH_BACKGROUND));
	SetBkMode(hdc, TRANSPARENT);

	int	x, y, pos, i;
	TCHAR	s[1000];
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
	SetTextColor(hdc, COLOR_TEXT);

#define WEB_H_STEP	8
	UINT web_hoffset = WEB_H_STEP - HScrollPos % WEB_H_STEP;
	if (web_hoffset == WEB_H_STEP)web_hoffset = 0;
	UINT long_step = WEB_H_STEP * 4;
	UINT FFTDrawHeight = FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON;
	for (x = WAVE_RECT_BORDER_LEFT + web_hoffset; x <= rt.right - WAVE_RECT_BORDER_RIGHT; x += WEB_H_STEP)
	{
		UINT64 realPos = HScrollPos + x - WAVE_RECT_BORDER_LEFT;
		UINT LineOffset = realPos % long_step ? DIVSHORT : DIVLONG;
		UINT64 lineNum = realPos / WEB_H_STEP;
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP - LineOffset, NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP);

		if ((lineNum % 4) == 0) {
			if ((lineNum % 16) == 0) {
				UINT64 pos = (UINT64)(((double)realPos) / HScrollZoom);
				sprintf(s, "%lu", pos);
				r.left = x;
				r.top = WAVE_RECT_BORDER_TOP - DIVLONG - FONT_HEIGHT;
				DrawText(hdc, s, strlen(s), &r, NULL);
				//sprintf(s, "%.6fs", (double)pos / clsWaveData.AdcSampleRate);
				//formatKKDouble((double)pos / clsWaveData.AdcSampleRate, "s", s);
				sprintf(s, "%.03fhz", (double)(pos * clsWaveData.AdcSampleRate / FFTSize));
				r.top = WAVE_RECT_BORDER_TOP + (FFTDrawHeight) + DIVLONG;
				DrawText(hdc, s, strlen(s), &r, NULL);
			}
			SelectObject(hdc, lineNum % 16 ? hPen : hPenLighter);
			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + FFTDrawHeight);
		}
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + FFTDrawHeight, NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + FFTDrawHeight + LineOffset);
	}

	r.top = WAVE_RECT_BORDER_TOP - FONT_HEIGHT / 2;
	r.left = rt.right - WAVE_RECT_BORDER_RIGHT + DIVLONG + 2;
	r.right = rt.right;
	r.bottom = rt.bottom;
	UINT web_voffset = WEB_H_STEP - VScrollPos % WEB_H_STEP;
	if (web_voffset == WEB_H_STEP)web_voffset = 0;
	i = 0;
	for (y = WAVE_RECT_BORDER_TOP + web_voffset; y <= WAVE_RECT_BORDER_TOP + FFTDrawHeight; y += WEB_H_STEP)
	{
		UINT64 realPos = VScrollPos + y - WAVE_RECT_BORDER_TOP;
		UINT LineOffset = realPos % long_step ? DIVSHORT : DIVLONG;
		UINT64 lineNum = realPos / WEB_H_STEP;
		MoveToEx(hdc, WAVE_RECT_BORDER_LEFT - LineOffset, y, NULL);
		LineTo(hdc, WAVE_RECT_BORDER_LEFT, y);
		if ((lineNum % 4) == 0) {
			//if (lineNum % 8 == 0)
			{
				double pos = (((double)realPos) / VScrollZoom);
				//formatKDouble(DrawInfo.FullVotage - DrawInfo.VotagePerDIV * (realPos / DrawInfo.dbVZoom),
				//	DrawInfo.VotagePerDIV / DrawInfo.dbVZoom, "v", s);
				sprintf(s, "%.3fdb", (realPos / (4 * WEB_H_STEP * VScrollZoom)) * -10);
				r.top = y - (FONT_HEIGHT >> 1);
				DrawText(hdc, s, strlen(s), &r, NULL);
			}
			i++;
			SelectObject(hdc, lineNum % 16 ? hPen : hPenLighter);
			LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, y);
		}
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, y, NULL);
		LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT + LineOffset, y);
	}
	DeleteObject(hPen);
	DeleteObject(hPenLighter);

	PaintFFT(hdc, fft);

	//---------------------------------------
	{
		//绘制
#define DRAW_TEXT_X		(WAVE_RECT_BORDER_LEFT + 10)	
#define DRAW_TEXT_Y		(WAVE_RECT_BORDER_TOP + DIVLONG + 20)
		double FullVotage = 5.0;
		double VotagePerDIV = (FullVotage / (unsigned __int64)((UINT64)1 << (sizeof(ADCDATATYPE) * 8)));
		char tstr1[100], tstr2[100];
		r.top = DRAW_TEXT_Y;
		r.left = DRAW_TEXT_X;
		r.right = rt.right;
		r.bottom = rt.bottom;
		char t1[100], t2[100], t3[100], t4[100], t5[100], t6[100], t7[100], t8[100];
		sprintf(s, "32pix / DIV\r\n"\
			"AdcSampleRate: %s    Real AdcSampleRate: %s\r\n"\
			"FFT Size: %s  FFT Step: %s  FFTPerSec:%.03f\r\n"\
			"Sepctrum Hz：%s\r\n"\
			"HZoom = %f\r\n"\
			"SDR SampleRate:%s  decimationFactor: %d, %d  BW:%dkHZ\r\n"\
			"rfHz = %s"
			,
			fomatKINT64(clsWaveData.AdcSampleRate, t2), fomatKINT64(clsWaveData.NumPerSec, t3),
			fomatKINT64(clsWaveFFT.FFTSize, t4), fomatKINT64(clsWaveFFT.FFTStep, t5), clsWaveFFT.FFTPerSec,
			formatKDouble(clsWinOneSpectrum.Hz, 0.001, "", t6),
			clsWinSpect.HScrollZoom,
			formatKDouble(clsGetDataSDR.deviceParams->devParams->fsFreq.fsHz, 0.001, "", t7),
			(INT)clsGetDataSDR.chParams->ctrlParams.decimation.enable,
			(INT)clsGetDataSDR.chParams->ctrlParams.decimation.decimationFactor,
			(INT)clsGetDataSDR.chParams->tunerParams.bwType,
			formatKDouble(clsGetDataSDR.chParams->tunerParams.rfFreq.rfHz, 0.001, "", t8)

		);
		SetBkMode(hdc, TRANSPARENT);
		//	SetBkMode(hdc, OPAQUE); 
		//	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);

		SetTextColor(hdc, COLOR_TEXT);
		DrawText(hdc, s, strlen(s), &r, NULL);
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

	SpectrumToWin(hDC);

	EndPaint(hWnd, &ps);
}

void CFFTWin::PaintFFT(HDC hdc, CFFT* fft)
{
	WaitForSingleObject(fft->hMutexBuff, INFINITE);

	double Y = 0;
	int X;
	int FFTLength = HalfFFTSize;
	UINT FFTDrawHeight = FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON;
	double* pBuf = fft->FFTOutBuff;
	double fftvmax;
	double fftvmin;
	HPEN hPen;
	int i = 0;
	UINT64 maxHightPix = DBMin * -64 * VScrollZoom;

	int Xstep = HScrollZoom > 1.0 ? HScrollZoom : 1;
	int istep = HScrollZoom < 1.0 ? ((double)1.0 / HScrollZoom) : 1;
	if (pBuf) {
		double scale = (double)maxHightPix / fft->FFTMaxValue * 100;
		//double scale = FFTDrawHeight / fftvmax;
		fftvmax = pBuf[FFTLength];
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength) Y = FFTDrawHeight - pBuf[i] * scale;
		Y = BOUND(Y, 0, FFTDrawHeight);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = FFTDrawHeight - pBuf[i] * scale;
			Y = BOUND(Y, 0, FFTDrawHeight);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pLogBuf = fft->FFTOutLogBuff;
	if (pLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		fftvmax = pLogBuf[FFTLength];
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength) Y = pLogBuf[i] * -64 * VScrollZoom - VScrollPos;
		Y = BOUND(Y, 0, FFTDrawHeight);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pLogBuf[i] * -64 * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, FFTDrawHeight);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}
	ReleaseMutex(fft->hMutexBuff);

}
void CFFTWin::GetRealClientRect(PRECT lprc)
{
	DWORD dwStyle;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	GetClientRect(hWnd, lprc);
	if (dwStyle & WS_HSCROLL)
		lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (dwStyle & WS_VSCROLL)
		lprc->right += GetSystemMetrics(SM_CXVSCROLL);
}

void CFFTWin::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
{
	INT     iMax, iMin, iPos;
	int		dn = 0, tbdn = 0;
	RECT    rc;

	switch (message)
	{
	case WM_KEYDOWN:
		//printf("winSpectrum KeyAndScroll WM_KEYDOWN\r\n");
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
		GetScrollRange(hWndVScroll, SB_CTL, &iMin, &iMax);
		iPos = GetScrollPos(hWndVScroll, SB_CTL);
		GetClientRect(hWnd, &rc);
		switch (GET_WM_VSCROLL_CODE(wParam, lParam))
		{
		case SB_LINEDOWN:
			dn = 1;
			break;
		case SB_LINEUP:
			dn = -1;
			break;
		case SB_PAGEDOWN:
			dn = (FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON) / 2;
			break;
		case SB_PAGEUP:
			dn = -(FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON) / 2;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			SCROLLINFO si;
			ZeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_TRACKPOS;
			GetScrollInfo(hWndVScroll, SB_CTL, &si);
			tbdn = si.nTrackPos;
			break;
		}
		if (dn != 0)
		{
			VScrollPos = BOUND(VScrollPos + dn, 0, VScrollHeight);
		}
		if (tbdn != 0)
		{
			VScrollPos = BOUND(tbdn, 0, VScrollHeight);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWndVScroll, SB_CTL, VScrollPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		break;
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
			SCROLLINFO si;
			ZeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_TRACKPOS;
			GetScrollInfo(hWnd, SB_HORZ, &si);
			tbdn = si.nTrackPos;
			//			tbdn = GET_WM_HSCROLL_POS(wParam, lParam);
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
		}
		break;
	}
}


void CFFTWin::SpectrumToWin(HDC hDC)
{

	WaitForSingleObject(hDrawMutex, INFINITE);

	RECT rt = WinRect;

	int spectY = SpectrumY + 1;
	int destLeft = rt.left + WAVE_RECT_BORDER_LEFT;
	int destWidth = rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	StretchBlt(hDC,
		destLeft, FFTHeight,
		destWidth, rt.bottom - FFTHeight - spectY,
		hDCFFT,
		0, spectY,
		destWidth, rt.bottom - FFTHeight - spectY,
		SRCCOPY);
	if (spectY)	StretchBlt(hDC,
			destLeft, rt.bottom - spectY,
			destWidth, spectY,
			hDCFFT,
			0, 0,
			destWidth, spectY,
			SRCCOPY);

	//printf("%d,%d,%d,%d\r\n", rt.top, rt.left, rt.right, rt.bottom);
	ReleaseMutex(hDrawMutex);

}


#define FFT_MAX_SAMPLING 16

void CFFTWin::PaintSpectrum(CFFT* fft)
{
	WaitForSingleObject(hDrawMutex, INFINITE);

	double* pBuf = fft->FFTOutLogBuff;

	UINT32 halfFFTSize = HalfFFTSize;
	UINT DrawLen = WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	RECT rt;
	UINT64 color;
	UINT8 c;
	BYTE R, G = 0, C;
	COLORREF cx;

	if (SpectrumY == -1) SpectrumY = WinRect.bottom - FFTHeight - 1;

	SelectObject(hDCFFT, CreateFont(FONT_HEIGHT, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));

	double fftmaxv;
	double fftminv;
	fftmaxv = pBuf[halfFFTSize];
	fftminv = pBuf[halfFFTSize + 1];

	HBRUSH hbr;

	int istep = isSpectrumZoomedShow == true ? 1 : (HScrollZoom < 1.0 ? ((double)1.0 / HScrollZoom) : 1);
	int X;
	double fftmaxdiff = fftmaxv - fftminv;
	double fftmaxdiffstep = fftmaxdiff / 4.0;
	double fftdiffstep1 = fftmaxv - 1 * fftmaxdiffstep;
	double fftdiffstep2 = fftmaxv - 2 * fftmaxdiffstep;
	double fftdiffstep3 = fftmaxv - 3 * fftmaxdiffstep;
	double scroll = isSpectrumZoomedShow == true ? 1.0 : HScrollZoom;
	if (isDrawLogSpectrum == true) {
		X = 0;
		//for (int y = 0; y < DrawLen; y++) 
		for (int i = HScrollPos / scroll;
			i < halfFFTSize &&
			(i * scroll - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			i += istep)
		{
			/*
			UINT vcolor = (UINT)(((double)(1.0*0xFFFFFFFF) / fftmaxdiff) * (pBuf[i] - fftminv));
			//color = BOUND((UINT64)0xFFFFFFFF + (UINT64)0xFFFFFFFF / 10 * pBuf[i], 0, (UINT64)0xFFFFFFFF); // adc最大幅度为 2的32次方，也就是10的10次方.
			if (C = (BYTE)(vcolor >> 24)) cx = RGB(255, ~C, ~C);
			else {
				if (C = (BYTE)(vcolor >> 16)) cx = RGB(C, C, 255);
				else {
					if (C = (BYTE)(vcolor >> 8)) cx = RGB(0, ~C, C);
					else {
						C = (BYTE)vcolor;
						cx = RGB(0, C, 0);
					}
				}
			}
			*/

			//c = BOUND(0xFF + 0xFF / 10 * pBuf[i], 0, 0xFF); // adc最大幅度为 2的32次方，也就是10的10次方.

			 //c = (BYTE)(0xFF / fftmaxdiff) * (pBuf[i] - fftminv);

			//SetPixel(hDCFFT, X, SpectrumY, RGB(~c, ~c, 255));
//			SetPixel(hDCFFT, X, SpectrumY, RGB(~c, ~c, c));
			//SetPixel(hDCFFT, X, SpectrumY, cx);


			double v = pBuf[i];
			double d;
			d = v - fftdiffstep1;
			if (d > 0) {
				C = (BYTE)((0xFF / fftmaxdiffstep) * d);
				cx = RGB(~C, 255, ~C);
			}
			else {
				d = v - fftdiffstep2;
				if (d > 0) {
					C = (BYTE)((0xFF / fftmaxdiffstep) * d);
					cx = RGB(C, C, 255);
				}
				else {
					d = v - fftdiffstep3;
					if (d > 0) {
						C = (BYTE)((0xFF / fftmaxdiffstep) * d);
						cx = RGB(0, 0, C);
					}
					else {
						d = v - fftminv;
						C = (BYTE)((0xFF / fftmaxdiffstep) * d);
						cx = RGB(~C, ~C, 0);
					}
				}
			}
			SetPixel(hDCFFT, X, SpectrumY, cx);
			X++;
		}
	}
	else {
		X = 0;
		for (int i = HScrollPos / scroll;
			i < halfFFTSize &&
			(i * scroll - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			i += istep)
		{
			//			color = (UINT)(((double)(1.0 * 0xFFFFFFFF) / fftmaxdiff) * (pBuf[i] - fftminv));
			//			color = pBuf[y] > pWinData->FFTAvgMaxValue ? (UINT64)0xFFFFFFFF : pBuf[y] * (double)0xFFFFFFFF / pWinData->FFTAvgMaxValue;
						//color = pBuf[i] > clsWaveFFT.FFTMaxValue ? (UINT64)0xFFFFFFFF : pBuf[i] * (double)0xFFFFFFFF / clsWaveFFT.FFTMaxValue;
						//color = pBuf[y] > fftmaxv ? (UINT64)0xFFFFFF : pBuf[y] * (double)0x1FFFFF / fftmaxv;
						//c = pBuf[y] > pWinData->FFTAvgMaxValue ? 0xFF : (0xFF * pBuf[y] / pWinData->FFTAvgMaxValue);

			/*
						if (C = (BYTE)(color >> 24)) cx = RGB(255, ~C, ~C);
						else {
							if (C = (BYTE)(color >> 16)) cx = RGB(C, C, 255);
							else {
								if (C = (BYTE)(color >> 8)) cx = RGB(0, ~C, C);
								else {
									C = (BYTE)color;
									cx = RGB(0, C, 0);
								}
							}
						}
						*/
						//			color = (UINT)(((double)(1.0 * 0xFFFFFFFF) / fftmaxdiff) * (pBuf[i] - fftminv));
			color = (UINT)(((double)(1.0 * 0xFFFFFFFF) / clsWaveFFT.FFTMaxValue) * (pBuf[i]));
			if (C = (BYTE)(color >> 24)) cx = RGB(255, ~C, ~C);
			else {
				if (C = (BYTE)(color >> 16)) cx = RGB(C, C, 255);
				else {
					if (C = (BYTE)(color >> 8)) cx = RGB(~C, ~C, C);
					else {
						C = (BYTE)color;
						cx = RGB(C, 255, ~C);
					}
				}
			}
			/*
						double v = pBuf[i];
						double d;
						d = v - fftdiffstep1;
						if (d > 0) {
							C = (BYTE)((0xFF / fftmaxdiffstep) * d);
							cx = RGB(~C, 255, ~C);
						}
						else {
							d = v - fftdiffstep2;
							if (d > 0) {
								C = (BYTE)((0xFF / fftmaxdiffstep) * d);
								cx = RGB(C, C, 255);
							}
							else {
								d = v - fftminv;
								C = (BYTE)((0xFF / fftmaxdiffstep) * d);
								cx = RGB(0, 0, C);
							}
						}
				*/
			SetPixel(hDCFFT, X, SpectrumY, cx);
			X++;
		}
	}
	DeleteObject(SelectObject(hDCFFT, GetStockObject(SYSTEM_FONT)));
	SpectrumY--;
	ReleaseMutex(hDrawMutex);
}

void CFFTWin::InitDrawBuff(void)
{
	WaitForSingleObject(hDrawMutex, INFINITE);

	SpectrumY = -1;

	RECT rt;
	HDC hDC = GetDC(hWnd);

	rt.top = 0;
	rt.left = 0;
	rt.right = WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	rt.bottom = WinRect.bottom - FFTHeight;

	if (hDCFFT) DeleteObject(hDCFFT);
	if (hBMPFFT) DeleteObject(hBMPFFT);
	hDCFFT = CreateCompatibleDC(hDC);
	hBMPFFT = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hDCFFT, hBMPFFT);
	FillRect(hDCFFT, &rt, (HBRUSH)GetStockObject(BLACK_BRUSH));

	ReleaseDC(hWnd, hDC);
	ReleaseMutex(hDrawMutex);

	if(hFFT_Thread == NULL)	hFFT_Thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFFT::FFT_Thread, this->fft, 0, NULL);

}
