


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
#include "CWinFFT.h"
#include "CWinSpectrum.h"
#include "CDataFromSDR.h"
#include "CWinOneFFT.h"
#include "CWaveAnalyze.h"

using namespace std;

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

CWinOneFFT clsWinOneFFT;

CWinOneFFT::CWinOneFFT()
{
	OPENCONSOLE;

	RegisterWindowsClass();

	rfButton = new CScreenButton::BUTTON{ 10, 10, 100, 100, "0,000,000,000", 32, 1, true, 1, 0, 2000000000, 
		CScreenButton::Button_Align_Right,  CWinOneFFT::rfButtonDraw, CWinOneFFT::rfButtonOnMouse };
	rfStepButton = new CScreenButton::BUTTON{ 10, 20 + 32, 100, 100, "0,000,000,000", 16, 2, true, 1, 1, 2000000000,
		CScreenButton::Button_Align_Right,  CWinOneFFT::rfButtonDraw, CWinOneFFT::rfButtonOnMouse };


}

CWinOneFFT::~CWinOneFFT()
{
	CLOSECONSOLE;
}


void CWinOneFFT::rfButtonDraw(CScreenButton::BUTTON* pButton, HDC hdc, RECT* srcRt)
{
	//CScreenButton::BUTTON* pButton = (CScreenButton::BUTTON*)me;

	HFONT hFont;
	HFONT hFontDefault;
	RECT rt;

	HPEN hPen = (HPEN)GetStockObject(WHITE_PEN);
	SetBkMode(hdc, TRANSPARENT);

	hFont = CreateFont(pButton->fontsize, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("courier"));
	hFontDefault = (HFONT)SelectObject(hdc, hFont);

	rt.top = srcRt->top + pButton->Y;
	rt.left = pButton->alginMode == CScreenButton::BUTTON_ALIGN_MODE::Button_Align_Right ? 
		srcRt->right - pButton->X - pButton->W : srcRt->left + pButton->X;
	rt.right = rt.left + pButton->W;
	rt.bottom = rt.top + pButton->H;

	MoveToEx(hdc, rt.left, rt.top, NULL);
	LineTo(hdc, rt.right, rt.top);
	LineTo(hdc, rt.right, rt.bottom);
	LineTo(hdc, rt.left, rt.bottom);
	LineTo(hdc, rt.left, rt.top);

	DrawText(hdc, pButton->txt, strlen(pButton->txt), &rt, NULL);

	SelectObject(hdc, hFontDefault);
	DeleteObject(hFont);

	SelectObject(hdc, hPen);
}

void CWinOneFFT::rfButtonInit(CScreenButton::BUTTON* pButton)
{
	HDC hdc = GetDC(clsWinOneFFT.hWnd);
	HFONT hFont = CreateFont(pButton->fontsize, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("courier"));
	HFONT hFontDefault = (HFONT)SelectObject(hdc, hFont);
	SIZE sizeText;
	GetTextExtentPoint32(hdc, pButton->txt, strlen(pButton->txt), &sizeText);
	pButton->W = sizeText.cx;
	pButton->H = sizeText.cy;
	SelectObject(hdc, hFontDefault);
	DeleteObject(hFont);
	ReleaseDC(clsWinOneFFT.hWnd, hdc);
}

void CWinOneFFT::rfButtonRefresh(CScreenButton::BUTTON* pButton, INT64 value)
{
	if (value >= pButton->min && value <= pButton->max) {
		pButton->value = value;
		char s[100];
		char* s1 = fomatKINT64Width(pButton->value, 4, s);
		strcpy(pButton->txt, s1 + 2);
	}
}

