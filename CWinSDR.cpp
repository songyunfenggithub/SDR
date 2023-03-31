
#include "stdafx.h"
#include "windows.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>

#include <map>

#include "public.h"
#include "CWaveData.h"
#include "CWaveFFT.h"
#include "CWinFFT.h"
#include "CWinSDR.h"
#include "CWinOneSpectrum.h"

#include "CDataFromSDR.h"
#include "CSDR.h"

#include "SDRPlay_API.3.09/API/inc/sdrplay_api.h"

#pragma comment(lib,"SDRPlay_API.3.09/API/x64/sdrplay_api.lib")

using namespace std;

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define DIVLONG		10
#define DIVSHORT	5

#define TIMEOUT		500

#define SCROLLBAR_WIDTH		16

CWinSDR clsWinSDR;

CWinSDR::CWinSDR()
{
	OPENCONSOLE;
	RegisterWindowsClass();
}

CWinSDR::~CWinSDR()
{
	if (hWnd) DestroyWindow(hWnd);
	CLOSECONSOLE;
}

void CWinSDR::RegisterWindowsClass(void)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinSDR::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_SDR;
	wcex.lpszClassName = SDR_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinSDR::OpenWindow(void)
{
	if (hWnd) {
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
	}
}


void CWinSDR::ProcessKey(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT     iMax, iMin, iPos;
	int		dn = 0, tbdn = 0;
	RECT    rc;
	POINT	pt;
	UINT32 hz;

	switch (message)
	{
	case WM_LBUTTONDOWN:
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		hz = (uint32_t)(clsWaveData.AdcSampleRate * pt.y / clsWaveFFT.FFTSize);
		cout << pt.x << ":" << pt.y << ":" << hz << endl;
		break;

	case WM_KEYDOWN:
		/* Translate keyboard messages to scroll commands */
		printf("win spectrum WM_KEYDOWN\r\n");
		switch (wParam) {
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
		switch (wParam) {
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
		/* Calculate new vertical scroll position */
		GetScrollRange(hWnd, SB_VERT, &iMin, &iMax);
		iPos = GetScrollPos(hWnd, SB_VERT);
		GetClientRect(hWnd, &rc);

		switch (GET_WM_VSCROLL_CODE(wParam, lParam)) {
		case SB_LINEDOWN:
			dn = rc.bottom / 16 + 1;
			break;

		case SB_LINEUP:
			dn = -rc.bottom / 16 + 1;
			break;

		case SB_PAGEDOWN:
			dn = rc.bottom / 2 + 1;
			break;

		case SB_PAGEUP:
			dn = -rc.bottom / 2 + 1;
			break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			dn = GET_WM_VSCROLL_POS(wParam, lParam) - iPos;
			break;

		default:
			dn = 0;
			break;
		}
		/* Limit scrolling to current scroll range */
		if (dn = BOUND(iPos + dn, iMin, iMax) - iPos)
		{
			ScrollWindow(hWnd, 0, -dn, NULL, NULL);
			SetScrollPos(hWnd, SB_VERT, iPos + dn, TRUE);
		}
		break;

	case WM_HSCROLL:
		/* Calculate new horizontal scroll position */
		GetScrollRange(hWnd, SB_HORZ, &iMin, &iMax);
		iPos = GetScrollPos(hWnd, SB_HORZ);
		GetClientRect(hWnd, &rc);

		switch (GET_WM_HSCROLL_CODE(wParam, lParam)) {
		case SB_LINEDOWN:
			dn = rc.right / 16 + 1;
			break;

		case SB_LINEUP:
			dn = -rc.right / 16 + 1;
			break;

		case SB_PAGEDOWN:
			dn = rc.right / 2 + 1;
			break;

		case SB_PAGEUP:
			dn = -rc.right / 2 + 1;
			break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			tbdn = GET_WM_HSCROLL_POS(wParam, lParam);//-iPos;
			break;

		default:
			dn = 0;
			break;
		}
		if (dn != 0)
		{
			DrawInfo.dwHZoomedPos = BOUND((int)DrawInfo.dwHZoomedPos + dn,
				0, DrawInfo.dwHZoomedWidth);
			DrawInfo.wHSclPos = DrawInfo.dwHZoomedPos >> DrawInfo.iHFit;
		}
		if (tbdn != 0)
		{
			DrawInfo.wHSclPos = BOUND(tbdn, 0, DrawInfo.wHSclMax);
			DrawInfo.dwHZoomedPos = DrawInfo.wHSclPos << DrawInfo.iHFit;
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_HORZ, DrawInfo.wHSclPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	}
}

LRESULT CALLBACK CWinSDR::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinSDR.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinSDR::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rt;

	clsWinSDR.ProcessKey(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_CREATE:

		OPENCONSOLE;

		clsWinSDR.hWnd = hWnd;


		break;

	case WM_TIMER:
		//InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;

	case WM_SIZE:
	{
	}
	break;

	case WM_CHAR:
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
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

BOOL CWinSDR::OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
		//Setting COMMANDS-----------------------

	case IDM_SDR_SET:
		DialogBox(hInst, (LPCTSTR)IDD_DLG_SDR_SET, hWnd, (DLGPROC)CWinSDR::DlgSDRSetProc);
		break;

	case IDM_SPECTRUM_HORZ_ZOOM_INCREASE:
		if (WinOneSpectrumHScrollZoom < SDR_ZOOM_MAX) {
			WinOneSpectrumHScrollZoom *= 2;
			int w = SDR_WIDTH - SDR_WIN_WIDTH / WinOneSpectrumHScrollZoom;
			if (w < 0) w = 0;
			SetScrollRange(hWndSpectrumHScrollBar, SB_HORZ, 0, w, TRUE);
		}
		break;
	case IDM_SPECTRUM_HORZ_ZOOM_DECREASE:
		if (WinOneSpectrumHScrollZoom * SDR_WIDTH > SDR_WIN_WIDTH) {
			WinOneSpectrumHScrollZoom /= 2;
			int w = SDR_WIDTH - SDR_WIN_WIDTH / WinOneSpectrumHScrollZoom;
			if (w < 0) w = 0;
			SetScrollRange(hWndSpectrumHScrollBar, SB_HORZ, 0, w, TRUE);
		}
		break;
	case IDM_SPECTRUM_HORZ_ZOOM_HOME:
	{
		WinOneSpectrumHScrollZoom = 1.0;
		int w = SDR_WIDTH - SDR_WIN_WIDTH / WinOneSpectrumHScrollZoom;
		if (w < 0) w = 0;
		SetScrollRange(hWndSpectrumHScrollBar, SB_HORZ, 0, w, TRUE);
	}
	break;
	case IDM_SPECTRUM_VERT_ZOOM_INCREASE:
		if (WinOneSpectrumVScrollZoom < SDR_ZOOM_MAX) {
			WinOneSpectrumVScrollZoom *= 2;
			int w = clsWaveFFT.FFTSize / 2 - WinOneSpectrumHeight / WinOneSpectrumVScrollZoom;
			if (w < 0)w = 0;
			SetScrollRange(hWndSpectrumVScrollBar, SB_VERT, 0, w, TRUE);
		}
		break;
	case IDM_SPECTRUM_VERT_ZOOM_DECREASE:
		if (WinOneSpectrumVScrollZoom * clsWaveFFT.FFTSize / 2 > WinOneSpectrumHeight) {
			WinOneSpectrumVScrollZoom /= 2;
			int w = clsWaveFFT.FFTSize / 2 - WinOneSpectrumHeight / WinOneSpectrumVScrollZoom;
			if (w < 0)w = 0;
			SetScrollRange(hWndSpectrumVScrollBar, SB_VERT, 0, w, TRUE);
		}
		break;
	case IDM_SPECTRUM_VERT_ZOOM_HOME:
	{
		WinOneSpectrumVScrollZoom = 1.0;
		int w = clsWaveFFT.FFTSize / 2 - WinOneSpectrumHeight / WinOneSpectrumVScrollZoom;
		if (w < 0)w = 0;
		SetScrollRange(hWndSpectrumVScrollBar, SB_VERT, 0, w, TRUE);
	}
	break;
	case IDM_SPECTRUM_FOLLOW:
		SpectrumAutoFollow = !SpectrumAutoFollow;
		CheckMenuItem(hMenu, IDM_SPECTRUM_FOLLOW, MF_BYCOMMAND | (SpectrumAutoFollow == true ? MF_CHECKED : MF_UNCHECKED));
		break;

	case IDM_FFT_HORZ_ZOOM_INCREASE:
		if (clsWinFFT.HScrollZoom < FFT_ZOOM_MAX) {
			clsWinFFT.HScrollZoom *= 2;
			PostMessage(clsWinFFT.hWnd, WM_SIZE, 0, 0);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_DECREASE:
		if (clsWinFFT.HScrollZoom * clsWaveFFT.FFTSize > clsWinFFT.WinWidth) {
			clsWinFFT.HScrollZoom /= 2;
			PostMessage(clsWinFFT.hWnd, WM_SIZE, 0, 0);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_HOME:
		clsWinFFT.HScrollZoom = 1.0;
		PostMessage(clsWinFFT.hWnd, WM_SIZE, 0, 0);
		break;
	case IDM_FFT_VERT_ZOOM_INCREASE:
		//if (clsWinFFT.VScrollZoom < FFT_ZOOM_MAX)
		//	clsWinFFT.VScrollZoom *= 2;
		break;
	case IDM_FFT_VERT_ZOOM_DECREASE:
		//if (clsWinFFT.VScrollZoom < FFT_ZOOM_MAX)
		//	clsWinFFT.VScrollZoom *= 2;
		break;
	case IDM_FFT_VERT_ZOOM_HOME:
		clsWinFFT.VScrollZoom = 1.0;
		break;
	case IDM_FFT_ORIGNAL_SHOW:
		clsWinFFT.bFFTOrignalShow = !clsWinFFT.bFFTOrignalShow;
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_SHOW,
			(clsWinFFT.bFFTOrignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_ORIGNAL_LOG_SHOW:
		clsWinFFT.bFFTOrignalLogShow = !clsWinFFT.bFFTOrignalLogShow;
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_LOG_SHOW,
			(clsWinFFT.bFFTOrignalLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_FILTTED_SHOW:
		clsWinFFT.bFFTFilttedShow = !clsWinFFT.bFFTFilttedShow;
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_SHOW,
			(clsWinFFT.bFFTFilttedShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_FILTTED_LOG_SHOW:
		clsWinFFT.bFFTFilttedLogShow = !clsWinFFT.bFFTFilttedLogShow;
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_LOG_SHOW,
			(clsWinFFT.bFFTFilttedLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_HOLD:
		clsWinFFT.bFFTHold = !clsWinFFT.bFFTHold;
		CheckMenuItem(hMenu, IDM_FFT_HOLD,
			(clsWinFFT.bFFTHold ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;

	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

VOID CWinSDR::Paint(HWND hWnd)
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

void CWinSDR::PaintFFT(WHICHSIGNAL WhichSignal)
{

}


void CWinSDR::KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT     iMax, iMin, iPos;
	int		dn = 0, tbdn = 0;
	RECT    rc;
	SCROLLINFO si;

	switch (message)
	{
	case WM_KEYDOWN:
		/* Translate keyboard messages to scroll commands */
		switch (wParam)
		{
		case VK_UP:
			PostMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0L);
			break;
		case VK_DOWN:
			PostMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
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
		GetScrollRange(hWndSpectrumVScrollBar, SB_VERT, &iMin, &iMax);
		iPos = GetScrollPos(hWndSpectrumVScrollBar, SB_VERT);
		GetClientRect(hWnd, &rc);
		switch (GET_WM_VSCROLL_CODE(wParam, lParam))
		{
		case SB_LINEDOWN:
			dn = 16;
			break;
		case SB_LINEUP:
			dn = -16;
			break;
		case SB_PAGEDOWN:
			dn = 128;
			break;
		case SB_PAGEUP:
			dn = -128;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			tbdn = GET_WM_VSCROLL_POS(wParam, lParam);
			break;
		default:
			dn = 0;
			break;
		}

		if (dn != 0)
		{
			WinOneSpectrumVScrollPos = BOUND(WinOneSpectrumVScrollPos + dn, 0, clsWaveFFT.FFTSize / 2 - WinOneSpectrumHeight / WinOneSpectrumVScrollZoom);
		}
		if (tbdn != 0)
		{
			WinOneSpectrumVScrollPos = BOUND(tbdn + dn, 0, clsWaveFFT.FFTSize / 2 - WinOneSpectrumHeight / WinOneSpectrumVScrollZoom);
		}
		if (dn != 0 || tbdn != 0)
		{
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = clsWaveFFT.FFTSize / 2 - WinOneSpectrumHeight / WinOneSpectrumVScrollZoom;
			si.nPage = 128;
			si.nPos = WinOneSpectrumVScrollPos;
			SetScrollInfo(hWndSpectrumVScrollBar, SB_CTL, &si, TRUE);

			//SetScrollPos(hWndSpectrumVScrollBar, SB_VERT, WinOneSpectrumVScrollPos, TRUE);
			UpdateWindow(hWndSpectrumVScrollBar);

			InvalidateRect(hWndOrignalSpectrum, NULL, TRUE);
			UpdateWindow(hWndOrignalSpectrum);

			InvalidateRect(hWndOrignalLogSpectrum, NULL, TRUE);
			UpdateWindow(hWndOrignalLogSpectrum);

			InvalidateRect(hWndFilttedSpectrum, NULL, TRUE);
			UpdateWindow(hWndFilttedSpectrum);

			InvalidateRect(hWndFilttedLogSpectrum, NULL, TRUE);
			UpdateWindow(hWndFilttedLogSpectrum);
		}
		break;

	case WM_HSCROLL:
		//Calculate new horizontal scroll position
		GetScrollRange(hWndSpectrumHScrollBar, SB_HORZ, &iMin, &iMax);
		iPos = GetScrollPos(hWndSpectrumHScrollBar, SB_HORZ);
		GetClientRect(hWnd, &rc);

		switch (GET_WM_HSCROLL_CODE(wParam, lParam))
		{
		case SB_LINERIGHT:
			dn = 16;
			break;
		case SB_LINELEFT:
			dn = -16;
			break;
		case SB_PAGERIGHT:
			dn = 128;
			break;
		case SB_PAGELEFT:
			dn = -128;
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
			WinOneSpectrumHScrollPos = BOUND(WinOneSpectrumHScrollPos + dn, 0, SDR_WIDTH - SDR_WIN_WIDTH / WinOneSpectrumHScrollZoom);
		}
		if (tbdn != 0)
		{
			WinOneSpectrumHScrollPos = BOUND(tbdn, 0, SDR_WIDTH - SDR_WIN_WIDTH / WinOneSpectrumHScrollZoom);
		}
		if (dn != 0 || tbdn != 0)
		{
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = SDR_WIDTH - SDR_WIN_WIDTH / WinOneSpectrumHScrollZoom;
			si.nPage = 128;
			si.nPos = WinOneSpectrumHScrollPos;
			SetScrollInfo(hWndSpectrumHScrollBar, SB_CTL, &si, TRUE);

			InvalidateRect(hWndOrignalSpectrum, NULL, TRUE);
			UpdateWindow(hWndOrignalSpectrum);

			InvalidateRect(hWndOrignalLogSpectrum, NULL, TRUE);
			UpdateWindow(hWndOrignalLogSpectrum);

			InvalidateRect(hWndFilttedSpectrum, NULL, TRUE);
			UpdateWindow(hWndFilttedSpectrum);

			InvalidateRect(hWndFilttedLogSpectrum, NULL, TRUE);
			UpdateWindow(hWndFilttedLogSpectrum);
		}
		break;
	}

}

void destroyItemData(HWND hWndTreeView, HTREEITEM hItem)
{
	if (hItem)
	{
		TVITEM tvi = { 0 };
		tvi.mask = TVIF_HANDLE | TVIF_PARAM;
		tvi.hItem = hItem;
		TreeView_GetItem(hWndTreeView, &tvi);
		HTREEITEM hChild = TreeView_GetChild(hWndTreeView, hItem);
		while (hChild)
		{
			destroyItemData(hWndTreeView, hChild);
			hChild = TreeView_GetNextSibling(hWndTreeView, hChild);
		}
	}
}

void destroyTreeItemData(HWND hWndTreeView)
{
	HTREEITEM hRoot = TreeView_GetRoot(hWndTreeView);
	destroyItemData(hWndTreeView, hRoot);
	HTREEITEM hNextItem = TreeView_GetNextSibling(hWndTreeView, hRoot);
	while (hNextItem)
	{
		destroyItemData(hWndTreeView, hNextItem);
		hNextItem = TreeView_GetNextSibling(hWndTreeView, hNextItem);
	}
}

LRESULT CALLBACK CWinSDR::DlgSDRSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{

		//DWORD dwPos = GetDlgItemInt(hDlg, IDC_EDITGOTO, 0, 0);
		//SetDlgItemInt(hDlg, IDC_EDIT_PLAY_STOP_POSITION,	clsSoundCard.dwPlayStopPosition, TRUE);
		//SetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, clsWaveFFT.FFTSize, false);
		//SetDlgItemInt(hDlg, IDC_EDIT_FFT_STEP, clsWaveFFT.FFTStep, false);

		clsWinSDR.hDlgSDRParams = hDlg;
		clsWinSDR.hWndTitle		= GetDlgItem(hDlg, IDC_STATIC_SDR_PARAMS_TITLE);
		clsWinSDR.hWndEdit		= GetDlgItem(hDlg, IDC_EDIT_SDR_PARAMS);
		clsWinSDR.hWndCombox	= GetDlgItem(hDlg, IDC_COMBO_SDR_PARAMS);
		clsWinSDR.hWndComment	= GetDlgItem(hDlg, IDC_STATIC_SDR_PARAMS_COMMENT);
		ShowWindow(clsWinSDR.hWndCombox, SW_HIDE);

		clsWinSDR.hWndTreeView = GetDlgItem(hDlg, IDC_SDR_PARMAS_TREE);
		SetFocus(clsWinSDR.hWndTreeView);
		int i = 0;
		clsSDR.buildTreeItems(clsWinSDR.hWndTreeView, NULL, &i);
		HTREEITEM item = TreeView_GetRoot(clsWinSDR.hWndTreeView);
		TreeView_SelectItem(clsWinSDR.hWndTreeView, item);
	}
	break;
	case WM_COMMAND:
			//LOWORD(wParam)    子窗口ID
			//HIWORD(wParam)    通知码
			//lParam 子窗口句柄
			switch (LOWORD(wParam))
			{
			case IDOK:
			case IDCANCEL:
				if (LOWORD(wParam) == IDOK)
				{
					//clsWaveFFT.FFTSize = GetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, NULL, false);
					//clsWaveFFT.FFTStep = GetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, NULL, false);
					//clsWaveFFT.Init();
					//clsWinFFT.Init();
					//clsWinSDR.Init();
					clsSDR.SDR_params_apply();
				}
				clsSDR.sel_SDR_params_index = -1;
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
				break;
			case IDAPPLY:
				clsSDR.SDR_params_apply();
				break;
			case IDC_COMBO_SDR_PARAMS:
				//if ((HWND)lParam == clsWinSDR.hWndCombox)
				if(HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_KILLFOCUS)
					// If the user makes a selection from the list:
					//   Send CB_GETCURSEL message to get the index of the selected list item.
					//   Send CB_GETLBTEXT message to get the item.
					//   Display the item in a messagebox.
				{
					int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL,
						(WPARAM)0, (LPARAM)0);
					char  ListItem[256];
					(TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT,
						(WPARAM)ItemIndex, (LPARAM)ListItem);
					printf("%s\r\n", ListItem);
					clsSDR.edit_check_range();
				}
				break;
			case IDC_EDIT_SDR_PARAMS:
				printf("edit %d, %d, %d\r\n", HIWORD(wParam), EN_KILLFOCUS, EN_CHANGE);
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					printf("edit_check_range EN_KILLFOCUS\r\n");

					clsSDR.edit_check_range();
				}
				break;
			}
		break;
	case WM_DESTROY:
	{
		HWND hWndTreeView = GetDlgItem(hDlg, IDC_SDR_PARMAS_TREE);
		destroyTreeItemData(hWndTreeView);
	}
	break;
	case WM_NOTIFY:
	{
		if (wParam == IDC_SDR_PARMAS_TREE && ((LPNMHDR)lParam)->code == NM_CUSTOMDRAW)
		{
			LPNMTVCUSTOMDRAW pNMTVCD = (LPNMTVCUSTOMDRAW)lParam;
			HWND hWndTreeView = ((LPNMHDR)lParam)->hwndFrom;
			HTREEITEM hItem = (HTREEITEM)TreeView_GetSelection(hWndTreeView);
			TVITEM tvi = { 0 };
			tvi.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
			tvi.hItem = hItem;
			TreeView_GetItem(hWndTreeView, &tvi);
			int i = (int)tvi.lParam;
			printf("i:%d, oldi:%d, level:%d, t:%s\r\n", i, clsSDR.sel_SDR_params_index, SDR_params[i].level, SDR_params[i].txt);
			if (clsSDR.sel_SDR_params_index != i)
			{
				printf("edit_check_range tree click.\r\n");
				clsSDR.edit_check_range();

				clsSDR.refresh_input_panel(i);
				//printf("%d,%d,%s\r\n", i, SDR_params[i].level, SDR_params[i].txt);
				if (SDR_params[clsSDR.sel_SDR_params_index].paramUpdateReason != SDR_params[i].paramUpdateReason && clsSDR.SDR_parmas_changed == true)
				{
					if(SDR_params[clsSDR.sel_SDR_params_index].paramUpdateReason != sdrplay_api_Update_None)
						clsSDR.SDR_params_apply();
				}
				clsSDR.sel_tvi = tvi;
				clsSDR.sel_SDR_params_index = i;
			}
		}
	}
	break;
	}
	return FALSE;
}


void CWinSDR::Init(void)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nPage = 128;
	si.nPos = 0;

	si.nMax = SDR_WIDTH - SDR_WIN_WIDTH;
	SetScrollInfo(hWndSpectrumHScrollBar, SB_CTL, &si, TRUE);

	si.nMax = clsWaveFFT.FFTSize / 2 - WinOneSpectrumHeight;
	SetScrollInfo(hWndSpectrumVScrollBar, SB_CTL, &si, TRUE);
}
