#include "stdafx.h"
#include "windows.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "Debug.h"

#include "CDataFromSDR.h"

#include "CData.h"
#include "CScreenButton.h"
#include "CFFT.h"
#include "CFilter.h"
#include "CWinFFT.h"
#include "CWinSpectrum.h"
#include "CWinOneSpectrum.h"
#include "CWinOneFFT.h"
#include "CWinTools.h"
#include "CWinSDRSet.h"

#include "CAnalyze.h"

using namespace std;
using namespace WINS; 
using namespace WINS::SPECTRUM; 
using namespace METHOD;

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define DIVLONG		10
#define DIVSHORT	5

#define SCROLLBAR_WIDTH		16

#define TOOLSBAR_SET_FILTER_CENTER_FREQ	1
#define TOOLSBAR_SET_AM_FREQ_ADD	2
#define TOOLSBAR_SET_AM_FREQ_SUB	3
#define TOOLSBAR_SET_FM_FREQ_ADD	4
#define TOOLSBAR_SET_FM_FREQ_SUB	5
#define TOOLSBAR_TBSTYLE_DROPDOWN	6

#define FFT_ZOOM_MAX		16

#define FFT_COLOR					::COLOR_PEN::Pen_Green
#define FFT_COLOR_LOG				::COLOR_PEN::Pen_Yellow
#define FFT_COLOR_FILTTED			::COLOR_PEN::Pen_Red
#define FFT_COLOR_FILTTED_LOG		::COLOR_PEN::Pen_Blue

CWinSpectrum clsWinSpect;

CWinSpectrum::CWinSpectrum()
{
	RegisterWindowsClass();
	Init();
}

CWinSpectrum::~CWinSpectrum()
{

}

void CWinSpectrum::RegisterWindowsClass(void)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinSpectrum::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_SPECTRUM;
	wcex.lpszClassName = WIN_SPECTRUM_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinSpectrum::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(WIN_SPECTRUM_CLASS, "Spectrum windows", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 1400, 900, NULL, NULL, hInst, NULL);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

