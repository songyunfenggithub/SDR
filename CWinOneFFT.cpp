


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
#include "CData.h"
#include "CWaveFFT.h"
#include "CWaveFilter.h"
#include "CWinFFT.h"
#include "CWinSpectrum.h"
#include "CDataFromSDR.h"
#include "CWinOneFFT.h"
#include "CAnalyze.h"

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

#define FONT_HEIGHT	14

#define TIMEOUT		100

#define DIVLONG		10
#define DIVSHORT	5

CWinOneFFT clsWinOneFFT;

CWinOneFFT::CWinOneFFT()
{
	OPENCONSOLE;

	RegisterWindowsClass();
	int Y = 10;
	rfButton = new CScreenButton(new CScreenButton::BUTTON{ 10, Y, 100, 100, 13,  NULL, "0,000,000,000", CScreenButton::BUTTON_FONT_SIZE::Button_Font_Size_32, CScreenButton::Button_Mouse_Num1, CScreenButton::Button_Mouse_None,
		true, 0, 0, 0, 2000000000,
		CScreenButton::Button_Align_Right,  NULL, NULL, NULL});
	rfStepButton = new CScreenButton(new CScreenButton::BUTTON{ 10, Y += 32, 100, 100, 13, NULL, "0,000,000,000", CScreenButton::BUTTON_FONT_SIZE::Button_Font_Size_16,CScreenButton::Button_Mouse_Num2, CScreenButton::Button_Mouse_None,
		true, 1, 0, 0, 2000000000,
		CScreenButton::Button_Align_Right,  NULL, NULL, NULL});
	averageFilterButton = new CScreenButton(new CScreenButton::BUTTON{ 10, Y += 20, 100, 100, 9, NULL, "AvgFlt:", CScreenButton::BUTTON_FONT_SIZE::Button_Font_Size_16, CScreenButton::Button_Step_Num, CScreenButton::Button_Mouse_None,
		true, 1, 1, 2, FFT_DEEP,
		CScreenButton::Button_Align_Right,  NULL, NULL, CWinOneFFT::averageFilterButtonFunc });
	confirmP1RfGoButton = new CScreenButton(new CScreenButton::BUTTON{ 10, Y += 20, 100, 100, 4, NULL, "RfGo", CScreenButton::BUTTON_FONT_SIZE::Button_Font_Size_16, CScreenButton::Button_Confirm, CScreenButton::Button_Mouse_None,
		true, 1, 1, 2, FFT_DEEP,
		CScreenButton::Button_Align_Right,  NULL, NULL, CWinOneFFT::confirmP1RfGoButtonFunc });
}

