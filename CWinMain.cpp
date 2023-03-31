// WinMain.cpp: implementation of the CWinMain class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include <stdio.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <commdlg.h>

#include "public.h"
#include "CSoundCard.h"
#include "CDevices.h"
#include "CFile.h"


#include "CWaveFunc.h"
#include "CWaveData.h"
#include "CWaveFilter.h"
#include "CWaveFFT.h"

#include "CWinMain.h"
#include "CWinSpectrum.h"
#include "CWinPMT.h"
#include "CWinFilter.h"
#include "CWinSDR.h"

#include "CDataFromTcpIp.h"
#include "CDataFromUSB.h"
#include "CDataFromUsart.h"
#include "CDataFromSoundCard.h"
#include "CDataFromSDR.h"

#include "cuda_Filter.cuh"
#include "cuda_FFT.cuh"

#include "CWaveAnalyze.h"
#include "CAudioWin.h"

#include "MyDebug.h"

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define WAVE_RECT_HEIGHT				0x200UL
#define WAVE_RECT_BORDER_TOP		50UL
#define WAVE_RECT_BORDER_LEFT		50UL
#define WAVE_RECT_BORDER_RIGHT		200UL
#define WAVE_RECT_BORDER_BOTTON		0UL

#define DIVLONG		10
#define DIVSHORT	5

#define FONT_HEIGHT	14

#define TIMEOUT		50

extern CDevices	clsDevices;
extern CFile	clsFile;

CWinMain	clsWinMain;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWinMain::CWinMain()
{

	DrawInfo.wVSclMax = 256*2;
	DrawInfo.wHSclMax = 0;
	
	DrawInfo.iHZoom = 0;
	DrawInfo.iHOldZoom = 0;
	DrawInfo.iVZoom = -23;
	DrawInfo.iVOldZoom = 0;
	DrawInfo.dwHZoomedPos = 0;
	
	DrawInfo.FullVotage = FULL_VOTAGE;
	DrawInfo.VotagePerDIV = DrawInfo.FullVotage / ((UINT64)1 << (ADC_DATA_SAMPLE_BIT - 1));

	DrawInfo.fAutoScroll	= TRUE;
	DrawInfo.bOrignalSignalShow = TRUE;
	DrawInfo.bFilttedSignalShow = TRUE;

}

CWinMain::~CWinMain()
{

}

LRESULT CALLBACK CWinMain::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

BOOL CWinMain::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

ATOM CWinMain::MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)CWinMain::WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL;// (HBRUSH)GetStockObject( BLACK_BRUSH );(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_MENU_MAIN;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

LRESULT CALLBACK CWinMain::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinMain.WndProcReal(hWnd, message, wParam, lParam);
}

void CWinMain::KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT     iMax, iMin, iPos;
	int		dn = 0, tbdn = 0;
	RECT    rc;

	switch (message)
	{
	case WM_KEYDOWN:
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
			tbdn = GET_WM_VSCROLL_POS(wParam, lParam);
			break;
		default:
			dn = 0;
			break;
		}

		if (dn != 0)
		{
			DrawInfo.dwVZoomedPos = BOUND(DrawInfo.dwVZoomedPos + dn, 0, DrawInfo.dwVZoomedHeight - WAVE_RECT_HEIGHT);
			DrawInfo.wVSclPos = DrawInfo.dwVZoomedPos >> DrawInfo.iVFit;
		}
		if (tbdn != 0)
		{
			DrawInfo.wVSclPos = BOUND(tbdn, 0, DrawInfo.wVSclMax);
			DrawInfo.dwVZoomedPos = DrawInfo.wVSclPos << DrawInfo.iVFit;
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_VERT, DrawInfo.wVSclPos, TRUE);
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
			dn = (rc.right - 2 * DIVLONG) / 16 + 1;
			break;
		case SB_LINEUP:
			dn = -(rc.right - 2 * DIVLONG) / 16 + 1;
			break;
		case SB_PAGEDOWN:
			dn = rc.right / 2 + 1;
			break;
		case SB_PAGEUP:
			dn = -rc.right / 2 + 1;
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
			DrawInfo.dwHZoomedPos = BOUND(DrawInfo.dwHZoomedPos + dn, 0, DrawInfo.dwHZoomedWidth);
			DrawInfo.wHSclPos = DrawInfo.dwHZoomedPos >> DrawInfo.iHFit;
		}
		if (tbdn != 0)
		{
			DbgMsg(("scroll tumb"));
			DrawInfo.wHSclPos = BOUND(tbdn, 0, DrawInfo.wHSclMax);
			DrawInfo.dwHZoomedPos = DrawInfo.wHSclPos << DrawInfo.iHFit;
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_HORZ, DrawInfo.wHSclPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		break;
	}
}

