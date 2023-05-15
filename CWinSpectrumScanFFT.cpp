
#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "Debug.h"
#include "CSoundCard.h"
#include "CData.h"
#include "CFFT.h"
#include "CFilter.h"
#include "CWinFFT.h"
#include "CWinSDR.h"
#include "CWinTools.h"

#include "CWinSpectrumScanFFT.h"
#include "CWinSpectrumScanFFTShow.h"


using namespace std;
using namespace WINS;
using namespace WINS::SPECTRUM_SCAN;
using namespace DEVICES;

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define DIVLONG		10
#define DIVSHORT	5

#define TIMEOUT		100

#define FONT_HEIGHT	14

#define POINT_WIDTH 5
#define POINT_WIDTH2 10

CWinSpectrumScanFFT clsWinSpectrumScanFFT;

CWinSpectrumScanFFT::CWinSpectrumScanFFT()
{
	OPENCONSOLE_SAVED;
	RegisterWindowsClass();
}

CWinSpectrumScanFFT::~CWinSpectrumScanFFT()
{
	//CLOSECONSOLE;
}

void CWinSpectrumScanFFT::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinSpectrumScanFFT::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_SPECTRUM_SCAN_FFT;
	wcex.lpszClassName = WIN_SPECTRUM_SCAN_FFT_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinSpectrumScanFFT::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(WIN_SPECTRUM_SCAN_FFT_CLASS, "频谱扫描FFT窗口", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 1400, 1000, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

LRESULT CALLBACK CWinSpectrumScanFFT::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinSpectrumScanFFT.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinSpectrumScanFFT::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rt;

	switch (message)
	{
	case WM_CREATE:
	{
		this->hWnd = hWnd;
		hMenu = GetMenu(hWnd);

		uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		//KillTimer(hWnd, uTimerId);

		MakeToolsBar();

		clsWinSpectrumScanFFTShow.hWnd = CreateWindow(WIN_SPECTRUM_SCAN_FFT_SHOW_CLASS, "频谱扫描FFTShow窗口", WS_CHILDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
				CW_USEDEFAULT, 0, 1400, 1000, hWnd, NULL, hInst, this);
		ShowWindow(clsWinSpectrumScanFFTShow.hWnd, SW_SHOW);
		UpdateWindow(clsWinSpectrumScanFFTShow.hWnd);

		hMenuFFT = GetSubMenu(hMenu, 0);
		hMenuSpectrum = GetSubMenu(hMenu, 1);

		CheckMenuRadioItem(hMenuSpectrum, 0, 3, clsWinSpectrumScanFFTShow.SpectrumShow, MF_BYPOSITION);

		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_SHOW, (clsWinSpectrumScanFFTShow.bFFTShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_LOG_SHOW, (clsWinSpectrumScanFFTShow.bFFTLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_BRIEFLY_SHOW, (clsWinSpectrumScanFFTShow.bFFTBrieflyShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_FFT_BRIEFLY_LOG_SHOW, (clsWinSpectrumScanFFTShow.bFFTBrieflyLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

	}
	break;
	case WM_CHAR:
		DbgMsg("WinOneSDRScanFFT WM_CHAR\r\n");
		break;
	case WM_LBUTTONDOWN:
		break;
	case WM_MOUSEMOVE:
		break;
	case WM_TIMER:
		break;
	break;
	case WM_SIZE:
		GetClientRect(hWnd, &WinRect);
		GetClientRect(hWndRebar, &RebarRect);
		MoveWindow(hWndRebar, 0, 0, WinRect.right, RebarRect.bottom, true);
		MoveWindow(clsWinSpectrumScanFFTShow.hWnd, 0, RebarRect.bottom, WinRect.right, WinRect.bottom - RebarRect.bottom, true);
		//InvalidateRect(hWnd, NULL, TRUE);
		//UpdateWindow(hWnd);
		break;
	case WM_COMMAND:
		return OnCommand(message, wParam, lParam);
		break;
	//case WM_SYSCOMMAND:
	//	if (LOWORD(wParam) == SC_CLOSE)
	//	{
	//		ShowWindow(hWnd, SW_HIDE);
	//		break;
	//	}
	//	return DefWindowProc(hWnd, message, wParam, lParam);
	//	break;
	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;
	case WM_PAINT:
	{
		HDC	hDC;
		PAINTSTRUCT ps;
		hDC = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		this->hWnd = NULL;
		DbgMsg("WM_DESTROY CWinSpectrumScanFFT\r\n");
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND CWinSpectrumScanFFT::MakeToolsBar(void)
{
	hWndRebar = clsWinTools.CreateRebar(hWnd);
	static TBBUTTON tbb[5] = {
		{ MAKELONG(1, 0), IDM_SPECTRUM_SCAN_FFT, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"FFT" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL },
		{ MAKELONG(2, 0), IDM_SPECTRUM_SCAN_SET, TBSTATE_ENABLED, BTNS_AUTOSIZE,	{0}, 0, (INT_PTR)L"Setting" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP, {0}, 0, NULL },
		{ MAKELONG(0, 0), 0, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN,	{0}, 0, (INT_PTR)L"按钮" }
	};
	CWinTools::TOOL_TIPS tips[5] = {
		{ IDM_SPECTRUM_SCAN_FFT,"打开频谱扫描FFT窗口." },
		{ 0, NULL },
		{ IDM_SPECTRUM_SCAN_SET,"打开频谱扫描设置窗口." },
		{ 0, NULL },
		{ 0, "TBSTYLE_DROPDOWN" }
	};
	hWndToolbar = clsWinTools.CreateToolbar(hWnd, tbb, 5, tips, 5);

	// Add images
	TBADDBITMAP tbAddBmp = { 0 };
	tbAddBmp.hInst = HINST_COMMCTRL;
	tbAddBmp.nID = IDB_STD_SMALL_COLOR;
	SendMessage(hWndToolbar, TB_ADDBITMAP, 0, (WPARAM)&tbAddBmp);

	clsWinTools.CreateRebarBand(hWndRebar, "窗口", 1, 500, 0, hWndToolbar);

	//hWndTrack = CreateTrackbar(hWnd, 0, 100, 10);
	clsWinTools.CreateRebarBand(hWndRebar, "Value", 2, 0, 0, NULL);
	return hWndRebar;
}

bool CWinSpectrumScanFFT::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDM_SPECTRUM_ORIGNAL_SHOW:
		SendMessage(clsWinSpectrumScanFFTShow.hWnd, message, wParam, lParam);
		CheckMenuRadioItem(hMenuSpectrum, 0, 3, 0, MF_BYPOSITION);
		break;
	case IDM_SPECTRUM_ORIGNAL_LOG_SHOW:
		SendMessage(clsWinSpectrumScanFFTShow.hWnd, message, wParam, lParam);
		CheckMenuRadioItem(hMenuSpectrum, 0, 3, 1, MF_BYPOSITION);
		break;
	case IDM_SPECTRUM_BRIEFLY_SHOW:
		SendMessage(clsWinSpectrumScanFFTShow.hWnd, message, wParam, lParam);
		CheckMenuRadioItem(hMenuSpectrum, 0, 3, 2, MF_BYPOSITION);
		break;
	case IDM_SPECTRUM_BRIEFLY_LOG_SHOW:
		SendMessage(clsWinSpectrumScanFFTShow.hWnd, message, wParam, lParam);
		CheckMenuRadioItem(hMenuSpectrum, 0, 3, 3, MF_BYPOSITION);
		break;

	case IDM_FFT_HORZ_ZOOM_INCREASE:
	case IDM_FFT_HORZ_ZOOM_DECREASE:
	case IDM_FFT_HORZ_ZOOM_HOME:
	case IDM_FFT_VERT_ZOOM_INCREASE:
	case IDM_FFT_VERT_ZOOM_DECREASE:
	case IDM_FFT_VERT_ZOOM_HOME:
		PostMessage(clsWinSpectrumScanFFTShow.hWnd, message, wParam, lParam);
		break;

	case IDM_FFT_ORIGNAL_SHOW:
		SendMessage(clsWinSpectrumScanFFTShow.hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_SHOW, (clsWinSpectrumScanFFTShow.bFFTShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_ORIGNAL_LOG_SHOW:
		SendMessage(clsWinSpectrumScanFFTShow.hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_LOG_SHOW, (clsWinSpectrumScanFFTShow.bFFTLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_FILTTED_SHOW:
		SendMessage(clsWinSpectrumScanFFTShow.hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_SHOW, (clsWinSpectrumScanFFTShow.bFFTBrieflyShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_FILTTED_LOG_SHOW:
		SendMessage(clsWinSpectrumScanFFTShow.hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_LOG_SHOW, (clsWinSpectrumScanFFTShow.bFFTBrieflyLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_SET:
		//DialogBoxParam(hInst, (LPCTSTR)IDD_DLG_FFT_SET, hWnd, (DLGPROC)DlgFFTSetProc, (LPARAM)this);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}