CWinOneFFT::~CWinOneFFT()
{
	//CLOSECONSOLE;
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
	switch (message)
	{
	case WM_CREATE:
	{
		OPENCONSOLE;
		clsWinOneFFT.hWnd = hWnd;
		uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		//KillTimer(hWnd, uTimerId);
		

		rfButton->ButtonInit(hWnd);
		rfStepButton->ButtonInit(hWnd);
		averageFilterButton->ButtonInit(hWnd);
		confirmP1RfGoButton->ButtonInit(hWnd);

		RestoreValue();
		clsAnalyze.set_SDR_rfHz(rfButton->Button->value);
		rfButton->RefreshMouseNumButton(rfButton->Button->value);
		rfStepButton->RefreshMouseNumButton(rfStepButton->Button->value);
	}
	break;
	case WM_CHAR:
		printf("WinOneFFT WM_CHAR\r\n");
		PostMessage(clsWinSpect.hWnd, message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		if (confirmP1RfGoButton->MouseInButton(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == false
			&& rfButton->MouseInButton(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == false) {
			POINT* P;
			P = message == WM_LBUTTONUP ? &ScreenP1 : &ScreenP2;
			P->x = GET_X_LPARAM(lParam);
			P->y = GET_Y_LPARAM(lParam);
			if (P->x > WAVE_RECT_BORDER_LEFT && P->x < WinRect.right - WAVE_RECT_BORDER_RIGHT
				&& P->y > WAVE_RECT_BORDER_TOP && P->y < WinRect.bottom - WAVE_RECT_BORDER_BOTTON)
			{
				if (message == WM_LBUTTONUP) P1_Use = true; else P2_Use = true;
			}
			else {
				if (message == WM_LBUTTONUP) P1_Use = false; else P2_Use = false;
			}
			P2SubP1();
		}
	case WM_MOUSEMOVE:
		if (message == WM_MOUSEMOVE) {
			MouseX = GET_X_LPARAM(lParam);
			MouseY = GET_Y_LPARAM(lParam);
			OnMouse();
		}
		rfButton->OnMouseMouseNumButton(hWnd, message, wParam, lParam);
		rfStepButton->OnMouseMouseNumButton(hWnd, message, wParam, lParam);
		averageFilterButton->OnMouseButton(hWnd, message, wParam, lParam);
		confirmP1RfGoButton->OnMouseButton(hWnd, message, wParam, lParam);
		break;
	case WM_LBUTTONDBLCLK:
			DbgMsg("WM_LBUTTONDBLCLK\r\n");
		break;
	case WM_TIMER:
		if (bFFTHold == false && FFTNeedReDraw == false) {
			FFTNeedReDraw = true;
		}
		//InvalidateRect(hWnd, NULL, TRUE);
		//UpdateWindow(hWnd);
		break;
	case WM_SIZE:
		GetRealClientRect(&WinRect);
		VScrollWidth = 2 * WAVE_RECT_HEIGHT * VScrollZoom - (WinRect.bottom - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON);
		UP_TO_ZERO(VScrollWidth);
		SetScrollRange(hWnd, SB_VERT, 0, VScrollWidth, TRUE);
		GetClientRect(hWnd, &WinRect);
		break;
	case WM_COMMAND:
		return OnCommand(message, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;
	case WM_PAINT:
		Paint();
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		KeyAndScroll(message, wParam, lParam);
		break;
	case WM_DESTROY:
		DbgMsg("CWinOneFFT WM_DESTROY\r\n");
		SaveValue();
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CWinOneFFT::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDM_FFT_VERT_ZOOM_INCREASE:
		if (VScrollZoom < FFT_ZOOM_MAX) {
			VScrollZoom *= 2.0;
			PostMessage(hWnd, WM_SIZE, wParam, lParam);
		}
		break;
	case IDM_FFT_VERT_ZOOM_DECREASE:
		if (VScrollZoom > 0.5) {
			VScrollZoom /= 2.0;
			PostMessage(hWnd, WM_SIZE, wParam, lParam);
		}
		break;
	case IDM_FFT_VERT_ZOOM_HOME:
		VScrollZoom = 1.0;
		PostMessage(hWnd, WM_SIZE, wParam, lParam);
		break;
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

void CWinOneFFT::OnMouse(void)
{
	RECT rt;
	GetClientRect(hWnd, &rt);

	int i = 0;
	char s[500], t[100];
	FILTER_CORE_DATA_TYPE* pFilterCore = clsWaveFilter.pCurrentFilterInfo->FilterCore;
	int FilterLength = clsWaveFilter.pCurrentFilterInfo->CoreLength;
	int X = (clsWinSpect.HScrollPos + MouseX - WAVE_RECT_BORDER_LEFT) / clsWinSpect.HScrollZoom;
	X = BOUND(X, 0, (clsWinSpect.HScrollPos + rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / clsWinSpect.HScrollZoom);
	double Y = X > FilterLength ? 0 : pFilterCore[X];
	int n = 0;
	n += sprintf(strMouse + n, "X: %d, core V: %lf", X, Y);

	double Hz = (double)X * clsData.AdcSampleRate / clsWaveFFT.FFTSize;
	Y = X > clsWaveFFT.FFTSize / 2 ? 0 : clsWinSpect.OrignalFFTBuff[X];
	n += sprintf(strMouse + n, " | ");
	n += sprintf(strMouse + n, "Hz: %.03f, FFT: %s", Hz, formatKDouble(Y, 0.001, "", t));

	int Ypos = BOUND(MouseY - WAVE_RECT_BORDER_TOP, 0, WAVE_RECT_HEIGHT);
	double Ylog = 20 * (X > clsWaveFFT.FFTSize / 2 ? 0 : clsWinSpect.FilttedFFTBuffLog[X]);
	n += sprintf(strMouse + n, " | ");
	n += sprintf(strMouse + n, "Y: %.03fdb, logFFT: %fdb", -(double)Ypos / 64 * 20, Ylog);

}

void CWinOneFFT::Paint(void)
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

	HFONT hFont_Arial = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial"));
	SelectObject(hdc, hFont_Arial);

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

	SetTextColor(hdc, COLOR_ORIGNAL_FFT);
#define WEB_H_STEP	8
	UINT web_hoffset = WEB_H_STEP - clsWinSpect.HScrollPos % WEB_H_STEP;
	if (web_hoffset == WEB_H_STEP)web_hoffset = 0;
	UINT long_step = WEB_H_STEP * 4;
	UINT FFTDrawHeight = WinRect.bottom - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON;
	for (x = WAVE_RECT_BORDER_LEFT + web_hoffset; x <= rt.right - WAVE_RECT_BORDER_RIGHT; x += WEB_H_STEP)
	{
		UINT64 realPos = clsWinSpect.HScrollPos + x - WAVE_RECT_BORDER_LEFT;
		UINT LineOffset = realPos % long_step ? DIVSHORT : DIVLONG;
		UINT64 lineNum = realPos / WEB_H_STEP;
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP - LineOffset, NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP);

		if ((lineNum % 4) == 0) {
			if ((lineNum % 20) == 0) {
				UINT64 pos = (UINT64)(((double)realPos) / clsWinSpect.HScrollZoom);
				sprintf(s, "%lu", pos);
				r.left = x;
				r.top = WAVE_RECT_BORDER_TOP - DIVLONG - FONT_HEIGHT;
				DrawText(hdc, s, strlen(s), &r, NULL);
				//sprintf(s, "%.6fs", (double)pos / *SampleRate);
				//formatKKDouble((double)pos / *SampleRate, "s", s);
				sprintf(s, "%.03fhz", (double)(pos * clsData.AdcSampleRate / clsWaveFFT.FFTSize));
				r.top = WAVE_RECT_BORDER_TOP + (FFTDrawHeight)+DIVLONG;
				DrawText(hdc, s, strlen(s), &r, NULL);
			}
			SelectObject(hdc, lineNum % 20 ? hPen : hPenLighter);
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
	FFTDrawHeight = WAVE_RECT_HEIGHT;
	long_step = WEB_H_STEP * 4;
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
			if (lineNum % 8 == 0)
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
	int averagei = 0;
	if (bFFTOrignalShow && pOriBuf)
	{
		averageOrignal = 0.0;
		minOrignal = DBL_MAX;;
		maxOrignal = -1.0 * DBL_MAX;
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength) 
			Y = WAVE_RECT_HEIGHT - pOriBuf[i] * scale * VScrollZoom - VScrollPos;
		Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
		averageOrignal += Y;
		minOrignal = min(minOrignal, Y);
		maxOrignal = max(maxOrignal, Y);
		averagei++;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = WAVE_RECT_HEIGHT - pOriBuf[i] * scale * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
			averageOrignal += Y;
			minOrignal = min(minOrignal, Y);
			maxOrignal = max(maxOrignal, Y);
			averagei++;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		averageOrignal /= averagei;
		DeleteObject(hPen);
	}
	if (bFFTFilttedShow && pFilBuf)
	{
		averagei = 0;
		averageFiltted = 0.0;
		minFiltted = DBL_MAX;;
		maxFiltted = -1.0 * DBL_MAX;
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength)
			Y = WAVE_RECT_HEIGHT - pFilBuf[i] * scale * VScrollZoom - VScrollPos;
		Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
		averageFiltted += Y;
		minFiltted = min(minFiltted, Y);
		maxFiltted = max(maxFiltted, Y);
		averagei++;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = WAVE_RECT_HEIGHT - pFilBuf[i] * scale * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
			averageFiltted += Y;
			minFiltted = min(minFiltted, Y);
			maxFiltted = max(maxFiltted, Y);
			averagei++;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		averageFiltted /= averagei;
		DeleteObject(hPen);
	}

	averageOrignalLog = 0.0;
	minOrignalLog = DBL_MAX;;
	maxOrignalLog = -1.0 * DBL_MAX;
	double* pOriLogBuf = clsWinSpect.OrignalFFTBuffLog;
	if (bFFTOrignalLogShow && pOriLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		averagei = 0;
		fftvmax = pOriLogBuf[FFTLength];
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength)
			Y = pOriLogBuf[i] * -64 * VScrollZoom - VScrollPos;
		Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
		averageOrignalLog += Y;
		minOrignalLog = min(minOrignalLog, Y);
		maxOrignalLog = max(maxOrignalLog, Y);
		averagei++;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pOriLogBuf[i] * -64 * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, 64 * 12);
			averageOrignalLog += Y;
			minOrignalLog = min(minOrignalLog, Y);
			maxOrignalLog = max(maxOrignalLog, Y);
			averagei++;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		averageOrignalLog /= averagei;
		MoveToEx(hdc, WAVE_RECT_BORDER_LEFT, WAVE_RECT_BORDER_TOP + averageOrignalLog, NULL);
		LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, WAVE_RECT_BORDER_TOP + averageOrignalLog);
		DeleteObject(hPen);
	}

	averagei = 0;
	averageFilttedLog = 0.0;
	minFilttedLog = DBL_MAX;;
	maxFilttedLog = -1.0 * DBL_MAX;
	double* pFilLogBuf = clsWinSpect.FilttedFFTBuffLog;
	if (bFFTFilttedLogShow && pFilLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength)
			Y = pFilLogBuf[i] * -64 * VScrollZoom - VScrollPos;
		Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
		averageFilttedLog += Y;
		minFilttedLog = min(minFilttedLog, Y);
		maxFilttedLog = max(maxFilttedLog, Y);
		averagei++;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pFilLogBuf[i] * -64 * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, 64 * 12);
			averageFilttedLog += Y;
			minFilttedLog = min(minFilttedLog, Y);
			maxFilttedLog = max(maxFilttedLog, Y);
			averagei++;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		averageFilttedLog /= averagei;
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
#define DRAW_TEXT_Y		(WAVE_RECT_BORDER_TOP + DIVLONG + 50)
		double FullVotage = 5.0;
		double VotagePerDIV = (FullVotage / (unsigned __int64)((UINT64)1 << (sizeof(ADCDATATYPE) * 8)));
		char tstr1[100], tstr2[100];
		r.top = DRAW_TEXT_Y;
		r.left = DRAW_TEXT_X;
		r.right = rt.right;
		r.bottom = rt.bottom;
		char t1[100], t2[100], t3[100], t4[100], t5[100], t6[100], t7[100], t8[100];
		sprintf(s, "32pix/DIV %.03fHz/Div\r\n"\
			"Filter: Desc=%s, decimationFactorBit=%d, SampleRate=%d\r\n"\
			"AdcSampleRate: %s    Real AdcSampleRate: %s\r\n"\
			"FFT Size: %s  FFT Step: %s  FFTPerSec:%.03f\r\n"\
			"Sepctrum Hz：%s\r\n"\
			"HZoom=%f VZoom=%f\r\n"\
			"SDR SampleRate:%s  decimationFactor: %d, %d  BW:%dkHZ\r\n"\
			"rfHz = %s"
			,
			clsData.AdcSampleRate * (32 / clsWinSpect.HScrollZoom) / clsWaveFFT.FFTSize,
			clsWaveFilter.rootFilterInfo.CoreDescStr, 
			clsWaveFilter.rootFilterInfo.decimationFactorBit, 
			clsWaveFilter.rootFilterInfo.SampleRate,
			fomatKINT64(clsData.AdcSampleRate, t2), fomatKINT64(clsData.NumPerSec, t3),
			fomatKINT64(clsWaveFFT.FFTSize, t4), fomatKINT64(clsWaveFFT.FFTStep, t5), clsWaveFFT.FFTPerSec,
			formatKDouble(clsWinOneSpectrum.Hz, 0.001, "", t6),
			clsWinSpect.HScrollZoom, VScrollZoom,
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
		r.right = rt.right;
		r.bottom = rt.bottom;
		SetTextColor(hdc, COLOR_ORIGNAL_FFT);
		DrawText(hdc, strPP, strlen(strPP), &r, NULL);
	}

	DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

	buttonRt.top += WAVE_RECT_BORDER_TOP;
	buttonRt.left += WAVE_RECT_BORDER_LEFT;
	buttonRt.right -= WAVE_RECT_BORDER_RIGHT;
	buttonRt.bottom = WAVE_RECT_BORDER_TOP + WAVE_RECT_BORDER_TOP;

	rfButton->DrawMouseNumButton(hdc, &buttonRt);
	rfStepButton->DrawMouseNumButton(hdc, &buttonRt);
	averageFilterButton->DrawButton(hdc, &buttonRt);
	confirmP1RfGoButton->DrawButton(hdc, &buttonRt);

	DrawPoint(hdc, &ScreenP1, hFont_Arial);
	DrawPoint(hdc, &ScreenP2, hFont_Arial);

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

void CWinOneFFT::DrawPoint(HDC hdc, POINT* P, HFONT hFont)
{
#define PW 5
#define PW2 10
	RECT r;

	if (P->x > WAVE_RECT_BORDER_LEFT && P->x < WinRect.right - WAVE_RECT_BORDER_RIGHT
		&& P->y > WAVE_RECT_BORDER_TOP && P->y < WinRect.bottom - WAVE_RECT_BORDER_BOTTON)
	{
		HPEN hPen = CreatePen(PS_SOLID, 0, RGB(128, 255, 128));;
		HPEN hPen_old = (HPEN)SelectObject(hdc, (HPEN)hPen);
		HFONT hFont_old = (HFONT)SelectObject(hdc, (HFONT)hFont);

		MoveToEx(hdc, P->x - PW, P->y - PW, NULL);
		LineTo(hdc, P->x + PW, P->y - PW);
		LineTo(hdc, P->x + PW, P->y + PW);
		LineTo(hdc, P->x - PW, P->y + PW);
		LineTo(hdc, P->x - PW, P->y - PW);

		MoveToEx(hdc, P->x - PW2, P->y, NULL);
		LineTo(hdc, P->x + PW2, P->y);
		MoveToEx(hdc, P->x, P->y - PW2, NULL);
		LineTo(hdc, P->x, P->y + PW2);
		r.top = P->y - PW2 / 2;
		r.left = P->x + PW2 + 2;
		r.right = WinRect.right;
		r.bottom = WinRect.bottom;
		DrawText(hdc, P == &ScreenP1 ? "P1" : "P2", 2, &r, NULL);

		SelectObject(hdc, (HPEN)hPen_old);
		SelectObject(hdc, (HFONT)hFont_old);
		DeleteObject(hPen);
	}
}

void CWinOneFFT::GetRealClientRect(PRECT lprc)
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
	if (stepn == 0)stepn = 1;
	int N = clsWaveFFT.HalfFFTSize / stepn;
	double maxd;
	double maxdlog;
	double mind = DBL_MAX;
	double minlogd = DBL_MAX;
	for (int i = 0; i < N; i++) {
		maxd = -1.0 * DBL_MAX;
		maxdlog = maxd;
		int nn = i * stepn;
		for (int n = 0; n < stepn; n++) {
			if (pfft[nn + n] > maxd) 
				maxd = pfft[nn + n];
			if (pfftlog[nn + n] > maxdlog) 
				maxdlog = pfftlog[nn + n];
		}
		int ii = i * stepn;
		pbrifft[ii] = maxd;
		pbrifftlog[ii] = maxdlog;
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
	double scale = WAVE_RECT_HEIGHT / clsWaveFFT.FFTMaxValue * 100;
	int Xstep = clsWinSpect.HScrollZoom > 1.0 ? clsWinSpect.HScrollZoom : 1;
	int istep = clsWinSpect.HScrollZoom < 1.0 ? ((double)1.0 / clsWinSpect.HScrollZoom) : 1;
	if (bFFTOrignalShow && pOriBuf)
	{
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength) Y = WAVE_RECT_HEIGHT - pOriBuf[i] * scale * VScrollZoom - VScrollPos;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = WAVE_RECT_HEIGHT - pOriBuf[i] * scale * VScrollZoom - VScrollPos;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}
	if (bFFTFilttedShow && pFilBuf)
	{
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength) Y = WAVE_RECT_HEIGHT - pFilBuf[i] * scale * VScrollZoom - VScrollPos;
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = WAVE_RECT_HEIGHT - pFilBuf[i] * scale * VScrollZoom - VScrollPos;
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pOriLogBuf = clsWinSpect.BrieflyOrigFFTBuffLog;
	if (bFFTOrignalLogShow && pOriLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		fftvmax = pOriLogBuf[FFTLength];
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength)
			Y = pOriLogBuf[i] * -64 * VScrollZoom - VScrollPos;
		Y = BOUND(Y, 0, 64 * 12);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_ORIGNAL_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pOriLogBuf[i] * -64 * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, 64 * 12);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}

	double* pFilLogBuf = clsWinSpect.BrieflyFiltFFTBuffLog;
	if (bFFTFilttedLogShow && pFilLogBuf) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		if (i < FFTLength)
			Y = pFilLogBuf[i] * -64 * VScrollZoom - VScrollPos;
		Y = BOUND(Y, 0, 64 * 12);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		hPen = CreatePen(PS_SOLID, 1, COLOR_FILTTED_FFT_LOG);
		SelectObject(hdc, hPen);
		for (i += istep; i < FFTLength && (i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pFilLogBuf[i] * -64 * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, 64 * 12);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}
		DeleteObject(hPen);
	}
}