LRESULT CALLBACK CWinMain::WndProcReal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:

		OPENCONSOLE;
		
		this->hWnd = hWnd;

		hMyMenu = GetMenu(hWnd);
		CheckMenuItem(hMyMenu, IDM_WAVEAUTOSCROLL,
			(DrawInfo.fAutoScroll ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMyMenu, IDM_WAVE_FOLLOW_ORIGNAL,
			(DrawInfo.fFollowByOrignal ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMyMenu, IDM_WAVE_ORIGNAL_SIGNAL_SHOW,
			(DrawInfo.bOrignalSignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMyMenu, IDM_WAVE_FILTTED_SIGNAL_SHOW,
			(DrawInfo.bFilttedSignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		clsWinSpect.hWnd = CreateWindow(SPECTRUM_WIN_CLASS, "Spectrum windows", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 1400, 800, NULL, NULL, hInst, NULL);
		clsWinSDR.hWnd = CreateWindow(SDR_WIN_CLASS, "SDR windows", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 1400, 800, NULL, NULL, hInst, NULL);


		DrawInfo.uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);

		//clsSoundCard.MyWave(NULL);

		//clsGetDataUSB.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDataFromUSB::GetDataUSBThreadFun, NULL, 0, NULL);
		//clsGetDataUsart.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDataFromUsart::GetDataUsartThreadFun, NULL, 0, NULL);

		//clsGetDataTcpIp.TcpIpThreadsStart();
		
		clsGetDataSDR.open_SDR_device();

		//clsGetDataSDC.OpenIn();

		//clsWaveFunc.WaveGenerate();

		/*
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CWaveFilter::filter_thread1, NULL, 0, NULL);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CWaveFilter::filter_thread2, NULL, 0, NULL);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CWaveFilter::filter_thread3, NULL, 0, NULL);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CWaveFilter::filter_thread4, NULL, 0, NULL);
		*/



		RestoreDefaultValue();

		CaculateHScroll();
		CaculateVScroll();

		clsWaveAnalyze.Init_Params();

		MainInit();

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CWaveFilter::cuda_filter_thread, NULL, 0, NULL);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CWaveFFT::FFT_Thread, NULL, 0, NULL);

		//cuda_FFT_hMutexBuff = CreateMutex(NULL, false, "cuda_FFT_hMutexBuff");
		m_audioWin = new CAudioWin();

		break;

	case WM_TIMER:
		//clsWaveData.GeneratorWave();
	{
		if (clsWaveData.AdcGetNew)
		{
			CaculateHScroll();
			clsWaveData.AdcGetNew = false;
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
	}
	break;

	case WM_SIZE:
		CaculateHScroll();
		InvalidateRect(hWnd, NULL, TRUE);
		break;

	case WM_COMMAND:
		OnCommand(hWnd, message, wParam, lParam);
		break;

	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;

	case WM_PAINT:
		Paint(hWnd);
		break;
	case WM_CLOSE:
	{
		int res = ::MessageBox(NULL, "[WM_CLOSE] 是否确定退出", "提示", MB_OKCANCEL);
		if (res == IDOK) { //点击 【确定】按钮
		}
		else if (res == IDCANCEL) {//点击【取消】按钮，不调用默认的消息处理 DefWindowProc 

			return 0;
		}
		else {
			return 0;
		}
	}
	break;
	case WM_QUIT:
	{
		::MessageBox(NULL, "WM_QUIT", "提示", MB_OK);
	}
	break;
	case WM_DESTROY:
		//::MessageBox(NULL, "WM_DESTROY", "提示", MB_OK);
		//clsGetDataUSB.GetDataWorking = false;
		//WaitForSingleObject(clsGetDataUSB.hThread, INFINITE);

		//退出所有正在处理线程
		Program_In_Process = false;
		//while (!isGetDataExited);
		Sleep(200);

		clsGetDataSDR.close_SDR_device();
		
		Cuda_Filter_UnInit();
		cuda_FFT_UnInit();

		if (clsWinSDR.hWnd) DestroyWindow(clsWinSDR.hWnd);
		if (clsWinSpect.hWnd) DestroyWindow(clsWinSpect.hWnd);
		if (clsWinFilter.hWnd) DestroyWindow(clsWinFilter.hWnd);

		SaveDefaultValue();

		DbgMsg("Main WM_DESTROY\r\n");

		CLOSECONSOLE;
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
	//{
	//	if (wParam == VK_ESCAPE) { //当按下ESC键时，模拟退出消息

	//		::MessageBox(NULL, "WM_KEYUP消息, ESC 键盘按下", "提示", MB_OK);

	//		//该函数将指定的消息发送到一个或多个窗口。此函数为指定的窗口调用窗口程序，直到窗口程序处理完消息再返回。
	//		//LRESULT SendMessage（HWND hWnd，UINT Msg，WPARAM wParam，LPARAM IParam）；


	//		//该函数将一个消息放入（寄送）到与指定窗口创建的线程相联系消息队列里，不等待线程处理消息就返回。
	//		//消息队列里的消息通过调用GetMessage和PeekMessage取得
	//		//B00L PostMessage（HWND hWnd，UINT Msg，WPARAM wParam，LPARAM lParam）；

	//		::PostMessage(hWnd, WM_CLOSE, 0, 0);  //手动发送一个WM_CLOSE的消息
	//	}
	//}
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		KeyAndScroll(hWnd, message, wParam, lParam);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
	//return 0;
}

VOID CWinMain::Paint(HWND hWnd)
{
	HDC		hDC;
	PAINTSTRUCT ps;
	RECT	rt, r;
	HPEN	hPen, hPenLighter;

	hDC = BeginPaint(hWnd, &ps);

	GetClientRect(hWnd, &rt);

	HDC		hdc = CreateCompatibleDC(hDC);
	HBITMAP hbmp = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hdc, hbmp);

	SelectObject(hdc, CreateFont(FONT_HEIGHT, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));

	FillRect(hdc, &rt, (HBRUSH)GetStockObject(BLACK_BRUSH));

	int	x, y, pos, i;
	TCHAR	s[1024];
	int		zoomX, zoomY;
	zoomX = 1;
	zoomY = 1;
	pos = 0;
	s[0] = 0;

	hPen = CreatePen(PS_DOT, 0, RGB(64, 64, 64));
	hPenLighter = CreatePen(PS_DOT, 0, RGB(128, 128, 128));
	SetBkColor(hdc, RGB(0, 0, 0));
	r.top = WAVE_RECT_BORDER_TOP - DIVLONG - FONT_HEIGHT;
	r.right = rt.right;
	r.bottom = rt.bottom;
	SetTextColor(hdc, RGB(255, 255, 255));

	for (x = WAVE_RECT_BORDER_LEFT, i = 0; x < rt.right - WAVE_RECT_BORDER_RIGHT; x += 8)
	{
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP - ((x - WAVE_RECT_BORDER_LEFT) % 32 ? DIVSHORT : DIVLONG), NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP);

		if (!((x - WAVE_RECT_BORDER_LEFT) % 32))
		{
			if (!(i % 4))
			{
				sprintf(s, "%d", i);
				r.left = x;
				DrawText(hdc, s, strlen(s), &r, NULL);
			}
			SelectObject(hdc, i % 10 ? hPen : hPenLighter);
			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
			i++;
		}
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT, NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT + ((x - WAVE_RECT_BORDER_LEFT) % 32 ? DIVSHORT : DIVLONG));
	}
	r.top = WAVE_RECT_BORDER_TOP - FONT_HEIGHT / 2;
	r.left = rt.right - WAVE_RECT_BORDER_RIGHT + DIVLONG + 2;
	r.right = rt.right;
	r.bottom = rt.bottom;
	for (y = WAVE_RECT_BORDER_TOP, i = 0; y <= WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT; y += 8)
	{
		MoveToEx(hdc, WAVE_RECT_BORDER_LEFT - ((y - WAVE_RECT_BORDER_TOP) % 32 ? DIVSHORT : DIVLONG), y, NULL);
		LineTo(hdc, WAVE_RECT_BORDER_LEFT, y);
		if (!((y - WAVE_RECT_BORDER_TOP) % 32))
		{
			if (i % 2 == 0) {
				formatKDouble(DrawInfo.FullVotage - DrawInfo.VotagePerDIV * ((DrawInfo.dwVZoomedPos + i * 32) / DrawInfo.dbVZoom),
					32 / DrawInfo.dbVZoom * DrawInfo.VotagePerDIV, "v", s);
				DrawText(hdc, s, strlen(s), &r, NULL);
				r.top += 2 * 32;
			}
			SelectObject(hdc, i % 4 ? hPen : hPenLighter);
			LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, y);
			i++;
		}
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, y, NULL);
		LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT + ((y - WAVE_RECT_BORDER_TOP) % 32 ? DIVSHORT : DIVLONG), y);
	}
	DeleteObject(hPen);
	DeleteObject(hPenLighter);

	//Draw Data Content -----------------------------------------------

	//lb.lbHatch = 0; 
	//lb.lbStyle = BS_SOLID; 
	//lb.lbColor = RGB(,0,0); 
	//hPen = ExtCreatePen(PS_COSMETIC | PS_DOT, 1, &lb, 0, NULL); 
		// Draw Data

	UINT dwWStep, dwXOffSet, bufStep, dwMark;
	if (DrawInfo.iHZoom > 0)
	{
		dwMark = (0xFFFFFFFF >> (32 - DrawInfo.iHZoom));
		dwWStep = 1 + dwMark;
		dwXOffSet = DrawInfo.dwHZoomedPos & dwMark;
		bufStep = 1;

	}
	else if (DrawInfo.iHZoom < 0)
	{
		dwWStep = 1;
		dwXOffSet = 0;
		bufStep = 1 + (0xFFFFFFFF >> (32 + DrawInfo.iHZoom));
	}
	else
	{
		dwWStep = 1;
		dwXOffSet = 0;
		bufStep = 1;
	}

	int InfolastPos;
	double vzoom = DrawInfo.dbVZoom;// DrawInfo.iVZoom >= 0 ? (1 << DrawInfo.iVZoom) : (1.0 / (1 << (-DrawInfo.iVZoom)));

	if (DrawInfo.bOrignalSignalShow) {
		//绘制原信号--------------------------------------------------
		hPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
		//SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		SelectObject(hdc, hPen);//(HPEN)GetStockObject(WHITE_PEN));
		x = WAVE_RECT_BORDER_LEFT - dwXOffSet;
		PADCDATATYPE pData = clsWaveData.AdcBuff +
			(
				DrawInfo.iHZoom > 0
				? DrawInfo.dwHZoomedPos >> DrawInfo.iHZoom
				: DrawInfo.dwHZoomedPos << (-DrawInfo.iHZoom)
				);
		//UINT64 voffset = WAVE_RECT_HEIGHT >> 1;

		y = DrawInfo.dwVZoomedHeight / 2 - (*pData * vzoom) - DrawInfo.dwVZoomedPos;
		y = BOUND(y, 0, WAVE_RECT_HEIGHT);
		//printf("zoom:%f, v:%d, zoomv: %d\r\n", vzoom, *pData, y);
		InfolastPos = -1;
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
		do
		{
			pData += bufStep;
			x += dwWStep;
			if (pData == &clsWaveData.AdcBuff[clsWaveData.AdcPos & DATA_BUFFER_MASK])InfolastPos = x;
			y = DrawInfo.dwVZoomedHeight / 2 - (*pData * vzoom) - DrawInfo.dwVZoomedPos;
			y = BOUND(y, 0, WAVE_RECT_HEIGHT);
			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + y);
		} while (x < rt.right - WAVE_RECT_BORDER_RIGHT);
		if (InfolastPos >= 0)
		{
			MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
		}
		DeleteObject(hPen);
	}

	if (DrawInfo.bFilttedSignalShow) {
		//绘制滤波信号--------------------------------------------------
		hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
		//SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		SelectObject(hdc, hPen);//(HPEN)GetStockObject(WHITE_PEN));
		x = WAVE_RECT_BORDER_LEFT - dwXOffSet;
		FILTEDDATATYPE* pFilterData = clsWaveData.FilttedBuff +
			(
				DrawInfo.iHZoom > 0
				? DrawInfo.dwHZoomedPos >> DrawInfo.iHZoom
				: DrawInfo.dwHZoomedPos << (-DrawInfo.iHZoom)
				);
		y = DrawInfo.dwVZoomedHeight / 2 - (*pFilterData * vzoom) - DrawInfo.dwVZoomedPos;
		y = BOUND(y, 0, WAVE_RECT_HEIGHT);
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
		InfolastPos = -1;
		do
		{
			pFilterData += bufStep;
			x += dwWStep;
			y = DrawInfo.dwVZoomedHeight / 2 - (*pFilterData * vzoom) - DrawInfo.dwVZoomedPos;
			y = BOUND(y, 0, WAVE_RECT_HEIGHT);
			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + y);
			if (pFilterData == &clsWaveData.FilttedBuff[clsWaveData.FilttedPos & DATA_BUFFER_MASK]) {
				MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
				LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
				MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
				//InfolastPos = x;
			}
		} while (x < rt.right - WAVE_RECT_BORDER_RIGHT);
		
		/*
		if (InfolastPos >= 0)
		{
			MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
		}
		*/
		DeleteObject(hPen);
	}

	//---------------------------------------

	double z = DrawInfo.iVZoom >= 0 ? (1.0 / ((UINT64)1 << DrawInfo.iVZoom)) : ((UINT64)1 << -DrawInfo.iVZoom);
	char tstr1[100], tstr2[100], tstradcpos[100], tstrfiltpos[100], tstrfftpos[100];
	//clsWaveData.NumPerSec = 38400;
	double TimePreDiv = 32.0 / clsWaveData.AdcSampleRate * 
		( DrawInfo.iHZoom >= 0 ? 1.0 / ((UINT64)1 << DrawInfo.iHZoom) : ((UINT64)1 << -DrawInfo.iHZoom));
	sprintf(s, "32 / DIV    %sV / DIV    %ss / DIV 中文\r\n"\
		"Pos:%s\r\nFilterPos:%s\r\nFFTPos:%s\r\n"\
		"hBarPos: %d\r\n"\
		"AdcSampleRate: %d    Real AdcSampleRate: %d",
		formatKKDouble(32.0 * DrawInfo.VotagePerDIV * z, "", tstr1),
		formatKKDouble(TimePreDiv, "", tstr2),
		fomatKINT64(clsWaveData.AdcPos, tstradcpos),
		fomatKINT64(clsWaveData.FilttedPos, tstrfiltpos),
		fomatKINT64(clsWaveFFT.FFTPos, tstrfftpos),
		DrawInfo.dwHZoomedPos,
		clsWaveData.AdcSampleRate, clsWaveData.NumPerSec
		);

	r.top		= WAVE_RECT_BORDER_TOP + DIVLONG;
	r.left		= WAVE_RECT_BORDER_LEFT + DIVLONG;
	r.right		= rt.right;
	r.bottom	= rt.bottom;
	SetBkMode(hdc, TRANSPARENT); 