void CWinOneFFT::rfButtonOnMouse(CScreenButton::BUTTON* pButton, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT MouseX = GET_X_LPARAM(lParam);
	UINT MouseY = GET_Y_LPARAM(lParam);
	RECT rt;

	GetClientRect(clsWinOneFFT.hWnd, &rt);
	rt.top = WAVE_RECT_BORDER_TOP + pButton->Y;
	rt.left = pButton->alginMode == CScreenButton::BUTTON_ALIGN_MODE::Button_Align_Right ? 
		rt.right - WAVE_RECT_BORDER_RIGHT - pButton->X - pButton->W : 
		WAVE_RECT_BORDER_LEFT + pButton->X;
	rt.right = rt.left + pButton->W;
	rt.bottom = rt.top + pButton->H;
	
	if (MouseX > rt.left && MouseX < rt.right && MouseY > rt.top && MouseY < rt.bottom) {
		int W = pButton->W / 13;
		int index = (rt.right - MouseX) / W;
		if ((index + 1) % 4 == 0) return;
		int index2;
		/*
		if (index < 3) index2 = index;
		else if (index == 3) return;
		else if (index < 7)index2 = index - 1;
		else if (index == 7)return;
		else if (index < 11) index2 = index - 2;
		else if (index == 11) return;
		else index2 = index - 3;
		*/
		index2 = index - (index + 1) / 4;

		double rfstep = pow(10, index2);
		INT64 savevalue = pButton->value;
		switch (message)
		{
		case WM_LBUTTONUP:
			pButton->value += rfstep;
			if (pButton->value > pButton->max) pButton->value = pButton->max;
			break;
		case WM_RBUTTONUP:
			pButton->value -= rfstep;
			if (pButton->value < pButton->min) pButton->value = pButton->min;
			break;
		case WM_MOUSEMOVE:
			break;
		default:
			break;
		}
		if (savevalue != pButton->value) {
			if (pButton->key == 1) {
				clsWaveAnalyze.set_SDR_rfHz(pButton->value);
			}
			else if (pButton->key == 2) {
				clsWaveAnalyze.rfHz_Step = pButton->value;
			}

			char s[100];
			char * s1 = fomatKINT64Width(pButton->value, 4, s);
			strcpy(pButton->txt, s1 + 2);

		}
	}
}

void CWinOneFFT::RegisterWindowsClass(void)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinOneFFT::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = FFT_ONE_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

