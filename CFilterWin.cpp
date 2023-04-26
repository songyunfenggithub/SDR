#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "myDebug.h"
#include "CSoundCard.h"
#include "CData.h"
#include "CWaveFFT.h"
#include "CWaveFilter.h"
#include "CFilterWin.h"
#include "CDemodulatorAM.h"

using namespace std;

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define WAVE_RECT_HEIGHT				0X200
#define WAVE_RECT_BORDER_TOP		100
#define WAVE_RECT_BORDER_LEFT		50
#define WAVE_RECT_BORDER_RIGHT		60
#define WAVE_RECT_BORDER_BOTTON		0

#define DIVLONG		10
#define DIVSHORT	5

#define TIMEOUT		500

#define BRUSH_BACKGROUND		BLACK_BRUSH

#define COLOR_BORDER_THICK		RGB(64, 64, 64)
#define COLOR_BORDER_THIN		RGB(128, 128, 128)
#define COLOR_BACKGROUND		RGB(128, 128, 128)

#define COLOR_FILTER_CORE		RGB(0, 255, 0)  
#define COLOR_FFT				RGB(255, 255, 0)
#define COLOR_FFT_LOG			RGB(0, 255, 255)

#define COLOR_TEXT_BACKGOUND	RGB(0, 0, 0)
#define COLOR_TEXT				RGB(255, 255, 255)

const char filterComment[] = "滤波器长度,  滤波器迭代层数,  x;  滤波器类型,  中心频率,  有效宽度;  ...\n"\
"example: 1023, 2, 0; 1, 50, 10\n"\
"滤波器长度 必须是奇数.\n"\
"滤波器迭代层数 0 不迭代; 1 - 3 迭代层数.\n"\
"x 无效设置，任意数.\n"\
"滤波器类型\n"\
"  0 指定为低通滤波器.\n"\
"  1 指定为高通滤波器.\n"\
"  2 指定为带通滤波器.\n"\
"  3 指定为阻带滤波器.\n"\
"中心频率\n"\
"有效宽度\n";

CFilterWin::CFilterWin()
{
	hCoreAnalyseMutex = CreateMutex(NULL, false, "hCoreAnalyseMutex");		//创建互斥对象

	//pFilterInfo = &clsWaveFilter.rootFilterInfo;
	RegisterWindowsClass();
}

CFilterWin::~CFilterWin()
{

}

