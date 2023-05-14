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

#include "CData.h"
#include "CFilter.h"

#include "CWinMain.h"
#include "CWinSpectrum.h"
#include "CWinPMT.h"
#include "CWinFilter.h"
#include "CFFTforFilterAnalyze.h"
#include "CWinSDR.h"

#include "CDataFromTcpIp.h"
#include "CDataFromUSB.h"
#include "CDataFromUsart.h"
#include "CDataFromSoundCard.h"
#include "CDataFromSDR.h"

#include "cuda_CFilter.cuh"
#include "cuda_CFilter2.cuh"

#include "CAnalyze.h"
#include "CWinAudio.h"
#include "CWinFiltted.h"
#include "CWinTools.h"

#include "Debug.h"

using namespace WINS; 
using namespace DEVICES;

#define WIN_MAIN_CLASS "WIN_MAIN_CLASS"

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

CWinMain	clsWinMain;

CWinMain::CWinMain()
{
	OPENCONSOLE;

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
	CLOSECONSOLE;
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

   hWnd = CreateWindow(WIN_MAIN_CLASS, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

ATOM CWinMain::RegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)CWinMain::StaticWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)GetStockObject( BLACK_BRUSH );//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_MENU_MAIN;
	wcex.lpszClassName	= WIN_MAIN_CLASS;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