//	SetBkMode(hdc, OPAQUE); 
//	SetBkColor(hdc,RGB(0,0,0));
	SetTextColor(hdc,RGB(255,255,255));

	DrawText(hdc, s, strlen(s), &r, NULL);

	sprintf(s,
	"HZoom: %d, %d \t VZoom: %d, %d",
		DrawInfo.iHZoom, DrawInfo.iHZoom >= 0 ? 1 << DrawInfo.iHZoom : -(1 << -DrawInfo.iHZoom),
		DrawInfo.iVZoom, DrawInfo.iVZoom >= 0 ? 1 << DrawInfo.iVZoom : -(1 << -DrawInfo.iVZoom)
	);
	r.top += FONT_HEIGHT * 6;
	DrawText(hdc, s, strlen(s), &r, NULL);

	sprintf(s,"vzoom %.40f",vzoom);
	r.top += FONT_HEIGHT;
	DrawText(hdc, s, strlen(s), &r, NULL);

	DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

	BitBlt(hDC, 
           0, 0, 
           rt.right, rt.bottom, 
           hdc, 
           0, 0, 
           SRCCOPY);
	DeleteObject(hdc);
	DeleteObject(hbmp);

	EndPaint(hWnd, &ps);
}

VOID CWinMain::GetRealClientRect (HWND hwnd, PRECT lprc)
{
    DWORD dwStyle;

    dwStyle = GetWindowLong (hwnd, GWL_STYLE);
    GetClientRect (hwnd,lprc);

    if (dwStyle & WS_HSCROLL)
        lprc->bottom += GetSystemMetrics (SM_CYHSCROLL);

    if (dwStyle & WS_VSCROLL)
        lprc->right  += GetSystemMetrics (SM_CXVSCROLL);
}