LRESULT CALLBACK CWinSpectrum::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinSpect.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinSpectrum::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rt;

	switch (message)
	{
	case WM_CREATE:
	{
		clsWinSpect.hWnd = hWnd;
		hMenu = GetMenu(hWnd);
		hMenuShow = GetSubMenu(hMenu, 1);

		MakeReBar();

		CheckMenuRadioItem(hMenuShow, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
		CheckMenuItem(hMenu, IDM_SPECTRUM_ZOOMED_SHOW,
			(clsWinOneSpectrum.bSpectrumBrieflyShow == true ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		CheckMenuItem(hMenu, IDM_SPECTRUM_FOLLOW, MF_BYCOMMAND | MF_CHECKED);
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_SHOW,
			(clsWinOneFFT.bFFTOrignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_LOG_SHOW,
			(clsWinOneFFT.bFFTOrignalLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_SHOW,
			(clsWinOneFFT.bFFTFilttedShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_LOG_SHOW,
			(clsWinOneFFT.bFFTFilttedLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_HOLD,
			(clsWinSpect.bFFTHold ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		//KillTimer(hWnd, 0);//DrawInfo.uTimerId);
		
		RestoreValue();
		
		hWndOneFFT = CreateWindow(WIN_FFT_ONE_CLASS, "FFT One window", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 200, 500, 200, hWnd, NULL, hInst, NULL);
		ShowWindow(hWndOneFFT, SW_SHOW);
		hWndOneSpectrum = CreateWindow(WIN_SPECTRUM_ONE_CLASS, "Spectrum One window", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 0, 500, 200, hWnd, NULL, hInst, NULL);
		ShowWindow(hWndOneSpectrum, SW_SHOW);

		clsWinSpect.FFTOrignal->hWnd = hWnd;
		clsWinSpect.FFTOrignal->Data = AdcDataI;
		clsWinSpect.FFTOrignal->Init(clsWinSpect.FFTOrignal->FFTInfo->FFTSize, clsWinSpect.FFTOrignal->FFTInfo->FFTStep, clsWinSpect.FFTOrignal->FFTInfo->AverageDeep);
		clsWinSpect.FFTOrignal->hPen = Pens[FFT_COLOR];
		clsWinSpect.FFTOrignal->hPenLog = Pens[FFT_COLOR_LOG];
		clsWinSpect.FFTOrignal->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFFT::FFT_Thread, clsWinSpect.FFTOrignal, 0, NULL);
		
		clsWinSpect.FFTFiltted->hWnd = hWnd;
		clsWinSpect.FFTFiltted->Data = AdcDataIFiltted;
		clsWinSpect.FFTFiltted->Init(clsWinSpect.FFTFiltted->FFTInfo->FFTSize, clsWinSpect.FFTFiltted->FFTInfo->FFTStep, clsWinSpect.FFTFiltted->FFTInfo->AverageDeep);
		clsWinSpect.FFTFiltted->hPen = Pens[FFT_COLOR_FILTTED];
		clsWinSpect.FFTFiltted->hPenLog = Pens[FFT_COLOR_FILTTED_LOG];
		clsWinSpect.FFTFiltted->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFFT::FFT_Thread, clsWinSpect.FFTFiltted, 0, NULL);

		uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
	}
	break;
	case WM_TIMER:
	{
		if (clsWinSpect.bFFTHold == false) {
			clsWinSpect.FFTOrignal->FFTNext = true;
			clsWinSpect.FFTFiltted->FFTNext = true;
		}
	}
	break;
	case WM_FFT:
	{
		clsWinOneFFT.BrieflyBuff((CFFT*)lParam);
		clsWinOneSpectrum.PaintSpectrum((CFFT*)lParam);
	}
	break;
	case WM_SIZE:
	{
		GetClientRect(hWnd, &WinRect);
		//WinOneSpectrumHeight = (WinHeight - SCROLLBAR_WIDTH) / 4;
		RECT rt;
		GetClientRect(hWndRebar, &rt);
		DbgMsg("rt.bottom: %d\r\n", rt.bottom);
		int FFThight = WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP + WAVE_RECT_BORDER_BOTTON;
		MoveWindow(hWndRebar, 0, 0, WinRect.right, rt.bottom, true);
		MoveWindow(hWndOneFFT, 0, rt.bottom, WinRect.right, FFThight, true);
		//MoveWindow(hWndOneFFT, 0, 0, WinWidth, WinHeight, true);
		MoveWindow(hWndOneSpectrum, 0, rt.bottom + FFThight, WinRect.right, WinRect.bottom - rt.bottom - FFThight, true);

		//MoveWindow(hWndSpectrumHScrollBar, 0, 4 * WinOneSpectrumHeight, WinWidthSpectrum, SCROLLBAR_WIDTH, true);
		//SetScrollRange(hWndSpectrumHScrollBar, SB_HORZ, 0, SPECTRUM_WIDTH - SPECTRUM_WIN_WIDTH / WinOneSpectrumHScrollZoom, TRUE);
		//MoveWindow(hWndSpectrumVScrollBar, WinWidthSpectrum, 0, SCROLLBAR_WIDTH, 4 * WinOneSpectrumHeight, true);

		//if (clsWaveFFT.HalfFFTSize - WinOneSpectrumHeight > 0)
			//SetScrollRange(hWndSpectrumVScrollBar, SB_VERT, 0, clsWaveFFT.HalfFFTSize - WinOneSpectrumHeight, TRUE);
		//滚动条初始化
		GetRealClientRect(&rt);
		HScrollRange = ((CFFT*)clsWinSpect.FFTOrignal)->FFTInfo->HalfFFTSize * HScrollZoom - (rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		if (HScrollRange < 0) HScrollRange = 0;
		SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
	}
	break;
	case WM_CHAR:
		DbgMsg("char:%c\r\n", wParam);
		switch (wParam)
		{
		case '1':
			PostMessage(hWnd, WM_COMMAND, IDM_SPECTRUM_HORZ_ZOOM_INCREASE, NULL);
			break;
		case '2':

			break;
		case '3':

			break;
		case '4':

			break;
		case '5':

			break;
		case '6':

			break;
		}
		//InvalidateRect(hWnd, NULL, TRUE);
		return 0L;
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
		FFTOrignal->FFTDoing = FALSE;
		FFTFiltted->FFTDoing = FALSE;

		SaveValue();

		if (clsWinOneFFT.hWnd) DestroyWindow(clsWinOneFFT.hWnd);
		if (clsWinOneSpectrum.hWnd) DestroyWindow(clsWinOneSpectrum.hWnd);

		DbgMsg("WinSpectrum WM_DESTROY\r\n");
		this->hWnd = NULL;
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CWinSpectrum::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
		//Setting COMMANDS-----------------------
	case IDM_SPECTRUM_PAUSE_BREAK:
		break;

	case IDM_FFT_SET:
		DialogBox(hInst, (LPCTSTR)IDD_DLG_FFT_SET, hWnd, (DLGPROC)DlgFFTSetProc);
		break;
	case IDM_SDR_SET:
		clsWinSDRSet.OpenWindow();
		break;

	case IDM_SPECTRUM_HORZ_ZOOM_INCREASE:
		break;
	case IDM_SPECTRUM_HORZ_ZOOM_DECREASE:
		break;
	case IDM_SPECTRUM_HORZ_ZOOM_HOME:
		break;
	case IDM_SPECTRUM_VERT_ZOOM_INCREASE:
		break;
	case IDM_SPECTRUM_VERT_ZOOM_DECREASE:
		break;
	case IDM_SPECTRUM_VERT_ZOOM_HOME:
		break;
	case IDM_SPECTRUM_FOLLOW:
		break;

	case IDM_FFT_HORZ_ZOOM_INCREASE:
		if (HScrollZoom < FFT_ZOOM_MAX) {
			HScrollZoom *= 2;
			HScrollRange = ((CFFT*)clsWinSpect.FFTOrignal)->FFTInfo->HalfFFTSize * HScrollZoom - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
			HScrollPos *= 2.0;
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
			clsWinOneFFT.P2SubP1();
			PostMessage(clsWinOneFFT.hWnd, WM_SIZE, 0, 0);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_DECREASE:
		if (HScrollZoom * ((CFFT*)clsWinSpect.FFTOrignal)->FFTInfo->HalfFFTSize > WinRect.right) {
			HScrollZoom /= 2.0;
			HScrollRange = ((CFFT*)clsWinSpect.FFTOrignal)->FFTInfo->HalfFFTSize * HScrollZoom - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			UP_TO_ZERO(HScrollRange);
			SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
			HScrollPos /= 2.0;
			HScrollPos = BOUND(HScrollPos, 0, HScrollRange);
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
			clsWinOneFFT.P2SubP1();
			PostMessage(clsWinOneFFT.hWnd, WM_SIZE, 0, 0);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_HOME:
		HScrollPos /= HScrollZoom;
		HScrollRange = ((CFFT*)clsWinSpect.FFTOrignal)->FFTInfo->HalfFFTSize - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
		SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
		HScrollZoom = 1.0;
		clsWinOneFFT.P2SubP1();
		PostMessage(clsWinOneFFT.hWnd, WM_SIZE, 0, 0);
		break;
	case IDM_FFT_VERT_ZOOM_INCREASE:
	case IDM_FFT_VERT_ZOOM_DECREASE:
	case IDM_FFT_VERT_ZOOM_HOME:
		PostMessage(clsWinOneFFT.hWnd, message, wParam, lParam);
		break;
	case IDM_FFT_ORIGNAL_SHOW:
		clsWinOneFFT.bFFTOrignalShow = !clsWinOneFFT.bFFTOrignalShow;
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_SHOW,
			(clsWinOneFFT.bFFTOrignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_ORIGNAL_LOG_SHOW:
		clsWinOneFFT.bFFTOrignalLogShow = !clsWinOneFFT.bFFTOrignalLogShow;
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_LOG_SHOW,
			(clsWinOneFFT.bFFTOrignalLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_FILTTED_SHOW:
		clsWinOneFFT.bFFTFilttedShow = !clsWinOneFFT.bFFTFilttedShow;
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_SHOW,
			(clsWinOneFFT.bFFTFilttedShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_FILTTED_LOG_SHOW:
		clsWinOneFFT.bFFTFilttedLogShow = !clsWinOneFFT.bFFTFilttedLogShow;
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_LOG_SHOW,
			(clsWinOneFFT.bFFTFilttedLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_HOLD:
		clsWinSpect.bFFTHold = !clsWinSpect.bFFTHold;
		CheckMenuItem(hMenu, IDM_FFT_HOLD,
			(clsWinSpect.bFFTHold ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;

	case IDM_SPECTRUM_ORIGNAL_SHOW:
	{
		clsWinOneSpectrum.whichSignel = 0;
		CheckMenuRadioItem(hMenuShow, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
	}
		break;
	case IDM_SPECTRUM_ORIGNAL_LOG_SHOW:
	{
		clsWinOneSpectrum.whichSignel = 1;
		CheckMenuRadioItem(hMenuShow, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
	}
		break;
	case IDM_SPECTRUM_FILTTED_SHOW:
	{
		clsWinOneSpectrum.whichSignel = 2;
		CheckMenuRadioItem(hMenuShow, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
	}
		break;
	case IDM_SPECTRUM_FILTTED_LOG_SHOW:
	{
		clsWinOneSpectrum.whichSignel = 3;
		CheckMenuRadioItem(hMenuShow, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
	}
		break;
	case IDM_SPECTRUM_ZOOMED_SHOW:
		clsWinOneSpectrum.bSpectrumBrieflyShow = !clsWinOneSpectrum.bSpectrumBrieflyShow;
		CheckMenuItem(hMenu, IDM_SPECTRUM_ZOOMED_SHOW,
			(clsWinOneSpectrum.bSpectrumBrieflyShow == true ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
		
	case TOOLSBAR_SET_FILTER_CENTER_FREQ:
		ToolsbarSetFilterCenterFreq();
		break;
	case TOOLSBAR_SET_AM_FREQ_ADD:
		ToolsbarSetAMFreqAdd();
		break;
	case TOOLSBAR_SET_AM_FREQ_SUB:
		ToolsbarSetAMFreqSub();
		break;
	case TOOLSBAR_SET_FM_FREQ_ADD:
		ToolsbarSetFMFreqAdd();
		break;
	case TOOLSBAR_SET_FM_FREQ_SUB:
		ToolsbarSetFMFreqSub();
		break;
	case TOOLSBAR_TBSTYLE_DROPDOWN:
		break;
	case IDM_EXIT:
		//DestroyWindow(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

VOID CWinSpectrum::Paint(void)
{
	HDC		hDC;
	PAINTSTRUCT ps;
	RECT	rt;

	hDC = BeginPaint(hWnd, &ps);

	SelectObject(hDC, CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));

	/*
	GetClientRect(hWnd, &rt);
	HDC hdcCache = CreateCompatibleDC(hDC);
	HBITMAP hbmpCache = CreateCompatibleBitmap(hDC, rt.left, rt.right);
	SelectObject(hdcCache, hbmpCache);
	FillRect(hdcCache, &rt, (HBRUSH)GetStockObject(BLACK_BRUSH));

	rt = ps.rcPaint;
	BitBlt(hDC,
		rt.left, rt.top,
		rt.right, rt.bottom,
		hdcCache,
		rt.left, rt.top,
		SRCCOPY);

	DbgMsg("%d,%d,%d,%d\r\n", rt.top, rt.left, rt.right, rt.bottom);
	*/


	DeleteObject(SelectObject(hDC, GetStockObject(SYSTEM_FONT)));
	DeleteObject(hDC);

	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK CWinSpectrum::DlgFFTSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		//DWORD dwPos = GetDlgItemInt(hDlg, IDC_EDITGOTO, 0, 0);
		//SetDlgItemInt(hDlg, IDC_EDIT_PLAY_STOP_POSITION,	clsSoundCard.dwPlayStopPosition, TRUE);
		SetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, ((CFFT*)clsWinSpect.FFTOrignal)->FFTInfo->FFTSize, false);
		SetDlgItemInt(hDlg, IDC_EDIT_FFT_STEP, ((CFFT*)clsWinSpect.FFTOrignal)->FFTInfo->FFTStep, false);
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if (LOWORD(wParam) == IDOK)
			{
				UINT fftsize = GetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, NULL, false);
				UINT fftstep = GetDlgItemInt(hDlg, IDC_EDIT_FFT_STEP, NULL, false);
				int bit;
				for (bit = 0; fftsize > (1 << bit); bit++);
				fftsize = 1 << bit;
				for (bit = 0; fftstep > (1 << bit); bit++);
				fftstep = 1 << bit;

				int halffftsize = fftsize / 2;

				clsWinSpect.HScrollPos = (clsWinSpect.HScrollPos * (halffftsize / FFTInfo_Signal.HalfFFTSize));
				clsWinSpect.HScrollRange = halffftsize * clsWinSpect.HScrollZoom - (clsWinSpect.WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
				SetScrollRange(clsWinOneFFT.hWnd, SB_HORZ, 0, clsWinSpect.HScrollRange, false);
				SetScrollPos(clsWinOneFFT.hWnd, SB_HORZ, clsWinSpect.HScrollPos, true);

				clsWinSpect.FFTOrignal->Init(fftsize, fftstep, FFTInfo_Signal.AverageDeep);
				clsWinSpect.FFTFiltted->Init(
					fftsize / (AdcDataI->SampleRate / AdcDataIFiltted->SampleRate), 
					fftstep / (AdcDataI->SampleRate / AdcDataIFiltted->SampleRate), 
					FFTInfo_Filtted.AverageDeep
				);
			}
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

void CWinSpectrum::Init(void)
{

}

VOID CWinSpectrum::GetRealClientRect(PRECT lprc)
{
	DWORD dwStyle;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	GetClientRect(hWnd, lprc);
	if (dwStyle & WS_HSCROLL)
		lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (dwStyle & WS_VSCROLL)
		lprc->right += GetSystemMetrics(SM_CXVSCROLL);
}

void CWinSpectrum::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
{
	INT     iMax, iMin, iPos;
	int		dn = 0, tbdn = 0;
	RECT    rc;

	switch (message)
	{
	case WM_KEYDOWN:
		//DbgMsg("winSpectrum KeyAndScroll WM_KEYDOWN\r\n");
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
			HScrollPos = BOUND(HScrollPos + dn, 0, HScrollRange);
		}
		if (tbdn != 0)
		{
			HScrollPos = BOUND(tbdn, 0, HScrollRange);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
			//DbgMsg("clsWinSpect.HScrollPos: %d, clsWinSpect.HScrollRange: %d.\r\n", clsWinSpect.HScrollPos, clsWinSpect.HScrollRange);
		}
		break;
	}
}

void CWinSpectrum::SaveValue(void)
{
#define VALUE_LENGTH	100
	char section[VALUE_LENGTH];
	sprintf(section, "CWinSpectrum");
	char value[VALUE_LENGTH];
	WritePrivateProfileString(section, "HScrollPos", std::to_string(HScrollPos).c_str(), IniFilePath);
	WritePrivateProfileString(section, "VScrollPos", std::to_string(VScrollPos).c_str(), IniFilePath);
	sprintf(value, "%.20f", HScrollZoom);
	WritePrivateProfileString(section, "HScrollZoom", value, IniFilePath);
	sprintf(value, "%.20f", VScrollZoom);
	WritePrivateProfileString(section, "VScrollZoom", value, IniFilePath);
	sprintf(value, "%d", FFTInfo_Signal.FFTSize);
	WritePrivateProfileString(section, "FFTSize", value, IniFilePath);
	sprintf(value, "%d", FFTInfo_Signal.FFTStep);
	WritePrivateProfileString(section, "FFTStep", value, IniFilePath);
	sprintf(value, "%d", FFTInfo_Signal.AverageDeep);
	WritePrivateProfileString(section, "FFTDeep", value, IniFilePath);

	DbgMsg("CWinSpectrum::SaveValue\r\n");
}

void CWinSpectrum::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	char section[VALUE_LENGTH];
	sprintf(section, "CWinSpectrum");
	GetPrivateProfileString(section, "HScrollPos", "0", value, VALUE_LENGTH, IniFilePath);
	HScrollPos = atoi(value);
	GetPrivateProfileString(section, "VScrollPos", "0", value, VALUE_LENGTH, IniFilePath);
	VScrollPos = atoi(value);
	GetPrivateProfileString(section, "HScrollZoom", "1.0", value, VALUE_LENGTH, IniFilePath);
	HScrollZoom = atof(value);
	GetPrivateProfileString(section, "VScrollZoom", "1.0", value, VALUE_LENGTH, IniFilePath);
	VScrollZoom = atof(value);

	//GetPrivateProfileString(section, "FFTSize", "65536", value, VALUE_LENGTH, IniFilePath);
	//FFTInfo_Signal.FFTSize = atof(value);
	//FFTInfo_Signal.HalfFFTSize = FFTInfo_Signal.FFTSize / 2;
	//GetPrivateProfileString(section, "FFTStep", "65536", value, VALUE_LENGTH, IniFilePath);
	//FFTInfo_Signal.FFTStep = atof(value);
	//GetPrivateProfileString(section, "FFTDeep", "16", value, VALUE_LENGTH, IniFilePath);
	//FFTInfo_Signal.AverageDeep = atof(value);

	//FFTInfo_Filtted.FFTSize = FFTInfo_Signal.FFTSize >> clsMainFilter.rootFilterInfo.decimationFactorBit;
	//FFTInfo_Filtted.HalfFFTSize = FFTInfo_Filtted.FFTSize / 2;
	//FFTInfo_Filtted.FFTStep = FFTInfo_Signal.FFTStep >> clsMainFilter.rootFilterInfo.decimationFactorBit;
	//FFTInfo_Filtted.AverageDeep = FFTInfo_Signal.AverageDeep;

	DbgMsg("CWinSpectrum::RestoreValue\r\n");
}

HWND CWinSpectrum::MakeReBar(void)
{
	hWndRebar = clsWinTools.CreateRebar(hWnd);
	static TBBUTTON tbb[13] = {
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"FC" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, 
		{ MAKELONG(1, 0), TOOLSBAR_SET_AM_FREQ_ADD, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"AM Go +" },
		{ MAKELONG(1, 0), TOOLSBAR_SET_AM_FREQ_SUB, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"AM Go -" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, 
		{ MAKELONG(2, 0), TOOLSBAR_SET_FM_FREQ_ADD, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"FM Go +" },
		{ MAKELONG(2, 0), TOOLSBAR_SET_FM_FREQ_SUB, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"FM Go -" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL },
		{ MAKELONG(2, 0), IDM_FFT_SET, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"FFT设置..." },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, 
		{ MAKELONG(2, 0), IDM_SDR_SET, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"SDR设置..." },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, 
		{ MAKELONG(4, 0), TOOLSBAR_TBSTYLE_DROPDOWN, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"下拉" }
	};
	CWinTools::TOOL_TIPS tips[13] = {
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ 0, NULL }, 
		{ TOOLSBAR_SET_AM_FREQ_ADD,"P1 点频率 + 移动到AM滤波器中心频率." },
		{ TOOLSBAR_SET_AM_FREQ_SUB,"P1 点频率 - 移动到AM滤波器中心频率." },
		{ 0, NULL }, 
		{ TOOLSBAR_SET_FM_FREQ_ADD, "P1 点频率 + 移动到FM滤波器中心频率." },
		{ TOOLSBAR_SET_FM_FREQ_SUB, "P1 点频率 - 移动到FM滤波器中心频率." },
		{ 0, NULL }, 
		{ IDM_FFT_SET, "FFT 设置." },
		{ 0, NULL }, 
		{ IDM_SDR_SET, "SDR 设备设置." },
		{ 0, NULL }, 
		{ 0, "TBSTYLE_DROPDOWN" }
	};

	hWndFreqToolbar = clsWinTools.CreateToolbar(hWnd, tbb, 13, tips, 13);
	// Add images
	TBADDBITMAP tbAddBmp = { 0 };
	tbAddBmp.hInst = HINST_COMMCTRL;
	tbAddBmp.nID = IDB_STD_SMALL_COLOR;
	SendMessage(hWndFreqToolbar, TB_ADDBITMAP, 0, (WPARAM)&tbAddBmp);

	clsWinTools.CreateRebarBand(hWndRebar, "频率", 1, 500, 0, hWndFreqToolbar);

	static TBBUTTON tbbSDR[21] = {
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"采样频率" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"SDR滤波器" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"采样频率" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"SDR滤波器" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"采样频率" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"SDR滤波器" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"采样频率" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"SDR滤波器" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"采样频率" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"SDR滤波器" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"采样频率" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"SDR滤波器" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"采样频率" },
		{ MAKELONG(3, 0), TOOLSBAR_SET_FILTER_CENTER_FREQ, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"SDR滤波器" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, // Separator
		{ MAKELONG(1, 0), TOOLSBAR_SET_AM_FREQ_ADD, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"自动增益" },
		{ MAKELONG(1, 0), TOOLSBAR_SET_AM_FREQ_SUB, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"增益" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, // Separator
		{ MAKELONG(2, 0), TOOLSBAR_SET_FM_FREQ_ADD, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"FM Go +" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, // Separator
		{ MAKELONG(4, 0), TOOLSBAR_TBSTYLE_DROPDOWN, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"下拉" }
	};
	CWinTools::TOOL_TIPS tipsSDR[21] = {
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ TOOLSBAR_SET_FILTER_CENTER_FREQ,"P1点频率指定为滤波器中心频率." },
		{ 0, NULL }, // Separator
		{ TOOLSBAR_SET_AM_FREQ_ADD,"P1 点频率 + 移动到AM滤波器中心频率." },
		{ TOOLSBAR_SET_AM_FREQ_SUB,"P1 点频率 - 移动到AM滤波器中心频率." },
		{ 0, NULL }, // Separator
		{ TOOLSBAR_SET_FM_FREQ_ADD, "P1 点频率 + 移动到FM滤波器中心频率." },
		{ 0, NULL }, // Separator
		{ 0, "TBSTYLE_DROPDOWN" }
	};

	hWndSDRToolbar = clsWinTools.CreateToolbar(hWnd, tbbSDR, 21, tipsSDR, 21);
	// Add images
	//TBADDBITMAP tbAddBmp = { 0 };
	//tbAddBmp.hInst = HINST_COMMCTRL;
	//tbAddBmp.nID = IDB_STD_SMALL_COLOR;
	SendMessage(hWndSDRToolbar, TB_ADDBITMAP, 0, (WPARAM)&tbAddBmp);

	clsWinTools.CreateRebarBand(hWndRebar, "SDR", 2, 0, 0, hWndSDRToolbar);
	return hWndRebar;
}

void CWinSpectrum::ToolsbarSetFilterCenterFreq(void)
{
	//if(clsWinOneFFT.P1_Use == false)return;
	//char str[2000];
	//int n = sprintf(str, "257, 0, 0; 2, %u, 3000", (UINT)((float)clsMainFilterI.TargetData->SampleRate / FFTInfo_Filtted.FFTSize * clsWinOneFFT.ScreenP1.x));
	//DbgMsg("Filter Desc: %s\r\n", str);
	////clsMainFilter.Cuda_Filter_N = CFilter::cuda_filter_2;
	//clsMainFilterI.setFilterCoreDesc(&clsMainFilterI.rootFilterInfo1, str);
	//clsMainFilterI.ParseCoreDesc();
}

void CWinSpectrum::ToolsbarSetAMFreqAdd(void)
{
	if (clsWinOneFFT.P1_Use == false)return;
	if(
		clsWinOneFFT.rfButton->RefreshMouseNumButton(
			clsGetDataSDR.chParams->tunerParams.rfFreq.rfHz 
			+ (UINT)((float)clsMainFilterI.TargetData->SampleRate / FFTInfo_Filtted.FFTSize * clsWinOneFFT.ScreenP1.x)
			+ clsMainFilterI.rootFilterInfo1.FreqCenter
		)
		)
		clsAnalyze.set_SDR_rfHz(clsWinOneFFT.rfButton->Button->value);
}

void CWinSpectrum::ToolsbarSetAMFreqSub(void)
{
	if (clsWinOneFFT.P1_Use == false)return;
	if (
		clsWinOneFFT.rfButton->RefreshMouseNumButton(
			clsGetDataSDR.chParams->tunerParams.rfFreq.rfHz
			- (UINT)((float)clsMainFilterI.TargetData->SampleRate / FFTInfo_Filtted.FFTSize * clsWinOneFFT.ScreenP1.x)
			- clsMainFilterI.rootFilterInfo1.FreqCenter
		)
		)
		clsAnalyze.set_SDR_rfHz(clsWinOneFFT.rfButton->Button->value);
}

void CWinSpectrum::ToolsbarSetFMFreqAdd(void)
{

}
void CWinSpectrum::ToolsbarSetFMFreqSub(void)
{

}

bool CWinSpectrum::DoNotify(UINT msg, WPARAM wParam, LPARAM lParam)
{
#define lpnm    ((LPNMHDR)lParam)
	switch (lpnm->code)
	{
	case TBN_DROPDOWN:
	{
#define lpnmTB  ((LPNMTOOLBAR)lParam)
		// Get the coordinates of the button.
		RECT rc = { 0 };
		SendMessage(lpnmTB->hdr.hwndFrom, TB_GETRECT, (WPARAM)lpnmTB->iItem, (LPARAM)&rc);
		// Convert to screen coordinates.
		MapWindowPoints(lpnmTB->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);
		// Get the menu.
		HMENU hMenuLoaded = LoadMenu(hInst, MAKEINTRESOURCE(lpnmTB->iItem == IDM_DEMODULATOR_AM ? IDC_MENU_POPUP_AM : IDC_MENUMAIN));
		// Get the submenu for the first menu item.
		HMENU hPopupMenu = GetSubMenu(hMenuLoaded, 0);
		// Set up the pop-up menu.
		// In case the toolbar is too close to the bottom of the screen,
		// set rcExclude equal to the button rectangle and the menu will appear above
		// the button, and not below it.
		TPMPARAMS tpm = { 0 };
		tpm.cbSize = sizeof(TPMPARAMS);
		tpm.rcExclude = rc;
		// Show the menu and wait for input. Using Toolbar Controls Windows common controls demo(CppWindowsCommonControls)
		// If the user selects an item, its WM_COMMAND is sent.
		TrackPopupMenuEx(hPopupMenu,
			TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
			rc.left, rc.bottom, hWnd, &tpm);
		DestroyMenu(hMenuLoaded);

		DbgMsg("TBN_DROPDOWN %d, %d\r\n", lpnmTB->iItem, IDM_DEMODULATOR_AM);
		return FALSE;
	}
	}
	return FALSE;
}