LRESULT CALLBACK CWinMain::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinMain.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinMain::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		this->hWnd = hWnd;
		hMenu = GetMenu(hWnd);
		CheckMenuItem(hMenu, IDM_WAVEAUTOSCROLL,
			(DrawInfo.fAutoScroll ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_WAVE_FOLLOW_ORIGNAL,
			(DrawInfo.fFollowByOrignal ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_WAVE_ORIGNAL_SIGNAL_SHOW,
			(DrawInfo.bOrignalSignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_WAVE_FILTTED_SIGNAL_SHOW,
			(DrawInfo.bFilttedSignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		DrawInfo.uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);

		MainInit();
		RestoreValue();
		clsAnalyze.Init_Params();
		
		CaculateHScroll();
		CaculateVScroll();
	
		m_audioWin = new CWinAudio();
		m_filttedWin = new CWinFiltted();
		m_FilterWin = new CWinFilter();
		m_FilterWin->cFilter = &clsMainFilterI;

		hWndRebar = MakeToolsBar();

		DbgMsg("CWinMain WM_CREATE\r\n");
		break;
	case WM_TIMER:
	{
		//AdcDataI->GeneratorWave();
		if (AdcDataQ->GetNew)
		{
			CaculateHScroll();
			AdcDataQ->GetNew = false;
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
	}
	break;
	case WM_SIZE:
	{
		GetClientRect(hWndRebar, &RebarRect);
		CaculateHScroll();
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
	}
	break;
	case WM_NOTIFY:
		clsWinTools.DoNotify(hWnd, message, wParam, lParam);
	break;
	case WM_COMMAND:
		OnCommand(message, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;
	case WM_PAINT:
		Paint();
		break;
	case WM_CLOSE:
	{
		int res = ::MessageBox(NULL, "[WM_CLOSE] 是否确定退出", "提示", MB_OKCANCEL);
		if (res == IDOK) { //点击 【确定】按钮
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		else if (res == IDCANCEL) {//点击【取消】按钮，不调用默认的消息处理 DefWindowProc 
		}
		else {
		}
	}
	break;
	case WM_QUIT:
	{
		::MessageBox(NULL, "WM_QUIT", "提示", MB_OK);
	}
	break;
	case WM_DESTROY:
		Program_In_Process = false;
		while (clsMainFilterI.Thread_Exit == false);
		while (clsAudioFilter.Thread_Exit == false);

		clsGetDataSDR.close_SDR_device();
		
		if (clsWinSDR.hWnd) 
			DestroyWindow(clsWinSDR.hWnd);
		if (clsWinSpect.hWnd) 
			DestroyWindow(clsWinSpect.hWnd);
		if (m_FilterWin->hWnd) 
			DestroyWindow(m_FilterWin->hWnd);
		if (m_audioWin->hWnd) 
			DestroyWindow(m_audioWin->hWnd);
		if (m_filttedWin->hWnd)
			DestroyWindow(m_filttedWin->hWnd);

		SaveValue();
		clsMainFilterI.SaveValue();
		clsMainFilterQ.SaveValue();
		clsAudioFilter.SaveValue();

		DbgMsg("Main WM_DESTROY\r\n");
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
		KeyAndScroll(message, wParam, lParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

VOID CWinMain::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
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

VOID CWinMain::Paint(void)
{
	HDC		hDC;
	PAINTSTRUCT ps;
	RECT	rt, r;
	HPEN	hPen, hPenLighter;

	hDC = BeginPaint(hWnd, &ps);

	GetClientRect(hWnd, &rt);
	rt.bottom -= RebarRect.bottom;

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

	UINT h_pos = DrawInfo.iHZoom > 0
		? DrawInfo.dwHZoomedPos >> DrawInfo.iHZoom
		: DrawInfo.dwHZoomedPos << (-DrawInfo.iHZoom);

	if (DrawInfo.bOrignalSignalShow) {
		DrawSignal_short(AdcDataI, h_pos, 0, hdc, dwXOffSet, bufStep, dwWStep, &rt);
		DrawSignal_short(AdcDataQ, h_pos, 1, hdc, dwXOffSet, bufStep, dwWStep, &rt);
	}

	if (DrawInfo.bFilttedSignalShow) {
		DrawSignal_float(AdcDataIFiltted, h_pos, 2, hdc, dwXOffSet, bufStep, dwWStep, &rt);
		DrawSignal_float(AdcDataQFiltted, h_pos, 3, hdc, dwXOffSet, bufStep, dwWStep, &rt);
	}

	//int InfolastPos;
	//double vzoom = DrawInfo.dbVZoom;// DrawInfo.iVZoom >= 0 ? (1 << DrawInfo.iVZoom) : (1.0 / (1 << (-DrawInfo.iVZoom)));
	//HPEN hPenSaved = NULL;
	//if (DrawInfo.bOrignalSignalShow) {
	//	//绘制原信号--------------------------------------------------
	//	hPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
	//	//SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
	//	hPenSaved = (HPEN)SelectObject(hdc, hPen);//(HPEN)GetStockObject(WHITE_PEN));
	//	x = WAVE_RECT_BORDER_LEFT - dwXOffSet;
	//	PADC_DATA_TYPE pData = (PADC_DATA_TYPE)AdcDataI->Buff +
	//		(	DrawInfo.iHZoom > 0
	//			? DrawInfo.dwHZoomedPos >> DrawInfo.iHZoom
	//			: DrawInfo.dwHZoomedPos << (-DrawInfo.iHZoom)	);
	//	
	//	y = DrawInfo.dwVZoomedHeight / 2 - (*pData * vzoom) - DrawInfo.dwVZoomedPos;
	//	y = BOUND(y, 0, WAVE_RECT_HEIGHT);
	//	//DbgMsg("zoom:%f, v:%d, zoomv: %d\r\n", vzoom, *pData, y);
	//	InfolastPos = -1;
	//	MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
	//	do
	//	{
	//		pData += bufStep;
	//		x += dwWStep;
	//		if (pData == &((PADC_DATA_TYPE)AdcDataI->Buff)[AdcDataI->Pos & AdcDataI->Mask])InfolastPos = x;
	//		y = DrawInfo.dwVZoomedHeight / 2 - (*pData * vzoom) - DrawInfo.dwVZoomedPos;
	//		y = BOUND(y, 0, WAVE_RECT_HEIGHT);
	//		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + y);
	//	} while (x < rt.right - WAVE_RECT_BORDER_RIGHT);
	//	if (InfolastPos >= 0)
	//	{
	//		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
	//		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
	//	}
	//	SelectObject(hdc, hPenSaved);
	//	DeleteObject(hPen);
	//}

	{
		//绘制信号MASKS--------------------------------------------------
		hPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
		HPEN hPenSaved = (HPEN)SelectObject(hdc, hPen);//(HPEN)GetStockObject(WHITE_PEN));
		x = WAVE_RECT_BORDER_LEFT - dwXOffSet;
		char* pData = AdcBuffMarks +
			(DrawInfo.iHZoom > 0
				? DrawInfo.dwHZoomedPos >> DrawInfo.iHZoom
				: DrawInfo.dwHZoomedPos << (-DrawInfo.iHZoom));
		do
		{
			x += dwWStep;
			if (*pData == 1) {
				MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
				LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT / 4 * 3);
			}
			else if(*pData == 2){
				MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT / 4, NULL);
				LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
			}
			pData += bufStep;
		} while (x < rt.right - WAVE_RECT_BORDER_RIGHT);
		SelectObject(hdc, hPenSaved);
		DeleteObject(hPen);
	}

	//if (DrawInfo.bFilttedSignalShow) {
	//	//绘制滤波信号--------------------------------------------------
	//	hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
	//	//SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
	//	SelectObject(hdc, hPen);//(HPEN)GetStockObject(WHITE_PEN));
	//	x = WAVE_RECT_BORDER_LEFT - dwXOffSet;
	//	FILTTED_DATA_TYPE* pFilterData = (FILTTED_DATA_TYPE*)AdcDataIFiltted->Buff +
	//		(
	//			DrawInfo.iHZoom > 0
	//			? DrawInfo.dwHZoomedPos >> DrawInfo.iHZoom
	//			: DrawInfo.dwHZoomedPos << (-DrawInfo.iHZoom)
	//			);
	//	y = DrawInfo.dwVZoomedHeight / 2 - (*pFilterData * vzoom) - DrawInfo.dwVZoomedPos;
	//	y = BOUND(y, 0, WAVE_RECT_HEIGHT);
	//	MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
	//	InfolastPos = -1;
	//	do
	//	{
	//		pFilterData += bufStep;
	//		x += dwWStep;
	//		y = DrawInfo.dwVZoomedHeight / 2 - (*pFilterData * vzoom) - DrawInfo.dwVZoomedPos;
	//		y = BOUND(y, 0, WAVE_RECT_HEIGHT);
	//		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + y);
	//		if (pFilterData == &((FILTTED_DATA_TYPE*)AdcDataIFiltted->Buff)[AdcDataIFiltted->Pos & AdcDataIFiltted->Mask]) {
	//			MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
	//			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
	//			MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
	//			//InfolastPos = x;
	//		}
	//	} while (x < rt.right - WAVE_RECT_BORDER_RIGHT);
	//	
	//	/*
	//	if (InfolastPos >= 0)
	//	{
	//		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
	//		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
	//	}
	//	*/
	//	DeleteObject(hPen);
	//}

	//---------------------------------------

	double z = DrawInfo.iVZoom >= 0 ? (1.0 / ((UINT64)1 << DrawInfo.iVZoom)) : ((UINT64)1 << -DrawInfo.iVZoom);
	char tstr1[100], tstr2[100], tstradcpos[100], tstrfiltpos[100], tstrfiltbuffpos[100], tstrfftpos[100];
	//AdcDataI->NumPerSec = 38400;
	double TimePreDiv = 32.0 / AdcDataI->SampleRate * 
		( DrawInfo.iHZoom >= 0 ? 1.0 / ((UINT64)1 << DrawInfo.iHZoom) : ((UINT64)1 << -DrawInfo.iHZoom));
	sprintf(s, "32 / DIV    %sV / DIV    %ss / DIV 中文\r\n"\
		"Pos:%s\r\nFilttedPos:%s\r\nFilttedBuffPos=%s\r\n"\
		"hBarPos: %d\r\n"\
		"AdcSampleRate: %d    Real AdcSampleRate: %d\r\n"\
		"HZoom: %d, %d \t VZoom: %d, %d",
		formatKKDouble(32.0 * DrawInfo.VotagePerDIV * z, "", tstr1),
		formatKKDouble(TimePreDiv, "", tstr2),
		fomatKINT64(AdcDataI->Pos, tstradcpos),
		fomatKINT64(AdcDataI->ProcessPos, tstrfiltpos),
		fomatKINT64(AdcDataIFiltted->Pos, tstrfiltbuffpos),
		DrawInfo.dwHZoomedPos,
		AdcDataI->SampleRate, AdcDataI->NumPerSec,
		DrawInfo.iHZoom, DrawInfo.iHZoom >= 0 ? 1 << DrawInfo.iHZoom : -(1 << -DrawInfo.iHZoom),
		DrawInfo.iVZoom, DrawInfo.iVZoom >= 0 ? 1 << DrawInfo.iVZoom : -(1 << -DrawInfo.iVZoom)
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

	//sprintf(s,"vzoom %.40f",vzoom);
	//r.top += FONT_HEIGHT;
	//DrawText(hdc, s, strlen(s), &r, NULL);

	DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

	BitBlt(hDC, 
           0, RebarRect.bottom,
           rt.right, rt.bottom,
           hdc, 
           0, 0, 
           SRCCOPY);
	DeleteObject(hdc);
	DeleteObject(hbmp);

	EndPaint(hWnd, &ps);
}

VOID CWinMain::GetRealClientRect(PRECT lprc)
{
    DWORD dwStyle;

    dwStyle = GetWindowLong (hWnd, GWL_STYLE);
    GetClientRect (hWnd, lprc);

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

	GetRealClientRect(&rc);
	DrawInfo.dwDataWidth = AdcDataI->Len;//  AdcDataI->Pos;
	DrawInfo.dwHZoomedWidth = DrawInfo.dwDataWidth;
	if (DrawInfo.iHZoom > 0)
	{
		for (i = 0; i < DrawInfo.iHZoom; i++)
		{							     
			if (DrawInfo.dwHZoomedWidth & (AdcDataI->Len << 5)) // 最大放大32倍
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
		DrawInfo.dwHZoomedPos = (DrawInfo.fFollowByOrignal == true ? AdcDataI->Pos : AdcDataIFiltted->Pos) * DrawInfo.dbHZoom -
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
	SetScrollRange(hWnd, SB_HORZ, 0, iRangeH, FALSE);
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
	//UINT64	maxDataHeight = (UINT64)1 << (AdcDataI->DataBits + ADC_DATA_MAX_ZOOM_BIT);
	DrawInfo.dwVZoomedHeight = MoveBits(MoveBits(1, AdcDataI->DataBits), DrawInfo.iVZoom);
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

BOOL CWinMain::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
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
		//clsSoundCard.ClearNoise3();
		//clsSoundCard.MySound(NULL);
		break;
	case IDM_ANALYZECLEARNOISE:
		//clsSoundCard.ClearNoise2();
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
		//clsFile.SaveFile();
		clsFile.SaveBuffToFile();
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
		CheckMenuItem(hMenu, IDM_WAVEAUTOSCROLL, 
			(DrawInfo.fAutoScroll ? MF_CHECKED : MF_UNCHECKED ) | MF_BYCOMMAND);
		break;
	case IDM_WAVE_FOLLOW_ORIGNAL:
		DrawInfo.fFollowByOrignal = !DrawInfo.fFollowByOrignal;
		CheckMenuItem(hMenu, IDM_WAVE_FOLLOW_ORIGNAL,
			(DrawInfo.fFollowByOrignal ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;

//COMMANDS-----------------------
	case IDM_SPECTRUMWINDOW:
		clsWinSpect.OpenWindow();
		break;
	case IDM_FILTTEDWINDOW:
		m_filttedWin->OpenWindow();
		break;
	case IDM_SDR_WINDOW:
		clsWinSDR.OpenWindow();
		break;
	case IDM_AUDIO_WINDOW:
		m_audioWin->OpenWindow();
		break;
	case IDM_FILTERWINDOW:
		m_FilterWin->OpenWindow();
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
		CheckMenuItem(hMenu, IDM_WAVE_ORIGNAL_SIGNAL_SHOW,
			(DrawInfo.bOrignalSignalShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		break;
	case IDM_WAVE_FILTTED_SIGNAL_SHOW:
		DrawInfo.bFilttedSignalShow = !DrawInfo.bFilttedSignalShow;
		CheckMenuItem(hMenu, IDM_WAVE_FILTTED_SIGNAL_SHOW,
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

void CWinMain::SaveValue(void)
{
	WritePrivateProfileString("WIN_MAIN", "iHZoom", std::to_string(DrawInfo.iHZoom).c_str(), IniFilePath);
	WritePrivateProfileString("WIN_MAIN", "iVZoom", std::to_string(DrawInfo.iVZoom).c_str(), IniFilePath);
	WritePrivateProfileString("WIN_MAIN", "iVOldZoom", std::to_string(DrawInfo.iVOldZoom).c_str(), IniFilePath);
	WritePrivateProfileString("WIN_MAIN", "dwVZoomedPos", std::to_string(DrawInfo.dwVZoomedPos).c_str(), IniFilePath);
}

void CWinMain::RestoreValue(void)
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

static HPEN hPens[4] =
{
	CreatePen(PS_SOLID, 1, RGB(0,255,0)),
	CreatePen(PS_SOLID, 1, RGB(0, 255, 128)),
	CreatePen(PS_SOLID, 1, RGB(255, 255, 0)),
	CreatePen(PS_SOLID, 1, RGB(128, 255, 0))
};

void CWinMain::DrawSignal_short(CData* cData, UINT pos, UINT pen, HDC hdc, UINT dwXOffSet, UINT bufStep, UINT dwWStep, RECT* rt)
{
	int InfolastPos = -1;
	double vzoom = DrawInfo.dbVZoom;
	HPEN hPenSaved = (HPEN)SelectObject(hdc, hPens[pen]);//(HPEN)GetStockObject(WHITE_PEN));
	int x, y;
	//SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
	x = WAVE_RECT_BORDER_LEFT - dwXOffSet;
	short* buff = (short*)cData->Buff;
	short* pData = buff + pos;
	y = DrawInfo.dwVZoomedHeight / 2 - (*pData * vzoom) - DrawInfo.dwVZoomedPos;
	y = BOUND(y, 0, WAVE_RECT_HEIGHT);
	MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
	do
	{
		pData += bufStep;
		x += dwWStep;
		if (pData == &buff[cData->Pos & cData->Mask])InfolastPos = x;
		y = DrawInfo.dwVZoomedHeight / 2 - (*pData * vzoom) - DrawInfo.dwVZoomedPos;
		y = BOUND(y, 0, WAVE_RECT_HEIGHT);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + y);
	} while (x < rt->right - WAVE_RECT_BORDER_RIGHT);
	if (InfolastPos >= 0)
	{
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
	}
	SelectObject(hdc, hPenSaved);
}

void CWinMain::DrawSignal_float(CData* cData, UINT pos, UINT pen, HDC hdc, UINT dwXOffSet, UINT bufStep, UINT dwWStep, RECT* rt)
{
	int InfolastPos;
	double vzoom = DrawInfo.dbVZoom;// DrawInfo.iVZoom >= 0 ? (1 << DrawInfo.iVZoom) : (1.0 / (1 << (-DrawInfo.iVZoom)));
	HPEN 	hPenSaved = (HPEN)SelectObject(hdc, hPens[pen]);//(HPEN)GetStockObject(WHITE_PEN));
	int x, y;
	//SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));

	x = WAVE_RECT_BORDER_LEFT - dwXOffSet;
	float* buff = (float*)cData->Buff;
	float* pData = (float*)buff + pos;
	y = DrawInfo.dwVZoomedHeight / 2 - (*pData * vzoom) - DrawInfo.dwVZoomedPos;
	y = BOUND(y, 0, WAVE_RECT_HEIGHT);
	//DbgMsg("zoom:%f, v:%d, zoomv: %d\r\n", vzoom, *pData, y);
	InfolastPos = -1;
	MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
	do
	{
		pData += bufStep;
		x += dwWStep;
		if (pData == &buff[cData->Pos & cData->Mask])InfolastPos = x;
		y = DrawInfo.dwVZoomedHeight / 2 - (*pData * vzoom) - DrawInfo.dwVZoomedPos;
		y = BOUND(y, 0, WAVE_RECT_HEIGHT);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + y);
	} while (x < rt->right - WAVE_RECT_BORDER_RIGHT);
	if (InfolastPos >= 0)
	{
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT);
	}
	SelectObject(hdc, hPenSaved);
}

HWND CWinMain::MakeToolsBar(void)
{
	HWND hWndRebar = clsWinTools.CreateRebar(hWnd);
	static TBBUTTON tbb[8] = {
		{ MAKELONG(1, 0), IDM_AUDIO_WINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"Audio" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, 
		{ MAKELONG(2, 0), IDM_SPECTRUMWINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE,	{0}, 0, (INT_PTR)L"Spectrum" },
		{ MAKELONG(3, 0), IDM_FILTERWINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"Filter" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, 
		{ MAKELONG(4, 0), IDM_SDR_WINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"SDR" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP, {0}, 0, NULL }, 
		{ MAKELONG(0, 0), 0, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN,	{0}, 0, (INT_PTR)L"下拉" }
	};
	CWinTools::TOOL_TIPS tips[8] = {
		{ IDM_STARTPLAY,"打开音频信号窗口." },
		{ 0, NULL }, // Separator
		{ IDM_DEMODULATOR_AM,"打开信号频谱窗口." },
		{ IDM_DEMODULATOR_FM, "打开滤波器设计窗口." },
		{ 0, NULL }, // Separator
		{ IDM_DEMODULATOR_FM, "打开SDR设备窗口." },
		{ 0, NULL }, // Separator
		{ 0, "TBSTYLE_DROPDOWN" }
	};
	HWND hToolBar = clsWinTools.CreateToolbar(hWnd, tbb, 8, tips, 8);

	// Add images
	TBADDBITMAP tbAddBmp = { 0 };
	tbAddBmp.hInst = HINST_COMMCTRL;
	tbAddBmp.nID = IDB_STD_SMALL_COLOR;
	SendMessage(hToolBar, TB_ADDBITMAP, 0, (WPARAM)&tbAddBmp);

	clsWinTools.CreateRebarBand(hWndRebar, "窗口", 1, 500, 0, hToolBar);

	//hWndTrack = CreateTrackbar(hWnd, 0, 100, 10);
	clsWinTools.CreateRebarBand(hWndRebar, "Value", 2, 0, 0, NULL);
	return hWndRebar;
}