LRESULT CALLBACK CWinOneFFT::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinOneFFT.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinOneFFT::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

			rfButtonInit(rfButton);
			rfButtonInit(rfStepButton);
		}
		break;

	case WM_CHAR:
		printf("WinOneFFT WM_CHAR\r\n");
			PostMessage(clsWinSpect.hWnd, message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		//clsWinSpect.ActivehWnd = NULL;
		break;
	case WM_LBUTTONUP:
		rfButtonOnMouse(rfButton, message, wParam, lParam);
		rfButtonOnMouse(rfStepButton, message, wParam, lParam);
		break;
	case WM_RBUTTONUP:
		rfButtonOnMouse(rfButton, message, wParam, lParam);
		rfButtonOnMouse(rfStepButton, message, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		MouseX = GET_X_LPARAM(lParam);
		MouseY = GET_Y_LPARAM(lParam);
		OnMouse(hWnd);
		rfButtonOnMouse(rfButton, message, wParam, lParam);
		rfButtonOnMouse(rfStepButton, message, wParam, lParam);
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
		//KeyAndScroll(hWnd, message, wParam, lParam);
		break;

	case WM_DESTROY:
		//PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

BOOL CWinOneFFT::OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

void CWinOneFFT::OnMouse(HWND hWnd)
{
	RECT rt;
	GetClientRect(hWnd, &rt);

	int i = 0;
	char s[500], t[100];
	FILTERCOREDATATYPE* pFilterCore = clsWaveFilter.pCurrentFilterInfo->FilterCore;
	int FilterLength = clsWaveFilter.pCurrentFilterInfo->CoreLength;
	int X = (clsWinSpect.HScrollPos + MouseX - WAVE_RECT_BORDER_LEFT) / clsWinSpect.HScrollZoom;
	X = BOUND(X, 0, (clsWinSpect.HScrollPos + rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / clsWinSpect.HScrollZoom);
	double Y = X > FilterLength ? 0 : pFilterCore[X];
	int n = 0;
	n += sprintf(strMouse + n, "X: %d, core V: %lf", X, Y);

	double Hz = (double)X * clsWaveData.AdcSampleRate / clsWaveFFT.FFTSize;
	Y = X > clsWaveFFT.FFTSize / 2 ? 0 : clsWinSpect.OrignalFFTBuff[X];
	n += sprintf(strMouse + n, " | ");
	n += sprintf(strMouse + n, "Hz: %.03f, FFT: %s", Hz, formatKDouble(Y, 0.001, "", t));

	int Ypos = BOUND(MouseY - WAVE_RECT_BORDER_TOP, 0, WAVE_RECT_HEIGHT);
	double Ylog = 20 * (X > clsWaveFFT.FFTSize / 2 ? 0 : clsWinSpect.FilttedFFTBuffLog[X]);
	n += sprintf(strMouse + n, " | ");
	n += sprintf(strMouse + n, "Y: %.03fdb, logFFT: %fdb", -(double)Ypos / 64 * 20, Ylog);

}

VOID CWinOneFFT::Paint(HWND hWnd)
{
	HDC		hDC;
	PAINTSTRUCT ps;
	RECT	rt, r, buttonRt;
	HPEN	hPen, hPenLighter;

	hDC = BeginPaint(hWnd, &ps);

	//EndPaint(hWnd, &ps);
	//return;

	GetClientRect(hWnd, &rt);
	buttonRt = rt;

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

	for (x = WAVE_RECT_BORDER_LEFT, i = 0; x < rt.right - WAVE_RECT_BORDER_RIGHT; x += 8)
	{
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP - ((x - WAVE_RECT_BORDER_LEFT) % 32 ? DIVSHORT : DIVLONG), NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP);
		if (!((x - WAVE_RECT_BORDER_LEFT) % 32))
		{
			if (!(i % 5))
			{
				sprintf(s, "%.02fhz", (double)(i * 32 + clsWinSpect.HScrollPos) / clsWinSpect.HScrollZoom * clsWaveData.AdcSampleRate / clsWaveFFT.FFTSize);
				r.top = WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT + DIVLONG;
				r.left = x;
				SetTextColor(hdc, COLOR_ORIGNAL_FFT);
				DrawText(hdc, s, strlen(s), &r, NULL);
				sprintf(s, "%d", (int)((i * 32 + clsWinSpect.HScrollPos) / clsWinSpect.HScrollZoom));
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
	//double CoreCenter = 256;
	int FFTLength = clsWaveFFT.FFTSize / 2;

	WaitForSingleObject(clsWinSpect.hMutexBuff, INFINITE);

	PaintBriefly(hdc);

	//绘制滤波 FFT 信号--------------------------------------------------
	double* pOriBuf = clsWinSpect.OrignalFFTBuff;
	double* pFilBuf = clsWinSpect.FilttedFFTBuff;
	double fftvmax;
	double fftvmin;
	if (pOriBuf && pFilBuf) fftvmax = max(pOriBuf[FFTLength], pFilBuf[FFTLength]);
//	double scale = WAVE_RECT_HEIGHT / fftvmax;
	double scale = WAVE_RECT_HEIGHT / clsWaveFFT.FFTMaxValue * 100;
	int Xstep = clsWinSpect.HScrollZoom > 1.0 ? clsWinSpect.HScrollZoom : 1;
	int istep = clsWinSpect.HScrollZoom < 1.0 ? ((double)1.0 / clsWinSpect.HScrollZoom) : 1;
	if (bFFTOrignalShow && pOriBuf)
	{
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength) Y = WAVE_RECT_HEIGHT - pOriBuf[i] * scale;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = WAVE_RECT_HEIGHT - pOriBuf[i] * scale;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}
	if (bFFTFilttedShow && pFilBuf)
	{
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength) Y = WAVE_RECT_HEIGHT - pFilBuf[i] * scale;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = WAVE_RECT_HEIGHT - pFilBuf[i] * scale;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pOriLogBuf = clsWinSpect.OrignalFFTBuffLog;
	if (bFFTOrignalLogShow && pOriLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		fftvmax = pOriLogBuf[FFTLength];
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength)
			Y = pOriLogBuf[i] * -64;
		Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pOriLogBuf[i] * -64;
			Y = BOUND(Y, 0, 64 * 12);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pFilLogBuf = clsWinSpect.FilttedFFTBuffLog;
	if (bFFTFilttedLogShow && pFilLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength)
			Y = pFilLogBuf[i] * -64;
		Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
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
		r.top = WAVE_RECT_BORDER_TOP + DIVLONG + 80;
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
		double VotagePerDIV = (FullVotage / (unsigned __int64)((UINT64)1 << (sizeof(ADCDATATYPE) * 8)));
		char tstr1[100], tstr2[100];
		r.top = DRAW_TEXT_Y;
		r.left = DRAW_TEXT_X;
		r.right = rt.right;
		r.bottom = rt.bottom;
		char t1[100], t2[100], t3[100], t4[100], t5[100], t6[100], t7[100], t8[100];
		sprintf(s, "32pix / DIV\r\n"\
			"Core Length: %s\r\n"\
			"AdcSampleRate: %s    Real AdcSampleRate: %s\r\n"\
			"FFT Size: %s  FFT Step: %s  FFTPerSec:%.03f\r\n"\
			"Sepctrum Hz：%s\r\n"\
			"HZoom = %f\r\n"\
			"SDR SampleRate:%s  decimationFactor: %d, %d  BW:%dkHZ\r\n"\
			"rfHz = %s"
			,
			fomatKINT64(CoreLength, t1),
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

	buttonRt.top += WAVE_RECT_BORDER_TOP;
	buttonRt.left += WAVE_RECT_BORDER_LEFT;
	buttonRt.right -= WAVE_RECT_BORDER_RIGHT;
	buttonRt.bottom = WAVE_RECT_BORDER_TOP + WAVE_RECT_BORDER_TOP;

	rfButton->Draw(rfButton, hdc, &buttonRt);
	rfStepButton->Draw(rfStepButton, hdc, &buttonRt);

	BitBlt(hDC,
		0, 0,
		rt.right, rt.bottom,
		hdc,
		0, 0,
		SRCCOPY);
	DeleteObject(hdc);
	DeleteObject(hbmp);

	ReleaseMutex(clsWinSpect.hMutexBuff);

	EndPaint(hWnd, &ps);
}

VOID CWinOneFFT::GetRealClientRect(HWND hWnd, PRECT lprc)
{
	DWORD dwStyle;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	GetClientRect(hWnd, lprc);
	if (dwStyle & WS_HSCROLL)
		lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (dwStyle & WS_VSCROLL)
		lprc->right += GetSystemMetrics(SM_CXVSCROLL);
}

VOID CWinOneFFT::BrieflyBuff(WHICHSIGNAL which)
{

	WaitForSingleObject(clsWinSpect.hMutexBuff, INFINITE);

	double* pfft, * pfftlog;
	double* pbrifft, * pbrifftlog;
	if (which == WHICHSIGNAL::SIGNAL_ORIGNAL) {
		pfft = clsWinSpect.OrignalFFTBuff;
		pfftlog = clsWinSpect.OrignalFFTBuffLog;
		pbrifft = clsWinSpect.BrieflyOrigFFTBuff;
		pbrifftlog = clsWinSpect.BrieflyOrigFFTBuffLog;
	}
	else {
		pfft = clsWinSpect.FilttedFFTBuff;
		pfftlog = clsWinSpect.FilttedFFTBuffLog;
		pbrifft = clsWinSpect.BrieflyFiltFFTBuff;
		pbrifftlog = clsWinSpect.BrieflyFiltFFTBuffLog;
	}

	int stepn = 1 / clsWinSpect.HScrollZoom;
	int N = clsWaveFFT.HalfFFTSize / stepn;
	double maxd;
	double maxdlog;
	double mind = DBL_MAX;
	double minlogd = DBL_MAX;
	for (int i = 0; i < N; i++) {
		maxd = DBL_MIN;
		maxdlog = -1.0 * DBL_MAX;
		int nn = i * stepn;
		for (int n = 0; n < stepn; n++) {
			if (pfft[nn + n] > maxd) 
				maxd = pfft[nn + n];
			if (pfftlog[nn + n] > maxdlog) 
				maxdlog = pfftlog[nn + n];
		}
		pbrifft[i] = maxd;
		pbrifftlog[i] = maxdlog;
		if (mind > maxd) mind = maxd;
		if (minlogd > maxdlog) minlogd = maxdlog;
	}
	pbrifft[clsWaveFFT.HalfFFTSize] = pfft[clsWaveFFT.HalfFFTSize];
	pbrifft[clsWaveFFT.HalfFFTSize + 1] = mind;// pfft[clsWaveFFT.HalfFFTSize + 1];
	pbrifftlog[clsWaveFFT.HalfFFTSize] = pfftlog[clsWaveFFT.HalfFFTSize];
	pbrifftlog[clsWaveFFT.HalfFFTSize + 1] = minlogd;// pfftlog[clsWaveFFT.HalfFFTSize + 1];

	ReleaseMutex(clsWinSpect.hMutexBuff);
}

void CWinOneFFT::PaintBriefly(HDC hdc)
{
	HPEN hPen;
	double Y = 0;
	int X;
	int FFTLength = clsWaveFFT.HalfFFTSize;
	int i;
	RECT	rt;
	GetClientRect(hWnd, &rt);

	//绘制滤波 FFT 信号--------------------------------------------------
	double* pOriBuf = clsWinSpect.BrieflyOrigFFTBuff;
	double* pFilBuf = clsWinSpect.BrieflyFiltFFTBuff;
	double fftvmax;
	double fftvmin;
	if (pOriBuf && pFilBuf) fftvmax = max(pOriBuf[FFTLength], pFilBuf[FFTLength]);
	//double scale = 2 * CoreCenter / fftvmax;
	double scale = WAVE_RECT_HEIGHT / clsWaveFFT.FFTMaxValue * 100;
	int Xstep = clsWinSpect.HScrollZoom > 1.0 ? clsWinSpect.HScrollZoom : 1;
	int istep = 1;// clsWinSpect.HScrollZoom < 1.0 ? ((double)1.0 / clsWinSpect.HScrollZoom) : 1;
	if (bFFTOrignalShow && pOriBuf)
	{
//		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		i = clsWinSpect.HScrollPos;
		if (i < FFTLength) Y = WAVE_RECT_HEIGHT - pOriBuf[i] * scale;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = WAVE_RECT_HEIGHT - pOriBuf[i] * scale;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}
	if (bFFTFilttedShow && pFilBuf)
	{
		i = clsWinSpect.HScrollPos;// / clsWinSpect.HScrollZoom;
		if (i < FFTLength) Y = WAVE_RECT_HEIGHT - pFilBuf[i] * scale;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = WAVE_RECT_HEIGHT - pFilBuf[i] * scale;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pOriLogBuf = clsWinSpect.BrieflyOrigFFTBuffLog;
	if (bFFTOrignalLogShow && pOriLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		fftvmax = pOriLogBuf[FFTLength];
		i = clsWinSpect.HScrollPos;// / clsWinSpect.HScrollZoom;
		if (i < FFTLength)
			Y = pOriLogBuf[i] * -64;
		Y = BOUND(Y, 0, 64 * 12);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pOriLogBuf[i] * -64;
			Y = BOUND(Y, 0, 64 * 12);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pFilLogBuf = clsWinSpect.BrieflyFiltFFTBuffLog;
	if (bFFTFilttedLogShow && pFilLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		i = clsWinSpect.HScrollPos;// / clsWinSpect.HScrollZoom;
		if (i < FFTLength)
			Y = pFilLogBuf[i] * -64;
		Y = BOUND(Y, 0, 64 * 12);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pFilLogBuf[i] * -64;
			Y = BOUND(Y, 0, 64 * 12);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}
}