void CWinOneFFT::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
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
		GetScrollRange(hWnd, SB_VERT, &iMin, &iMax);
		iPos = GetScrollPos(hWnd, SB_VERT);
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
			dn = (rc.bottom - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON) / 2;
			break;
		case SB_PAGEUP:
			dn = -(rc.bottom - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON) / 2;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			SCROLLINFO si;
			ZeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_TRACKPOS;
			GetScrollInfo(hWnd, SB_VERT, &si);
			tbdn = si.nTrackPos;

			break;
		}
		if (dn != 0)
		{
			VScrollPos = BOUND(VScrollPos + dn, 0, VScrollWidth);
		}
		if (tbdn != 0)
		{
			VScrollPos = BOUND(tbdn, 0, VScrollWidth);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_VERT, VScrollPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
			//printf("clsWinSpect.HScrollPos: %d, clsWinSpect.HScrollWidth: %d.\r\n", clsWinSpect.HScrollPos, clsWinSpect.HScrollWidth);
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
			//HScrollPos = BOUND(HScrollPos + dn, 0, HScrollWidth);
		}
		if (tbdn != 0)
		{
			//HScrollPos = BOUND(tbdn, 0, HScrollWidth);
		}
		if (dn != 0 || tbdn != 0)
		{
			//SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
			//InvalidateRect(hWnd, NULL, TRUE);
			//UpdateWindow(hWnd);
			//printf("clsWinSpect.HScrollPos: %d, clsWinSpect.HScrollWidth: %d.\r\n", clsWinSpect.HScrollPos, clsWinSpect.HScrollWidth);
		}
		break;
	}
}