VOID CWinMain::CaculateHScroll(void)
{
	RECT       rc;
	INT        iRangeH, iRangeV, i, n;
	static UINT iSem = 0;

	GetRealClientRect(hWnd, &rc);
	DrawInfo.dwDataWidth = DATA_BUFFER_LENGTH;//  clsWaveData.AdcPos;
	DrawInfo.dwHZoomedWidth = DrawInfo.dwDataWidth;
	if (DrawInfo.iHZoom > 0)
	{
		for (i = 0; i < DrawInfo.iHZoom; i++)
		{							     
			if (DrawInfo.dwHZoomedWidth & (DATA_BUFFER_LENGTH << 5)) // 最大放大32倍
			{
				DrawInfo.iHZoom = i;
				break;
			}
			else
			{
				DrawInfo.dwHZoomedWidth = DrawInfo.dwHZoomedWidth << 1;
			}
		}
		DrawInfo.dbHZoom = 1 << DrawInfo.iHZoom;
	}
	else
	{
		for (n = 0, i = DrawInfo.iHZoom; i; i++)
		{
			if (DrawInfo.dwHZoomedWidth > (rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT))
			{
				DrawInfo.dwHZoomedWidth = DrawInfo.dwHZoomedWidth >> 1;
				n++;
			}
			else
			{
				DrawInfo.iHZoom = -n;
				break;
			}
		}
		DrawInfo.dbHZoom = (double)1.0 / (1 << -DrawInfo.iHZoom);
	}

	if (DrawInfo.fAutoScroll) {
		DrawInfo.dwHZoomedPos = (DrawInfo.fFollowByOrignal == true ? clsWaveData.AdcPos : clsWaveData.FilttedPos) * DrawInfo.dbHZoom -
			(rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		if (DrawInfo.dwHZoomedPos < 0) DrawInfo.dwHZoomedPos = 0;
	}
	else {
		i = DrawInfo.iHOldZoom - DrawInfo.iHZoom;
		if (i > 0)
			DrawInfo.dwHZoomedPos = DrawInfo.dwHZoomedPos >> i;
		else
			DrawInfo.dwHZoomedPos = DrawInfo.dwHZoomedPos << -i;
	}
	DrawInfo.iHOldZoom = DrawInfo.iHZoom;

	// caculate scroll max width
	UINT64 realHZoomedWidth = DrawInfo.dwHZoomedWidth - (rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
	for (DrawInfo.iHFit = 0; (realHZoomedWidth >> DrawInfo.iHFit) > 0xFFFF; DrawInfo.iHFit++);
	DrawInfo.wHSclMax = realHZoomedWidth >> DrawInfo.iHFit;
	DrawInfo.wHSclPos = DrawInfo.dwHZoomedPos >> DrawInfo.iHFit;
	DrawInfo.dwHZoomedPos = BOUND(DrawInfo.dwHZoomedPos, 0, realHZoomedWidth);
	DrawInfo.wHSclPos = BOUND(DrawInfo.wHSclPos, 0, DrawInfo.wHSclMax);

	//iRangeV = DrawInfo.wVSclMax;// - rc.bottom;
	iRangeH = DrawInfo.wHSclMax;// - rc.right;

	if (iRangeH < 0) iRangeH = 0;
	//if (iRangeV < 0) iRangeV = 0;

	//SetScrollRange(hWnd, SB_VERT, 0, iRangeV, TRUE);
	SetScrollRange(hWnd, SB_HORZ, 0, iRangeH, TRUE);
	//SetScrollPos(hWnd, SB_VERT, DrawInfo.wVSclPos, TRUE);
	SetScrollPos(hWnd, SB_HORZ, DrawInfo.wHSclPos, TRUE);
	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);
}

VOID CWinMain::CaculateVScroll(void)
{
	//RECT       rc;
	INT        iRangeH, iRangeV, i, n;
	static UINT iSem = 0;

	//GetRealClientRect(hWnd, &rc);

	//DrawInfo.dwDataHeight = (UINT64)1 << ADC_DATA_SAMPLE_BIT;
	UINT64	maxDataHeight = (UINT64)1 << (ADC_DATA_SAMPLE_BIT + ADC_DATA_MAX_ZOOM_BIT);
	DrawInfo.dwVZoomedHeight = MoveBits(MoveBits(1, ADC_DATA_SAMPLE_BIT), DrawInfo.iVZoom);
	DrawInfo.dbVZoom = DrawInfo.iVZoom > 0 ? (UINT64)1 << DrawInfo.iVZoom : (double)1.0 / ((UINT64)1 << -DrawInfo.iVZoom);

	i = DrawInfo.iVOldZoom - DrawInfo.iVZoom;
	if (i > 0) {
		DrawInfo.dwVZoomedPos = ((DrawInfo.dwVZoomedPos + WAVE_RECT_HEIGHT/2) >> i) - WAVE_RECT_HEIGHT / 2;
	}
	else {
		DrawInfo.dwVZoomedPos = ((DrawInfo.dwVZoomedPos + WAVE_RECT_HEIGHT / 2) << -i) - WAVE_RECT_HEIGHT / 2;
	}
	DrawInfo.iVOldZoom = DrawInfo.iVZoom;

	// caculate scroll max height
	for (DrawInfo.iVFit = 0; ((DrawInfo.dwVZoomedHeight - WAVE_RECT_HEIGHT) >> DrawInfo.iVFit) > 0xFFFF; DrawInfo.iVFit++);
	DrawInfo.wVSclMax = (DrawInfo.dwVZoomedHeight - WAVE_RECT_HEIGHT) >> DrawInfo.iVFit;
	DrawInfo.wVSclPos = DrawInfo.dwVZoomedPos >> DrawInfo.iVFit;
	DrawInfo.dwVZoomedPos = BOUND(DrawInfo.dwVZoomedPos, 0, DrawInfo.dwVZoomedHeight - WAVE_RECT_HEIGHT);
	DrawInfo.wVSclPos = BOUND(DrawInfo.wVSclPos, 0, DrawInfo.wVSclMax);

	iRangeV = DrawInfo.wVSclMax;
	if (iRangeV < 0) iRangeV = 0;
	SetScrollRange(hWnd, SB_VERT, 0, iRangeV, TRUE);
	SetScrollPos(hWnd, SB_VERT, DrawInfo.wVSclPos, TRUE);
	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);
}

