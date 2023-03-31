#include "stdafx.h"
#include "windows.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "myDebug.h"
#include "CWaveData.h"
#include "CWaveFFT.h"
#include "CWinFFT.h"
#include "CWinSpectrum.h"
#include "CWinOneSpectrum.h"
#include "CWinOneFFT.h"

using namespace std;

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define DIVLONG		10
#define DIVSHORT	5

#define SCROLLBAR_WIDTH		16

CWinSpectrum clsWinSpect;

CWinSpectrum::CWinSpectrum()
{
	OPENCONSOLE;


	hMutexBuff = CreateMutex(NULL, false, "CWinSpectrumhMutexBuff");

	RegisterWindowsClass();

	InitBuff();
}

CWinSpectrum::~CWinSpectrum()
{
	if(hWnd) DestroyWindow(hWnd);
	CLOSECONSOLE;
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
	wcex.lpszClassName = SPECTRUM_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinSpectrum::OpenWindow(void)
{
	if (hWnd) {
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
	}
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
		
		OPENCONSOLE;

		clsWinSpect.hWnd = hWnd;

		hMenu = GetMenu(hWnd);

		{
			HMENU m = GetSubMenu(hMenu, 2);
			CheckMenuRadioItem(m, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
		}

		CheckMenuItem(hMenu, IDM_SPECTRUM_FOLLOW, MF_BYCOMMAND | MF_CHECKED);

		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_SHOW,
			(clsWinFFT.bFFTOrignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_LOG_SHOW,
			(clsWinFFT.bFFTOrignalLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_SHOW,
			(clsWinFFT.bFFTFilttedShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_LOG_SHOW,
			(clsWinFFT.bFFTFilttedLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		CheckMenuItem(hMenu, IDM_FFT_HOLD,
			(clsWinFFT.bFFTHold ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		//KillTimer(hWnd, 0);//DrawInfo.uTimerId);


		hWndOneFFT = CreateWindow(FFT_ONE_WIN_CLASS, "FFT One window", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 200, 500, 200, hWnd, NULL, hInst, NULL);
		ShowWindow(hWndOneFFT, SW_SHOW);

		hWndOneSpectrum = CreateWindow(SPECTRUM_ONE_WIN_CLASS, "Spectrum One window", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 0, 500, 200, hWnd, NULL, hInst, NULL);
		ShowWindow(hWndOneSpectrum, SW_SHOW);
		//UpdateWindow(hWndOneSpectrum);

		/*
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nPage = 128;
			si.nPos = 0;

		hWndSpectrumHScrollBar = CreateWindowEx(0L, "SCROLLBAR", NULL, WS_CHILD | WS_VISIBLE | SBS_HORZ,
				0, 800, SPECTRUM_WIN_WIDTH, 16, hWnd, NULL, hInst, NULL);
		si.nMax = SPECTRUM_WIDTH - SPECTRUM_WIN_WIDTH;
		SetScrollInfo(hWndSpectrumHScrollBar, SB_CTL, &si, TRUE);

		hWndSpectrumVScrollBar = CreateWindowEx(0L, "SCROLLBAR", NULL, WS_CHILD | WS_VISIBLE | SBS_VERT,
			500, 0, 16, 800, hWnd, NULL, hInst, NULL);
		si.nMax = clsWaveFFT.HalfFFTSize - WinOneSpectrumHeight;
		SetScrollInfo(hWndSpectrumVScrollBar, SB_CTL, &si, TRUE);

		}
		*/
		break;

	case WM_TIMER:
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		break;

	case WM_SIZE:
	{
		GetClientRect(hWnd, &rt);
		WinWidth = rt.right;
		WinHeight = rt.bottom;
		//WinOneSpectrumHeight = (WinHeight - SCROLLBAR_WIDTH) / 4;
		int FFThight = WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP + WAVE_RECT_BORDER_BOTTON;
		MoveWindow(hWndOneFFT, 0, 0, WinWidth, FFThight, true);
		//MoveWindow(hWndOneFFT, 0, 0, WinWidth, WinHeight, true);
		MoveWindow(hWndOneSpectrum, 0, FFThight, WinWidth, WinHeight - FFThight, true);

		//MoveWindow(hWndSpectrumHScrollBar, 0, 4 * WinOneSpectrumHeight, WinWidthSpectrum, SCROLLBAR_WIDTH, true);
		//SetScrollRange(hWndSpectrumHScrollBar, SB_HORZ, 0, SPECTRUM_WIDTH - SPECTRUM_WIN_WIDTH / WinOneSpectrumHScrollZoom, TRUE);
		//MoveWindow(hWndSpectrumVScrollBar, WinWidthSpectrum, 0, SCROLLBAR_WIDTH, 4 * WinOneSpectrumHeight, true);

		//if (clsWaveFFT.HalfFFTSize - WinOneSpectrumHeight > 0)
			//SetScrollRange(hWndSpectrumVScrollBar, SB_VERT, 0, clsWaveFFT.HalfFFTSize - WinOneSpectrumHeight, TRUE);

				//滚动条初始化
		RECT rt;
		GetRealClientRect(hWnd, &rt);
		clsWinSpect.HScrollWidth = clsWaveFFT.HalfFFTSize - (rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		if (clsWinSpect.HScrollWidth < 0) clsWinSpect.HScrollWidth = 0;
		SetScrollRange(hWnd, SB_HORZ, 0, clsWinSpect.HScrollWidth, TRUE);

	}
		break;

	case WM_CHAR:
		printf("char:%c\r\n", wParam);
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
		if (clsWinOneFFT.hWnd) DestroyWindow(clsWinOneFFT.hWnd);
		if (clsWinOneSpectrum.hWnd) DestroyWindow(clsWinOneSpectrum.hWnd);
		//PostQuitMessage(0);
		DbgMsg("WinSpectrum WM_DESTROY\r\n");
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

BOOL CWinSpectrum::OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

	case IDM_SPECTRUM_HORZ_ZOOM_INCREASE:
		if (WinOneSpectrumHScrollZoom < SPECTRUM_ZOOM_MAX) {
			WinOneSpectrumHScrollZoom *= 2;
			int w = SPECTRUM_WIDTH - SPECTRUM_WIN_WIDTH / WinOneSpectrumHScrollZoom;
			if (w < 0) w = 0;
			//SetScrollRange(hWndSpectrumHScrollBar, SB_HORZ, 0, w, TRUE);
		}
		break;
	case IDM_SPECTRUM_HORZ_ZOOM_DECREASE:
		if (WinOneSpectrumHScrollZoom * SPECTRUM_WIDTH > SPECTRUM_WIN_WIDTH) {
			WinOneSpectrumHScrollZoom /= 2;
			int w = SPECTRUM_WIDTH - SPECTRUM_WIN_WIDTH / WinOneSpectrumHScrollZoom;
			if (w < 0) w = 0;
			//SetScrollRange(hWndSpectrumHScrollBar, SB_HORZ, 0, w, TRUE);
		}
		break;
	case IDM_SPECTRUM_HORZ_ZOOM_HOME:
	{
		WinOneSpectrumHScrollZoom = 1.0;
		int w = SPECTRUM_WIDTH - SPECTRUM_WIN_WIDTH / WinOneSpectrumHScrollZoom;
		if (w < 0) w = 0;
		//SetScrollRange(hWndSpectrumHScrollBar, SB_HORZ, 0, w, TRUE);
	}
		break;
	case IDM_SPECTRUM_VERT_ZOOM_INCREASE:
		if (WinOneSpectrumVScrollZoom < SPECTRUM_ZOOM_MAX) {
			WinOneSpectrumVScrollZoom *= 2;
			int w = clsWaveFFT.HalfFFTSize - WinOneSpectrumHeight / WinOneSpectrumVScrollZoom;
			if (w < 0)w = 0;
				//SetScrollRange(hWndSpectrumVScrollBar, SB_VERT, 0, w, TRUE);
		}
		break;
	case IDM_SPECTRUM_VERT_ZOOM_DECREASE:
		if (WinOneSpectrumVScrollZoom * clsWaveFFT.HalfFFTSize > WinOneSpectrumHeight) {
			WinOneSpectrumVScrollZoom /= 2;
			int w = clsWaveFFT.HalfFFTSize - WinOneSpectrumHeight / WinOneSpectrumVScrollZoom;
			if (w < 0)w = 0;
			//SetScrollRange(hWndSpectrumVScrollBar, SB_VERT, 0, w, TRUE);
		}
		break;
	case IDM_SPECTRUM_VERT_ZOOM_HOME:
	{
		WinOneSpectrumVScrollZoom = 1.0;
		int w = clsWaveFFT.HalfFFTSize - WinOneSpectrumHeight / WinOneSpectrumVScrollZoom;
		if (w < 0)w = 0;
		//SetScrollRange(hWndSpectrumVScrollBar, SB_VERT, 0, w, TRUE);
	}
		break;
	case IDM_SPECTRUM_FOLLOW:
		SpectrumAutoFollow = !SpectrumAutoFollow;
		CheckMenuItem(hMenu, IDM_SPECTRUM_FOLLOW, MF_BYCOMMAND | (SpectrumAutoFollow == true ? MF_CHECKED : MF_UNCHECKED));
		break;

	case IDM_FFT_HORZ_ZOOM_INCREASE:
		if (HScrollZoom < FFT_ZOOM_MAX) {
			HScrollZoom *= 2;
			PostMessage(clsWinOneFFT.hWnd, WM_SIZE, 0, 0);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_DECREASE:

		if (HScrollZoom * clsWaveFFT.FFTSize > clsWinOneFFT.WinWidth) {
			HScrollZoom /= 2.0;
			PostMessage(clsWinOneFFT.hWnd, WM_SIZE, 0, 0);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_HOME:
		HScrollZoom = 1.0;
		PostMessage(clsWinOneFFT.hWnd, WM_SIZE, 0, 0);
		break;
	case IDM_FFT_VERT_ZOOM_INCREASE:
		if (VScrollZoom < FFT_ZOOM_MAX)
			VScrollZoom *= 2.0;
		break;
	case IDM_FFT_VERT_ZOOM_DECREASE:
		if (VScrollZoom < FFT_ZOOM_MAX)
			VScrollZoom /= 2.0;
		break;
	case IDM_FFT_VERT_ZOOM_HOME:
		VScrollZoom = 1.0;
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
		clsWinOneFFT.bFFTHold = !clsWinOneFFT.bFFTHold;
		CheckMenuItem(hMenu, IDM_FFT_HOLD,
			(clsWinOneFFT.bFFTHold ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;

	case IDM_SPECTRUM_ORIGNAL_SHOW:
	{
		HMENU m = GetSubMenu(hMenu, 2);
		clsWinOneSpectrum.whichSignel = 0;
		CheckMenuRadioItem(m, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
	}
		break;
	case IDM_SPECTRUM_ORIGNAL_LOG_SHOW:
	{
		HMENU m = GetSubMenu(hMenu, 2);
		clsWinOneSpectrum.whichSignel = 1;
		CheckMenuRadioItem(m, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
	}
		break;
	case IDM_SPECTRUM_FILTTED_SHOW:
	{
		HMENU m = GetSubMenu(hMenu, 2);
		clsWinOneSpectrum.whichSignel = 2;
		CheckMenuRadioItem(m, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
	}
		break;
	case IDM_SPECTRUM_FILTTED_LOG_SHOW:
	{
		HMENU m = GetSubMenu(hMenu, 2);
		clsWinOneSpectrum.whichSignel = 3;
		CheckMenuRadioItem(m, 0, 3, clsWinOneSpectrum.whichSignel, MF_BYPOSITION);
	}
		break;
	case IDM_SPECTRUM_ZOOMED_SHOW:
		clsWinOneSpectrum.bSpectrumZoomedShow = !clsWinOneSpectrum.bSpectrumZoomedShow;
		CheckMenuItem(hMenu, IDM_SPECTRUM_ZOOMED_SHOW,
			(clsWinOneSpectrum.bSpectrumZoomedShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
		
	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

VOID CWinSpectrum::Paint(HWND hWnd)
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

	printf("%d,%d,%d,%d\r\n", rt.top, rt.left, rt.right, rt.bottom);
	*/


	DeleteObject(SelectObject(hDC, GetStockObject(SYSTEM_FONT)));
	DeleteObject(hDC);

	EndPaint(hWnd, &ps);
}

void CWinSpectrum::PaintFFT(WHICHSIGNAL WhichSignal)
{
	/*
	if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
		clsWinOneSpectrum.PaintFFTToSpectrum(&clsWinOneSpectrum.OrignalFFTData,		clsWinFFT.OrignalFFTBuff);
		clsWinOneSpectrum.PaintFFTToSpectrum(&clsWinOneSpectrum.OrignalLogFFTData,	clsWinFFT.OrignalFFTBuffLog);
	}
	else {
		clsWinOneSpectrum.PaintFFTToSpectrum(&clsWinOneSpectrum.FilttedFFTData,		clsWinFFT.FilttedFFTBuff);
		clsWinOneSpectrum.PaintFFTToSpectrum(&clsWinOneSpectrum.FilttedLogFFTData,	clsWinFFT.FilttedFFTBuffLog);
	}
	*/

}

LRESULT CALLBACK CWinSpectrum::DlgFFTSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		//DWORD dwPos = GetDlgItemInt(hDlg, IDC_EDITGOTO, 0, 0);
		//SetDlgItemInt(hDlg, IDC_EDIT_PLAY_STOP_POSITION,	clsSoundCard.dwPlayStopPosition, TRUE);
		SetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, clsWaveFFT.FFTSize, false);
		SetDlgItemInt(hDlg, IDC_EDIT_FFT_STEP, clsWaveFFT.FFTStep, false);
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if (LOWORD(wParam) == IDOK)
			{
				UINT fftsize = GetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, NULL, false);
				UINT fftstep = GetDlgItemInt(hDlg, IDC_EDIT_FFT_STEP, NULL, false);
				//clsWaveFFT.FFTSize = GetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, NULL, false);
				//clsWaveFFT.FFTStep = GetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, NULL, false);
				clsWaveFFT.InitAllBuff(fftsize, fftstep);
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
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nPage = 128;
	si.nPos = 0;

	si.nMax = SPECTRUM_WIDTH - SPECTRUM_WIN_WIDTH;
	//SetScrollInfo(hWndSpectrumHScrollBar, SB_CTL, &si, TRUE);

	si.nMax = clsWaveFFT.HalfFFTSize - WinOneSpectrumHeight;
	//SetScrollInfo(hWndSpectrumVScrollBar, SB_CTL, &si, TRUE);
}

VOID CWinSpectrum::GetRealClientRect(HWND hWnd, PRECT lprc)
{
	DWORD dwStyle;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	GetClientRect(hWnd, lprc);
	if (dwStyle & WS_HSCROLL)
		lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (dwStyle & WS_VSCROLL)
		lprc->right += GetSystemMetrics(SM_CXVSCROLL);
}

void CWinSpectrum::KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
			clsWinSpect.HScrollPos = BOUND(clsWinSpect.HScrollPos + dn, 0, clsWinSpect.HScrollWidth);
		}
		if (tbdn != 0)
		{
			clsWinSpect.HScrollPos = BOUND(tbdn, 0, clsWinSpect.HScrollWidth);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_HORZ, clsWinSpect.HScrollPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
			//printf("clsWinSpect.HScrollPos: %d, clsWinSpect.HScrollWidth: %d.\r\n", clsWinSpect.HScrollPos, clsWinSpect.HScrollWidth);
		}
		break;
	}
}

void CWinSpectrum::InitBuff(void)
{
	
	if (OrignalFFTBuff != NULL) delete[] OrignalFFTBuff;
	OrignalFFTBuff = new double[clsWaveFFT.HalfFFTSize + 2];

	if (OrignalFFTBuffLog != NULL) delete[] OrignalFFTBuffLog;
	OrignalFFTBuffLog = new double[clsWaveFFT.HalfFFTSize + 2];

	if (FilttedFFTBuff != NULL) delete[] FilttedFFTBuff;
	FilttedFFTBuff = new double[clsWaveFFT.HalfFFTSize + 2];

	if (FilttedFFTBuffLog != NULL) delete[] FilttedFFTBuffLog;
	FilttedFFTBuffLog = new double[clsWaveFFT.HalfFFTSize + 2];

	if (BrieflyOrigFFTBuff != NULL) delete[] BrieflyOrigFFTBuff;
	BrieflyOrigFFTBuff = new double[clsWaveFFT.HalfFFTSize + 2];

	if (BrieflyOrigFFTBuffLog != NULL) delete[] BrieflyOrigFFTBuffLog;
	BrieflyOrigFFTBuffLog = new double[clsWaveFFT.HalfFFTSize + 2];

	if (BrieflyFiltFFTBuff != NULL) delete[] BrieflyFiltFFTBuff;
	BrieflyFiltFFTBuff = new double[clsWaveFFT.HalfFFTSize + 2];

	if (BrieflyFiltFFTBuffLog != NULL) delete[] BrieflyFiltFFTBuffLog;
	BrieflyFiltFFTBuffLog = new double[clsWaveFFT.HalfFFTSize + 2];

	Buffs[0] = OrignalFFTBuff;
	Buffs[1] = OrignalFFTBuffLog;
	Buffs[2] = FilttedFFTBuff;
	Buffs[3] = FilttedFFTBuffLog;

	BrieflyBuffs[0] = BrieflyOrigFFTBuff;
	BrieflyBuffs[1] = BrieflyOrigFFTBuffLog;
	BrieflyBuffs[2] = BrieflyFiltFFTBuff;
	BrieflyBuffs[3] = BrieflyFiltFFTBuffLog;

}