void CWinOneFFT::SaveValue(void)
{
#define VALUE_LENGTH	100
	char section[VALUE_LENGTH];
	sprintf(section, "CWinOneFFT");
	char value[VALUE_LENGTH];
	WritePrivateProfileString(section, "RF", std::to_string(rfButton->Button->value).c_str(), IniFilePath);
	WritePrivateProfileString(section, "RF_STEP", std::to_string(rfStepButton->Button->value).c_str(), IniFilePath);
	WritePrivateProfileString(section, "AverageFilterValue", std::to_string(averageFilterButton->Button->value).c_str(), IniFilePath);
	WritePrivateProfileString(section, "VScrollPos", std::to_string(VScrollPos).c_str(), IniFilePath);
	sprintf(value, "%.20f", VScrollZoom);
	WritePrivateProfileString(section, "VScrollZoom", value, IniFilePath);
}

void CWinOneFFT::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	char section[VALUE_LENGTH];
	sprintf(section, "CWinOneFFT");
	GetPrivateProfileString(section, "RF", "0", value, VALUE_LENGTH, IniFilePath);
	rfButton->Button->value = atoi(value);
	GetPrivateProfileString(section, "RF_STEP", "0", value, VALUE_LENGTH, IniFilePath);
	rfStepButton->Button->value = atoi(value);
	GetPrivateProfileString(section, "AverageFilterValue", "2", value, VALUE_LENGTH, IniFilePath);
	averageFilterButton->Button->value = atoi(value);
	GetPrivateProfileString(section, "VScrollPos", "0", value, VALUE_LENGTH, IniFilePath);
	VScrollPos = atoi(value);
	GetPrivateProfileString(section, "VScrollZoom", "1.0", value, VALUE_LENGTH, IniFilePath);
	VScrollZoom = atof(value);
}