VOID CWinMain::SetScrollRanges(HWND hwnd)
{
    RECT       rc;
    UINT        iRangeH, iRangeV, i;    
    static UINT iSem = 0;

    if (!iSem){
        iSem++;
        GetRealClientRect (hwnd, &rc);

        for (i = 0; i < 2; i++){
            iRangeV = DrawInfo.wVSclMax - rc.bottom;
            iRangeH = DrawInfo.wHSclMax - rc.right;

			//iRangeV = 1000 - rc.bottom;
            //iRangeH = 1000 - rc.right;

            if (iRangeH < 0) iRangeH = 0;
            if (iRangeV < 0) iRangeV = 0;

            if (GetScrollPos ( hwnd,
                               SB_VERT) > iRangeV ||
                               (DrawInfo.wHSclPos = 
							   GetScrollPos (hwnd, SB_HORZ)) > iRangeH)
                InvalidateRect (hwnd, NULL, TRUE);

            SetScrollRange (hwnd, SB_VERT, 0, iRangeV, TRUE);
            SetScrollRange (hwnd, SB_HORZ, 0, iRangeH, TRUE);

            GetClientRect (hwnd, &rc);
        }
        iSem--;
    }
}

BOOL CWinMain::OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	wmId    = LOWORD(wParam); 
	wmEvent = HIWORD(wParam); 
	// Parse the menu selections:
//	DbgMsg(("MDIWave commandID:%08X",wmId));
	switch (wmId)
	{

//GENERATOR COMMANDS-----------------------
	case IDM_GENERATORWAVE:
		clsSoundCard.GeneratorWave();
		break;
//ANALYZE COMMANDS-----------------------
	case IDM_ANALYZE_SHOW_WAVE_VALUE:
		if(clsSoundCard.hWndWaveValue)break;
		clsSoundCard.hWndWaveValue = CreateDialog(hInst, 
                 MAKEINTRESOURCE(IDD_DLG_SHOW_WAVE_VALUE), 
                 hWnd, (DLGPROC) CSoundCard::DlgWaveValueProc); 
        ShowWindow(clsSoundCard.hWndWaveValue, SW_SHOW); 
		break;
	case IDM_ANALYZEWAVE:	  
		clsSoundCard.ClearNoise3();
		//clsSoundCard.MySound(NULL);
		break;
	case IDM_ANALYZECLEARNOISE:
		clsSoundCard.ClearNoise2();
		break;
	case IDM_ANALYZEGOTO:
		DialogBox(hInst, (LPCTSTR)IDD_DLGGOTO, hWnd, (DLGPROC)CSoundCard::DlgGotoProc);
		break;
//FILE COMMANDS-----------------------
	case IDM_FILENEW:
		clsFile.NewFile();
		break;
	case IDM_FILEOPEN:
		clsFile.OpenWaveFile();
		break;
	case IDM_FILECLOSE:

		break;
	case IDM_FILESAVE:
		clsFile.SaveFile();
		break;
	case IDM_FILESAVEAS:
		clsFile.SaveFileAs();
		break;
	case IDM_FILEPROPERTIES:
		DialogBox(hInst, (LPCTSTR)IDD_DLGPROPERTIES, hWnd, (DLGPROC)CSoundCard::DlgPropertiesProc);
		break;

//EDIT COMMANDS-----------------------
	case IDM_EDIT_SET_CUT_POS:
		if(clsFile.hWndGetPos)break;
		clsFile.hWndGetPos = CreateDialog(hInst, 
                 MAKEINTRESOURCE(IDD_DLGSAVELENGTH), 
                 hWnd, (DLGPROC) CFile::DlgSaveLengthProc); 
        ShowWindow(clsFile.hWndGetPos, SW_SHOW); 
		break;

//PLAY COMMANDS-----------------------
		case IDM_STARTPLAY:
			clsSoundCard.OpenOut(0,clsSoundCard.outBufLength);
			break;
		case IDM_PAUSEPLAY:
			clsSoundCard.PauseOut();
			break;
		case IDM_STOPPLAY:
			clsSoundCard.CloseOut();
			break;
		case IDM_FROMPOSPLAY:
			clsSoundCard.OpenOut( DrawInfo.iHZoom > 0 ? 
							 DrawInfo.dwHZoomedPos >> DrawInfo.iHZoom
							 :
							 DrawInfo.dwHZoomedPos << -DrawInfo.iHZoom
							, clsSoundCard.dwPlayStopPosition);
			break;
		case IDM_PLAY_STOP_POSITION:
		if(clsSoundCard.hWndPlayStopPosition)break;
		clsSoundCard.hWndPlayStopPosition = CreateDialog(hInst, 
                 MAKEINTRESOURCE(IDD_DLG_PLAY_STOP_POSITION), 
                 hWnd, (DLGPROC) CSoundCard::DlgPlayStopPosProc); 
        ShowWindow(clsSoundCard.hWndPlayStopPosition, SW_SHOW); 
			break;

//RECORD COMMANDS-----------------------
		case IDM_STARTRECORD:
			clsSoundCard.OpenIn(0, SOUNDCARD_IN_BUFFER_LENGTH);
			break;
		case IDM_STOPRECORD:
			clsSoundCard.CloseIn();
			clsWinMain.CaculateHScroll();
			InvalidateRect(clsWinMain.hWnd,NULL,TRUE);
		break;
		case IDM_RECORDFORMAT:
			clsDevices.FormatChoose(&clsSoundCard.FormatEx);
			break;

//DEVICES COMMANDS-----------------------
		case IDM_LISTWAVEINDEVICES:
			{
			UINT uDevId = DialogBoxParam(hInst, MAKEINTRESOURCE(DLG_WAVEDEVICE), hWnd,
								(DLGPROC)CDevices::WaveDeviceDlgProc,
								MAKELONG((WORD)clsDevices.guWaveInId, TRUE));
			}
         break;
		case IDM_LISTWAVEOUTDEVICES:
/*
			uDevId = DialogBoxParam(clsWinMain.hInst, DLG_WAVEDEVICE, hWnd,
                                    AcmAppWaveDeviceDlgProc,
                                    MAKELONG((WORD)guWaveOutId, FALSE));

            if (uDevId != guWaveOutId)
            {
                guWaveOutId = uDevId;
                AcmAppDisplayFileProperties(hwnd, &gaafd);
            }
*/
            break;

//ZOOM COMMANDS-----------------------
	case IDM_WAVEHZOOMRESET:
		DrawInfo.iHZoom = 0;
		CaculateHScroll();
		break;
	case IDM_WAVEHZOOMINCREASE:
		DrawInfo.iHZoom++;
		CaculateHScroll();
		break;
	case IDM_WAVEHZOOMDECREASE:
		DrawInfo.iHZoom--;
		CaculateHScroll();
		break;
	case IDM_WAVEVZOOMRESET:
		DrawInfo.iVZoom = 0;
		CaculateVScroll();
		break;
	case IDM_WAVEVZOOMINCREASE:
		if (DrawInfo.iVZoom < ADC_DATA_MAX_ZOOM_BIT) {
			DrawInfo.iVZoom++;
			CaculateVScroll();
		}
		break;
	case IDM_WAVEVZOOMDECREASE:
		if (MoveBits(MoveBits(1, ADC_DATA_SAMPLE_BIT), DrawInfo.iVZoom) > WAVE_RECT_HEIGHT) {
			DrawInfo.iVZoom--;
			CaculateVScroll();
		}
		break;
	case IDM_WAVEAUTOSCROLL:
		DrawInfo.fAutoScroll = !DrawInfo.fAutoScroll;
		if(DrawInfo.fAutoScroll)
			DrawInfo.uTimerId = SetTimer(hWnd, 0, TIMEOUT , NULL);
		else
			KillTimer(hWnd, 0);//DrawInfo.uTimerId);
		CheckMenuItem(hMyMenu, IDM_WAVEAUTOSCROLL, 
			(DrawInfo.fAutoScroll ? MF_CHECKED : MF_UNCHECKED ) | MF_BYCOMMAND);
		break;
	case IDM_WAVE_FOLLOW_ORIGNAL:
		DrawInfo.fFollowByOrignal = !DrawInfo.fFollowByOrignal;
		CheckMenuItem(hMyMenu, IDM_WAVE_FOLLOW_ORIGNAL,
			(DrawInfo.fFollowByOrignal ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;

//COMMANDS-----------------------
	case IDM_SPECTRUMWINDOW:
		clsWinSpect.OpenWindow();
		break;

//COMMANDS-----------------------
	case IDM_PMT_WINDOW:
		break;

//COMMANDS-----------------------
	case IDM_SDR_WINDOW:
		clsWinSDR.OpenWindow();
		break;
		//COMMANDS-----------------------
	case IDM_SOUND_WINDOW:
		m_audioWin->OpenWindow();
		break;
//COMMANDS-----------------------
	case IDM_FILTERWINDOW:
		clsWinFilter.OpenWindow();
		break;

//COMMANDS-----------------------
	case IDM_WAVE_AUTO_FIT:
		
		break;
	case IDM_WAVE_DC:

		break;
	case IDM_WAVE_AC:

		break;
	case IDM_WAVE_SIGNALS_OFFSET:

		break;
	case IDM_WAVE_ORIGNAL_SIGNAL_SHOW:
		DrawInfo.bOrignalSignalShow = !DrawInfo.bOrignalSignalShow;
		CheckMenuItem(hMyMenu, IDM_WAVE_ORIGNAL_SIGNAL_SHOW,
			(DrawInfo.bOrignalSignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		break;
	case IDM_WAVE_FILTTED_SIGNAL_SHOW:
		DrawInfo.bFilttedSignalShow = !DrawInfo.bFilttedSignalShow;
		CheckMenuItem(hMyMenu, IDM_WAVE_FILTTED_SIGNAL_SHOW,
			(DrawInfo.bFilttedSignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		break;

//COMMANDS-----------------------
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
	default:
	   return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

void CWinMain::SaveDefaultValue(void)
{
	WritePrivateProfileString("WIN_MAIN", "iHZoom", std::to_string(DrawInfo.iHZoom).c_str(), IniFilePath);
	WritePrivateProfileString("WIN_MAIN", "iVZoom", std::to_string(DrawInfo.iVZoom).c_str(), IniFilePath);
	WritePrivateProfileString("WIN_MAIN", "iVOldZoom", std::to_string(DrawInfo.iVOldZoom).c_str(), IniFilePath);
	WritePrivateProfileString("WIN_MAIN", "dwVZoomedPos", std::to_string(DrawInfo.dwVZoomedPos).c_str(), IniFilePath);
}

void CWinMain::RestoreDefaultValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	GetPrivateProfileString("WIN_MAIN", "iHZoom", "1", value, VALUE_LENGTH, IniFilePath);
	DrawInfo.iHZoom = atoi(value);
	GetPrivateProfileString("WIN_MAIN", "iVZoom", "1", value, VALUE_LENGTH, IniFilePath);
	DrawInfo.iVZoom = atoi(value);
	GetPrivateProfileString("WIN_MAIN", "iVOldZoom", "1", value, VALUE_LENGTH, IniFilePath);
	DrawInfo.iVOldZoom = atoi(value);
	GetPrivateProfileString("WIN_MAIN", "dwVZoomedPos", "1", value, VALUE_LENGTH, IniFilePath);
	DrawInfo.dwVZoomedPos = std::strtoll(value, NULL, 10);
}