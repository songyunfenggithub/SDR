
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
#include "CWinSDRSet.h"

#include "CWinSpectrumScan.h"
#include "CWinSpectrumScanShow.h"
#include "CWinSpectrumScanFFT.h"
#include "CWinSpectrumScanFFTShow.h"
#include "CWinSpectrumScanSet.h"

using namespace std;
using namespace WINS; 
using namespace WINS::SPECTRUM_SCAN;
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

CWinSpectrumScan clsWinSpectrumScan;

CWinSpectrumScan::CWinSpectrumScan()
{
	OPENCONSOLE_SAVED;
	hMutex = CreateMutex(NULL, false, "CWinSpectrumScanhMutex");
	RegisterWindowsClass();
}

CWinSpectrumScan::~CWinSpectrumScan()
{
	
}

void CWinSpectrumScan::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinSpectrumScan::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_SPECTRUM_SCAN;
	wcex.lpszClassName = WIN_SPECTRUM_SCAN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinSpectrumScan::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(WIN_SPECTRUM_SCAN_CLASS, "频谱扫描窗口", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 1400, 1000, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

LRESULT CALLBACK CWinSpectrumScan::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinSpectrumScan.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinSpectrumScan::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rt;

	switch (message)
	{
	case WM_CREATE:

		OPENCONSOLE_SAVED;
		{
			clsWinSpectrumScan.hWnd = hWnd;

			//uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
			//KillTimer(hWnd, uTimerId);

			hWndRebar = MakeToolsBar();

			clsWinSpectrumScanShow.hWnd = CreateWindow(WIN_SPECTRUM_SCAN_SHOW_CLASS, "", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
					CW_USEDEFAULT, 0, 1400, 1000,  hWnd, NULL, hInst, NULL);
			ShowWindow(clsWinSpectrumScanShow.hWnd, SW_SHOW);
			UpdateWindow(clsWinSpectrumScanShow.hWnd);

			clsWinSpectrumScanFFTShow.fft->Data = AdcDataI;
			clsWinSpectrumScanFFTShow.fft->hPen = Pens[COLOR_PEN::Pen_Green];
			clsWinSpectrumScanFFTShow.fft->hPenLog = Pens[COLOR_PEN::Pen_Yellow];
			clsWinSpectrumScanFFTShow.fft->FFT_Process_CallBackInit = CWinSpectrumScanFFTShow::FFTProcessCallBackInit;
			clsWinSpectrumScanFFTShow.fft->FFT_Process_CallBack = CWinSpectrumScanFFTShow::FFTProcessCallBack;
			clsWinSpectrumScanFFTShow.fft->Init(FFTInfo_Spectrum_Scan.FFTSize, FFTInfo_Spectrum_Scan.FFTStep, FFTInfo_Spectrum_Scan.AverageDeep);
			clsWinSpectrumScanFFTShow.fft->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFFT::FFT_Thread, clsWinSpectrumScanFFTShow.fft, 0, NULL);
		}
		break;

	case WM_CHAR:
		DbgMsg("WinOneSDRScan WM_CHAR\r\n");
		//PostMessage(clsWinSpect.hWnd, message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		//clsWinSpect.ActivehWnd = NULL;
		break;
	case WM_MOUSEMOVE:
		MouseX = GET_X_LPARAM(lParam);
		MouseY = GET_Y_LPARAM(lParam);
		OnMouse();
		break;
	case WM_TIMER:
		//InvalidateRect(hWnd, NULL, TRUE);
		//UpdateWindow(hWnd);
		break;
	case WM_SIZE:
		GetClientRect(hWnd, &WinRect);
		GetClientRect(hWndRebar, &RebarRect);
		MoveWindow(hWndRebar, 0, 0, WinRect.right, RebarRect.bottom, true);
		MoveWindow(clsWinSpectrumScanShow.hWnd, 0, RebarRect.bottom, WinRect.right, WinRect.bottom - RebarRect.bottom, true);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;
	case WM_COMMAND:
		return OnCommand(message, wParam, lParam);
		break;
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
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		KeyAndScroll(message, wParam, lParam);
		break;
	case WM_DESTROY:
		clsWinSpectrumScanFFTShow.fft->FFTDoing = false;
		if (clsWinSpectrumScanFFT.hWnd)	DestroyWindow(clsWinSpectrumScanFFT.hWnd);
		if (clsWinSpectrumScanSet.hWnd)	DestroyWindow(clsWinSpectrumScanSet.hWnd);
		this->hWnd = NULL;
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CWinSpectrumScan::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDM_SPECTRUM_SCAN_FFT:
		clsWinSpectrumScanFFT.OpenWindow();
		break;
	case IDM_SPECTRUM_SCAN_SET:
		clsWinSpectrumScanSet.OpenWindow();
		break;
	case IDM_SDR_SET:
		clsWinSDRSet.OpenWindow();
		break;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

void CWinSpectrumScan::OnMouse(void)
{
}

void CWinSpectrumScan::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
{
	INT     iMax, iMin, iPos;
	int		dn = 0, tbdn = 0;
	RECT    rc;

	switch (message)
	{
	case WM_KEYDOWN:
		DbgMsg("win fft WM_KEYDOWN\r\n");
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
			DbgMsg("CWinSDRScan HScrollPos: %d, HScrollRange: %d.\r\n", HScrollPos, HScrollRange);
		}
		break;
	}
}

HWND CWinSpectrumScan::MakeToolsBar(void)
{
	hWndRebar = clsWinTools.CreateRebar(hWnd);
	static TBBUTTON tbb[7] = {
		{ MAKELONG(1, 0), IDM_SPECTRUM_SCAN_FFT, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"FFT" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL },
		{ MAKELONG(2, 0), IDM_SPECTRUM_SCAN_SET, TBSTATE_ENABLED, BTNS_AUTOSIZE,	{0}, 0, (INT_PTR)L"Setting" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP, {0}, 0, NULL },
		{ MAKELONG(2, 0), IDM_SDR_SET, TBSTATE_ENABLED, BTNS_AUTOSIZE,	{0}, 0, (INT_PTR)L"SDR Setting" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP, {0}, 0, NULL },
		{ MAKELONG(0, 0), 0, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN,	{0}, 0, (INT_PTR)L"按钮" }
	};
	CWinTools::TOOL_TIPS tips[7] = {
		{ IDM_SPECTRUM_SCAN_FFT,"打开频谱扫描FFT窗口." },
		{ 0, NULL },
		{ IDM_SPECTRUM_SCAN_SET,"打开频谱扫描设置窗口." },
		{ 0, NULL },
		{ IDM_SDR_SET,"打开频谱扫描设置窗口." },
		{ 0, NULL },
		{ 0, "TBSTYLE_DROPDOWN" }
	};
	hWndToolbar = clsWinTools.CreateToolbar(hWnd, tbb, 7, tips, 7);

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