void CWinOneFFT::averageFilterButtonFunc(CScreenButton* button)
{
	WaitForSingleObject(clsWaveFFT.hMutexBuff, INFINITE);
	WaitForSingleObject(clsWinSpect.hMutexBuff, INFINITE);

	int memsize = (clsWaveFFT.HalfFFTSize + 2) * (FFT_DEEP + 1);
	memset(clsWaveFFT.FFTOrignalBuff, 0, memsize * sizeof(double));
	memset(clsWaveFFT.FFTOrignalLogBuff, 0, memsize * sizeof(double));
	memset(clsWaveFFT.FFTFilttedBuff, 0, memsize * sizeof(double));
	memset(clsWaveFFT.FFTFilttedLogBuff, 0, memsize * sizeof(double));

	clsWaveFFT.FFTOrignalBuffNum = 0;
	clsWaveFFT.FFTFilttedBuffNum = 0;
	clsWaveFFT.FFTDeep = button->Button->value;

	ReleaseMutex(clsWaveFFT.hMutexBuff);
	ReleaseMutex(clsWinSpect.hMutexBuff);
}

void CWinOneFFT::confirmP1RfGoButtonFunc(CScreenButton* button)
{
	POINT* P = &clsWinOneFFT.ScreenP1;
	if (P->x > WAVE_RECT_BORDER_LEFT && P->x < clsWinOneFFT.WinRect.right - WAVE_RECT_BORDER_RIGHT
		&& P->y > WAVE_RECT_BORDER_TOP && P->y < clsWinOneFFT.WinRect.bottom - WAVE_RECT_BORDER_BOTTON)	{

		double Hz = (double)(clsWinSpect.HScrollPos + P->x - WAVE_RECT_BORDER_LEFT) / clsWinSpect.HScrollZoom * clsData.AdcSampleRate / clsWaveFFT.FFTSize;
		if (button->Button->mouse_action == CScreenButton::Button_Mouse_Left) {
			clsWinOneFFT.rfButton->RefreshMouseNumButton(clsWinOneFFT.rfButton->Button->value + Hz);
		}
		else if (button->Button->mouse_action == CScreenButton::Button_Mouse_Right) {
			clsWinOneFFT.rfButton->RefreshMouseNumButton(clsWinOneFFT.rfButton->Button->value - Hz);
		}
		clsAnalyze.set_SDR_rfHz(clsWinOneFFT.rfButton->Button->value);
	}
}