void CFilterWin::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CFilterWin::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_FILTER;
	wcex.lpszClassName = FILTER_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CFilterWin::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(FILTER_WIN_CLASS, "Filter windows", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 1400, 800, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

LRESULT CALLBACK CFilterWin::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CFilterWin* me = (CFilterWin*)get_WinClass(hWnd);	
	switch (message)
	{
	case WM_MOUSEMOVE:
	{
		if (me->CoreAnalyseFFTBuff == NULL) break;
		HDC hDC = GetDC(hWnd);
		PAINTSTRUCT ps;
		RECT r, rt;
		HPEN hPen = CreatePen(PS_DOT, 0, RGB(64, 64, 64));
		GetClientRect(hWnd, &rt);
		SetBkColor(hDC, COLOR_TEXT_BACKGOUND);

		SelectObject(hDC, CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
			DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));

		SetBkMode(hDC, TRANSPARENT);
		//	SetBkMode(hdc, OPAQUE); 
		//	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);
		r.top		= WAVE_RECT_BORDER_TOP - DIVLONG - 40;
		r.left		= WAVE_RECT_BORDER_LEFT;
		r.right		= r.left + 700;
		r.bottom	= r.top + 20;
		FillRect(hDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		int i = 0;
		char s[500];
		FILTER_CORE_DATA_TYPE* pFilterCore = me->pFilterInfo->FilterCore;
		int FilterLength = me->pFilterInfo->CoreLength;
		int X = (me->HScrollPos + xPos - WAVE_RECT_BORDER_LEFT) / me->HScrollZoom;
		X = BOUND(X, 0, (me->HScrollPos + rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / me->HScrollZoom);
		FILTER_CORE_DATA_TYPE Y = X > FilterLength ? 0 : pFilterCore[X];
		sprintf(s, "X: %d, core V: %lf", X, Y);
		SetTextColor(hDC, COLOR_FILTER_CORE);
		DrawText(hDC, s, strlen(s), &r, NULL);
		//TextOut(hDC,0,0, s,	strlen(s));

		r.left += 200;
		SetTextColor(hDC, COLOR_FFT);
		FILTER_CORE_DATA_TYPE Hz = X * (clsData.AdcSampleRate / (1 << me->pFilterInfo->decimationFactorBit)) / me->CoreAnalyseFFTLength;
		Y = X > me->CoreAnalyseFFTLength / 2 ? 0 : me->CoreAnalyseFFTBuff[X];
		sprintf(s, "Hz: %.03f, FFT: %lf", Hz, Y);
		DrawText(hDC, s, strlen(s), &r, NULL);

		r.left += 250;
		SetTextColor(hDC, COLOR_FFT_LOG);
		int Ypos = BOUND(yPos - WAVE_RECT_BORDER_TOP, 0, WAVE_RECT_HEIGHT);
		double Ylog = 20 * (X > me->CoreAnalyseFFTLength/2 ? 0 : me->CoreAnalyseFFTLogBuff[X]);
		sprintf(s, "Y: %.03fdb, logFFT: %fdb", -(double)Ypos / 64 * 20, Ylog);
		DrawText(hDC, s, strlen(s), &r, NULL);

		DeletePen(hPen);

		DeleteObject(SelectObject(hDC, GetStockObject(SYSTEM_FONT)));

		ReleaseDC(hWnd, hDC);
	}
	break;
	case WM_CREATE:
	{
		OPENCONSOLE;
		me = (CFilterWin*)set_WinClass(hWnd, lParam);

		me->hWnd = hWnd;
		me->hMenuMain = GetMenu(hWnd);
		CheckMenuItem(me->hMenuMain, IDM_FILTER_CORE_SHOW,
			(me->filterCoreShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(me->hMenuMain, IDM_FILTER_CORE_SPECTRUM_SHOW,
			(me->filterCoreSpectrumShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(me->hMenuMain, IDM_FILTER_CORE_SPECTRUM_LOG_SHOW,
			(me->filterCoreSpectrumLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		me->hMenuFilterItems = CreateMenu();
		AppendMenu(me->hMenuMain, MF_POPUP, (UINT_PTR)me->hMenuFilterItems, "滤波器");
		me->set_CoreAnalyse_root_Filter(&clsWaveFilter.rootFilterInfo);
	}
	break;
	case WM_TIMER:
		break;
	case WM_SIZE:
		//CacheInit(hWnd);
		break;
	case WM_COMMAND:
		me->OnCommand(message, wParam, lParam);
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
		DbgMsg("CFilterWin WM_DESTROY\r\n");
		me->hWnd = NULL;
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

void CFilterWin::set_CoreAnalyse_root_Filter(CWaveFilter::PFILTERINFO pFilterInfo)
{
	rootFilterInfo = pFilterInfo;

	char s[100];
	int i = 0;

	i = GetMenuItemCount(hMenuFilterItems);
	i--;
	while(i >= 0)DeleteMenu(hMenuFilterItems, i--, MF_BYPOSITION);

	HMENU hMENUItem;
	i = 0;
	CWaveFilter::PFILTERINFO pfi = rootFilterInfo->nextFilter;
	while (pfi != NULL)
	{
		hMENUItem = CreateMenu();
		sprintf(s, "滤波器 %d", i);
		AppendMenu(hMenuFilterItems, MF_STRING, i, s);
		i++;
		pfi = pfi->nextFilter;
	}

	hMENUItem = CreateMenu();
	AppendMenu(hMenuFilterItems, MF_STRING, i, "总滤波器");

	InitFilterCoreAnalyse(pFilterInfo);
}

void CFilterWin::InitFilterCoreAnalyse(CWaveFilter::PFILTERINFO pFilterInfo)
{
	this->pFilterInfo = pFilterInfo;
	int index = pFilterInfo == rootFilterInfo ? rootFilterInfo->subFilterNum : pFilterInfo->subFilteindex;
	CheckMenuRadioItem(hMenuFilterItems, 0, rootFilterInfo->subFilterNum, index, MF_BYPOSITION);
	CoreAnalyse.pFilterWin = this;
	CoreAnalyse.pFilterInfo = pFilterInfo;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFilterWin::FilterCoreAnalyse, &CoreAnalyse, 0, NULL);
}

LPTHREAD_START_ROUTINE CFilterWin::FilterCoreAnalyse(LPVOID lp)
{
	CORE_ANALYSE_DATA *CoreAnalyse = (CORE_ANALYSE_DATA*) lp;
	CFilterWin* pFilterWin = CoreAnalyse->pFilterWin;
	CWaveFilter::PFILTERINFO pFilterInfo = CoreAnalyse->pFilterInfo;

	clsWaveFilter.FilterCoreAnalyse(pFilterWin, pFilterInfo);

	pFilterWin->HOriginalWidth = (pFilterInfo->CoreLength > pFilterWin->CoreAnalyseFFTLength ? pFilterInfo->CoreLength : pFilterWin->CoreAnalyseFFTLength);
	pFilterWin->HScrollWidth = pFilterWin->HScrollZoom * pFilterWin->HOriginalWidth;
	RECT rc;
	pFilterWin->GetRealClientRect(&rc);
	pFilterWin->HScrollWidth -= rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	if (pFilterWin->HScrollWidth < 0) pFilterWin->HScrollWidth = 0;
	SetScrollRange(pFilterWin->hWnd, SB_HORZ, 0, pFilterWin->HScrollWidth, TRUE);

	InvalidateRect(pFilterWin->hWnd, NULL, true);
	
	return 0;
}

bool CFilterWin::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);
	RECT rc;

	if (wmId >= 0 && wmId <= rootFilterInfo->subFilterNum) {
		CWaveFilter::PFILTERINFO p;
		if (rootFilterInfo->subFilterNum == wmId) p = rootFilterInfo;
		else {
			p = rootFilterInfo->nextFilter;
			while (p != NULL)
			{
				if (p->subFilteindex == wmId) {
					break;
				}
				p = p->nextFilter;
			}
		}
		if(p != NULL)InitFilterCoreAnalyse(p);
	}
	
	switch (wmId)
	{
		//Setting COMMANDS-----------------------
	case IDM_SPECTRUM_PAUSE_BREAK:
		break;

	case IDM_FILTER_CORE_SETTING:
		DialogBoxParam(hInst, (LPCTSTR)IDD_DLG_FILTER_CORE_SET, hWnd, (DLGPROC)DlgFilterCoreProc, (LPARAM)this);
		break;
	case IDM_FILTER_ZOOM_INC:
		if (HScrollZoom < 16) HScrollZoom *= 2;
		GetRealClientRect(&rc);
		SetScrollRange(hWnd, SB_HORZ, 0, (HScrollWidth = HScrollZoom * HOriginalWidth - rc.right) > 0 ? HScrollWidth : 0, TRUE);
		InvalidateRect(hWnd, NULL, true);
		break;
	case IDM_FILTER_ZOOM_DEC:
		GetRealClientRect(&rc);
		if (HScrollZoom > 1.5) HScrollZoom /= 2;
		else {
			if (HScrollZoom * HOriginalWidth > rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_LEFT) HScrollZoom /= 2;
		}
		SetScrollRange(hWnd, SB_HORZ, 0, (HScrollWidth = HScrollZoom * HOriginalWidth - rc.right) > 0 ? HScrollWidth : 0, TRUE);
		InvalidateRect(hWnd, NULL, true);
		break;
	case IDM_FILTER_ZOOM_HOME:
		HScrollZoom = 1.0;
		GetRealClientRect(&rc);
		SetScrollRange(hWnd, SB_HORZ, 0, (HScrollWidth = HScrollZoom * HOriginalWidth - rc.right) > 0 ? HScrollWidth : 0, TRUE);
		InvalidateRect(hWnd, NULL, true);
		break;
	case IDM_FILTER_CORE_SHOW:
		filterCoreShow = !filterCoreShow;
		CheckMenuItem(hMenuMain, IDM_FILTER_CORE_SHOW,
			(filterCoreShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;
	case IDM_FILTER_CORE_SPECTRUM_SHOW:
		filterCoreSpectrumShow = !filterCoreSpectrumShow;
		CheckMenuItem(hMenuMain, IDM_FILTER_CORE_SPECTRUM_SHOW,
			(filterCoreSpectrumShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;
	case IDM_FILTER_CORE_SPECTRUM_LOG_SHOW:
		filterCoreSpectrumLogShow = !filterCoreSpectrumLogShow;
		CheckMenuItem(hMenuMain, IDM_FILTER_CORE_SPECTRUM_LOG_SHOW,
			(filterCoreSpectrumLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

VOID CFilterWin::Paint(void)
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
	SelectObject(hdc, CreateFont(14, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));
	FillRect(hdc, &rt, (HBRUSH)GetStockObject(BRUSH_BACKGROUND));

	int	x, y, pos, i;
	TCHAR	s[2000];
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
				sprintf(s, "%.02fhz", (double)(i * 32 + HScrollPos) / HScrollZoom * 
					(clsData.AdcSampleRate / (1 << pFilterInfo->decimationFactorBit)) / CoreAnalyseFFTLength);
				r.top = WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT + DIVLONG;
				r.left = x;
				SetTextColor(hdc, COLOR_FFT);
				DrawText(hdc, s, strlen(s), &r, NULL);
				sprintf(s, "%d", (int)((i * 32 + HScrollPos) / HScrollZoom));
				r.top = WAVE_RECT_BORDER_TOP - DIVLONG - 16;
				r.left = x;
				SetTextColor(hdc, COLOR_FILTER_CORE);
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
	SelectObject(hdc,  hPen);
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
				SetTextColor(hdc, COLOR_FFT_LOG);
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

	//绘制信号--------------------------------------------------
	if (pFilterInfo != NULL) {
		FILTER_CORE_DATA_TYPE* pFilterCore = pFilterInfo->FilterCore;
		if (pFilterCore) {
			int istep = HScrollZoom < 1 ? 1 / HScrollZoom : 1;
			int Xstep = HScrollZoom > 1 ? HScrollZoom : 1;
			double scale;
			int X, Y;
			//绘制滤波器核--------------------------------------------------
			if (filterCoreShow == true) {
				int CoreLength = pFilterInfo->CoreLength;
				Y = 0;
				FILTER_CORE_DATA_TYPE CoreCenter = 256;
				FILTER_CORE_DATA_TYPE FilterCoreMaxValue = 0.0;
				for (i = 0; i < CoreLength; i++) if (FilterCoreMaxValue < abs(pFilterCore[i])) FilterCoreMaxValue = abs(pFilterCore[i]);
				//FilterCoreMaxValue = 1.0;
				scale = CoreCenter / FilterCoreMaxValue;
				i = HScrollPos / HScrollZoom;
				if (i < CoreLength) Y = CoreCenter - pFilterCore[i] * scale;
				X = WAVE_RECT_BORDER_LEFT;
				MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
				hPen = CreatePen(PS_SOLID, 1, COLOR_FILTER_CORE);
				SelectObject(hdc, hPen);
				for (i += istep; i < CoreLength && (i * HScrollZoom - HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
				{
					X += Xstep;
					Y = CoreCenter - pFilterCore[i] * scale;
					LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
				}
				DeleteObject(hPen);
			}

			WaitForSingleObject(hCoreAnalyseMutex, INFINITE);
			//绘制滤波 FFT 信号--------------------------------------------------
			if (CoreAnalyseFFTBuff)
			{
				int FFTLength = CoreAnalyseFFTLength / 2;
				if (filterCoreSpectrumShow == true) {
					double* pFFTBuf = CoreAnalyseFFTBuff;

					double fftvmax = pFFTBuf[FFTLength];
					scale = WAVE_RECT_HEIGHT;// / fftvmax;
					i = HScrollPos / HScrollZoom;
					if (i < FFTLength)
						Y = WAVE_RECT_HEIGHT - pFFTBuf[i] * scale;
					Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
					X = WAVE_RECT_BORDER_LEFT;
					MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
					hPen = CreatePen(PS_SOLID, 1, COLOR_FFT);
					SelectObject(hdc, hPen);
					for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
					{
						X += Xstep;
						Y = WAVE_RECT_HEIGHT - pFFTBuf[i] * scale;
						Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
						LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
					}
					DeleteObject(hPen);
				}

				if (filterCoreSpectrumLogShow == true) {
					//绘制滤波 FFT Log10 信号--------------------------------------------------
					double* pFFTLogBuf = CoreAnalyseFFTLogBuff;
					double fftlogmax = pFFTLogBuf[FFTLength];
					double fftlogmin = pFFTLogBuf[FFTLength + 1];
					scale = WAVE_RECT_HEIGHT;// / fftlogmin;
					i = HScrollPos / HScrollZoom;
					if (i < FFTLength)
						Y = pFFTLogBuf[i] * -64;
					//Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
					X = WAVE_RECT_BORDER_LEFT;
					MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
					hPen = CreatePen(PS_SOLID, 1, COLOR_FFT_LOG);
					SelectObject(hdc, hPen);
					for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
					{
						X += Xstep;
						Y = pFFTLogBuf[i] * -64;
						//Y = BOUND(Y, 0, WAVE_RECT_HEIGHT);
						LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
					}
					DeleteObject(hPen);
				}
			}
			ReleaseMutex(hCoreAnalyseMutex);
		}

		//---------------------------------------

		double FullVotage = 5.0;
		UINT64 a64 = 1;
		double VotagePerDIV = (FullVotage / (unsigned __int64)(a64 << (sizeof(ADCDATATYPE) * 8)));
		a64 = 1;
		char tstr1[100], tstr2[100];
		a64 = 1;
		r.top = WAVE_RECT_HEIGHT + WAVE_RECT_BORDER_TOP + DIVLONG + 20;
		r.left = WAVE_RECT_BORDER_TOP;
		r.right = rt.right;
		r.bottom = rt.bottom;
		sprintf(s, "32pix / DIV\r\n"\
			"AdcSampleRate: %d\r\n"\
			"Core Length: %d\r\n"\
			"decimationFactorBit: %d, SampleRate: %d, RealSampleRate: %d\r\n"\
			"FreqFallWidth: %.3f\r\n"\
			"CoreNum: %d CoreIndex:%d\r\n"\
			"Core Desc: %s\r\n"\
			"HZoom: %f",
			clsData.AdcSampleRate,
			pFilterInfo->CoreLength,
			pFilterInfo->decimationFactorBit, clsData.AdcSampleRate, clsData.AdcSampleRate / (1 << pFilterInfo->decimationFactorBit),
			pFilterInfo->FreqFallWidth,
			rootFilterInfo->subFilterNum, pFilterInfo->subFilteindex,
			rootFilterInfo->CoreDescStr,
			HScrollZoom
		);
		SetBkMode(hdc, TRANSPARENT);
		//	SetBkMode(hdc, OPAQUE); 
		//	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);

		SetTextColor(hdc, COLOR_TEXT);
		DrawText(hdc, s, strlen(s), &r, NULL);

		DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));
	}

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


VOID CFilterWin::GetRealClientRect(PRECT lprc)
{
	DWORD dwStyle;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	GetClientRect(hWnd, lprc);
	if (dwStyle & WS_HSCROLL)
		lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (dwStyle & WS_VSCROLL)
		lprc->right += GetSystemMetrics(SM_CXVSCROLL);
}

void CFilterWin::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
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
			printf("HScrollPos: %d, HScrollWidth: %d.\r\n", HScrollPos, HScrollWidth);
		}
		break;
	}

}

LRESULT CALLBACK CFilterWin::DlgFilterCoreProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CFilterWin* me = (CFilterWin*)get_DlgWinClass(hDlg);
	switch (message)
	{
	case WM_INITDIALOG:
		me = (CFilterWin*)set_DlgWinClass(hDlg, lParam);

		//DWORD dwPos = GetDlgItemInt(hDlg, IDC_EDITGOTO, 0, 0);
		//SetDlgItemInt(hDlg, IDC_EDIT_PLAY_STOP_POSITION,	clsSoundCard.dwPlayStopPosition, TRUE);
		SetDlgItemText(hDlg, IDC_EDIT1,	me->rootFilterInfo->CoreDescStr);
		SetDlgItemInt(hDlg, IDC_EDIT2, me->rootFilterInfo->decimationFactorBit, TRUE);
		CheckDlgButton(hDlg, IDC_CHECK1, TRUE);
		SetDlgItemText(hDlg, IDC_STATIC1, filterComment);
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if (LOWORD(wParam) == IDOK) 
			{
				char s[8192];
				GetDlgItemText(hDlg, IDC_EDIT1, s, FILTER_CORE_DESC_MAX_LENGTH);
				UINT decimationFactor = GetDlgItemInt(hDlg, IDC_EDIT2, NULL, false);
				//IsDlgButtonChecked(hDlg, IDC_CHECK1) == true ?
					//GetDlgItemInt(hDlg, IDC_EDIT2, NULL, TRUE) : clsData.AdcSampleRate;
				clsWaveFilter.rootFilterInfo.decimationFactorBit = decimationFactor;
				clsWaveFilter.rootFilterInfo.SampleRate = clsData.AdcSampleRate / (1 << clsWaveFilter.rootFilterInfo.decimationFactorBit);
				clsWaveFilter.setFilterCoreDesc(me->rootFilterInfo,s);
				clsWaveFilter.ParseCoreDesc(me->rootFilterInfo);
				me->set_CoreAnalyse_root_Filter(me->rootFilterInfo);
			}
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