void CWinOneFFT::P2SubP1(void)
{
	int n = 0, i = 0;
	int X;
	double Y, Ylog, YF, YFlog, YB, YBlog, YFB, YFBlog;
	char sX[50], sHZ[50], sY[50], sYlog[50], sYF[50], sYFlog[50], sYB[50], sYBlog[50], sYFB[50], sYFBlog[50];
	double P1Hz;
	double P2Hz;
	POINT *P;
	*strPP = '\0';
	bool P1Eanble = false;
	if (P1_Use == true) {
		P = &ScreenP1;
		X = (clsWinSpect.HScrollPos + P->x - WAVE_RECT_BORDER_LEFT) / clsWinSpect.HScrollZoom;
		if (X < clsWaveFFT.HalfFFTSize) {
			Y = clsWinSpect.OrignalFFTBuff[X];
			Ylog = clsWinSpect.OrignalFFTBuffLog[X];
			YF = clsWinSpect.FilttedFFTBuff[X];
			YFlog = clsWinSpect.FilttedFFTBuffLog[X];
			YB = clsWinSpect.BrieflyOrigFFTBuff[X];
			YBlog = clsWinSpect.BrieflyOrigFFTBuffLog[X];
			YFB = clsWinSpect.BrieflyFiltFFTBuff[X];
			YFBlog = clsWinSpect.BrieflyFiltFFTBuffLog[X];
			n += sprintf(strPP + n, "P1 X=%s, Hz=%s, Y=%s, Ylog=%sdb, YF=%s, YFlog=%sdb, YB=%s, YBlog=%sdb, YFB=%s, YFBlog=%sdb\r\n",
				fomatKINT64(X, sX),
				formatKDouble(P1Hz = (double)X * clsData.AdcSampleRate / clsWaveFFT.FFTSize, 0.001, "", sHZ),
				fomatKINT64(Y, sY),
				formatKDouble(Ylog * 20, 0.001, "", sYlog),
				formatKDouble(YF, 0.001, "", sYF),
				formatKDouble(YFlog * 20, 0.001, "", sYFlog),
				formatKDouble(YB, 0.001, "", sYB),
				formatKDouble(YBlog * 20, 0.001, "", sYBlog),
				formatKDouble(YFB, 0.001, "", sYFB),
				formatKDouble(YFBlog * 20, 0.001, "", sYFBlog)
			);
			P1Eanble = true;
		}
	}
	bool P2Eanble = false;
	if (P2_Use == true) {
		P = &ScreenP2;
		X = (clsWinSpect.HScrollPos + P->x - WAVE_RECT_BORDER_LEFT) / clsWinSpect.HScrollZoom;
		if (X < clsWaveFFT.HalfFFTSize) {
			Y = clsWinSpect.OrignalFFTBuff[X];
			Ylog = clsWinSpect.OrignalFFTBuffLog[X];
			YF = clsWinSpect.FilttedFFTBuff[X];
			YFlog = clsWinSpect.FilttedFFTBuffLog[X];
			YB = clsWinSpect.BrieflyOrigFFTBuff[X];
			YBlog = clsWinSpect.BrieflyOrigFFTBuffLog[X];
			YFB = clsWinSpect.BrieflyFiltFFTBuff[X];
			YFBlog = clsWinSpect.BrieflyFiltFFTBuffLog[X];
			n += sprintf(strPP + n, "P2 X=%s, Hz=%s, Y=%s, Ylog=%sdb, YF=%s, YFlog=%sdb, YB=%s, YBlog=%sdb, YFB=%s, YFBlog=%sdb\r\n",
				fomatKINT64(X, sX),
				formatKDouble(P2Hz = (double)X * clsData.AdcSampleRate / clsWaveFFT.FFTSize, 0.001, "", sHZ),
				fomatKINT64(Y, sY),
				formatKDouble(Ylog * 20, 0.001, "", sYlog),
				formatKDouble(YF, 0.001, "", sYF),
				formatKDouble(YFlog * 20, 0.001, "", sYFlog),
				formatKDouble(YB, 0.001, "", sYB),
				formatKDouble(YBlog * 20, 0.001, "", sYBlog),
				formatKDouble(YFB, 0.001, "", sYFB),
				formatKDouble(YFBlog * 20, 0.001, "", sYFBlog)
			);
			P2Eanble = true;
		}
	}
	if (P1Eanble == true && P2Eanble == true) {
		n += sprintf(strPP + n, "P2 - P1: Hz=%s", formatKDouble(P2Hz - P1Hz, 0.001, "", sYlog));
	}
}
