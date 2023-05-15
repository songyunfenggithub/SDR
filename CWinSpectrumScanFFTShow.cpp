
#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "Public.h"
#include "Debug.h"

#include "CSoundCard.h"
#include "CDataFromSDR.h"
#include "CData.h"
#include "CFFT.h"
#include "CFilter.h"

#include "CWinFFT.h"
#include "CWinSDR.h"
#include "CWinTools.h"
#include "CWinSpectrumScanFFTShow.h"


using namespace std;
using namespace WINS;
using namespace WINS::SPECTRUM_SCAN;
using namespace DEVICES;

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

#define FONT_HEIGHT	14

#define DIVLONG		10
#define DIVSHORT	5

#define POINT_WIDTH 5
#define POINT_WIDTH2 10

#define FFT_ZOOM_MAX		16

CWinSpectrumScanFFTShow clsWinSpectrumScanFFTShow;

CWinSpectrumScanFFTShow::CWinSpectrumScanFFTShow()
{
	OPENCONSOLE_SAVED;
	hDrawMutex = CreateMutex(NULL, false, "CWinScanFFThDrawMutex");
	RegisterWindowsClass();
	fft = new CFFT("SpectrumScan", &FFTInfo_Spectrum_Scan);
}

CWinSpectrumScanFFTShow::~CWinSpectrumScanFFTShow()
{
	//CLOSECONSOLE;
}

void CWinSpectrumScanFFTShow::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinSpectrumScanFFTShow::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_SPECTRUM_SCAN;
	wcex.style = CS_DBLCLKS;
	wcex.lpszClassName = WIN_SPECTRUM_SCAN_FFT_SHOW_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

LRESULT CALLBACK CWinSpectrumScanFFTShow::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinSpectrumScanFFTShow.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinSpectrumScanFFTShow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rt;

	switch (message)
	{
	case WM_CREATE:
	{
		this->hWnd = hWnd;
		fft->hWnd = hWnd;

		uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		//KillTimer(hWnd, uTimerId);
	}
	break;
	case WM_CHAR:
		DbgMsg("WinOneSDRScanFFT WM_CHAR\r\n");
		break;
	case WM_LBUTTONDOWN:
		LeftBottonDown = true;
		DragMousePoint.x = MouseX;
		DragMousePoint.y = MouseY;
		break;
	case WM_LBUTTONUP:
		LeftBottonDown = false;
	case WM_RBUTTONUP:
		P2SubP1(message);
	break;
	case WM_MOUSEMOVE:
		MouseX = GET_X_LPARAM(lParam);
		MouseY = GET_Y_LPARAM(lParam);
		OnMouse();
		DragMoveFFT();
		break;
	case WM_TIMER:
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		fft->FFTNext = true;
		break;
	case WM_LBUTTONDBLCLK:
	{
		DbgMsg("WM_LBUTTONDBLCLK\r\n");
		INT X = MouseX;
		if (X < WAVE_RECT_BORDER_LEFT || X >(WinRect.right - WAVE_RECT_BORDER_RIGHT)) break;
		X -= WAVE_RECT_BORDER_LEFT;
		float old_Zoom = HScrollZoom;
		HScrollZoom = 1.0;
		HScrollRange = fft->FFTInfo->HalfFFTSize * HScrollZoom -
			(WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		UP_TO_ZERO(HScrollRange);
		SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
		HScrollPos /= old_Zoom;
		if(old_Zoom < 1.0) HScrollPos += (X / old_Zoom - X); else HScrollPos -= (X - X / old_Zoom);
		HScrollPos = BOUND(HScrollPos, 0, HScrollRange);
		SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);

	}
	break;
	case WM_MOUSEWHEEL:
	{
		INT fwKeys = GET_KEYSTATE_WPARAM(wParam);
		INT zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		INT X = MouseX;
		if (X < WAVE_RECT_BORDER_LEFT || X >(WinRect.right - WAVE_RECT_BORDER_RIGHT)) break;
		//DbgMsg("WM_MOUSEWHEEL fwKeys：%d, zDelta:%d, x:%d\r\n", fwKeys, zDelta, X);
		X -= WAVE_RECT_BORDER_LEFT;
		zDelta = zDelta / 120;
		//DbgMsg("WM_MOUSEWHEEL hpos:%d\r\n", clsWinSpect.HScrollPos);
		if (zDelta > 0) {
			while (HScrollZoom * (1 << zDelta) > FFT_ZOOM_MAX) { zDelta--; }
			HScrollZoom *= (1 << zDelta);
			HScrollRange = fft->FFTInfo->HalfFFTSize * HScrollZoom -
				(WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			UP_TO_ZERO(HScrollRange);
			SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
			HScrollPos *= (1 << zDelta);
			HScrollPos += (X * (1 << zDelta) - X);
			HScrollPos = BOUND(HScrollPos, 0, HScrollRange);
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
		}
		else {
			if (
				HScrollZoom * fft->FFTInfo->HalfFFTSize <
				WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT
				)break;
			zDelta = -zDelta;
			INT deltaSave = zDelta;
			while (
				zDelta > 0 &&
				HScrollZoom / (1 << zDelta) * fft->FFTInfo->HalfFFTSize <
				WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT
				) {
				zDelta--;
			}
			if (deltaSave != zDelta) zDelta++;
			HScrollZoom /= (1 << zDelta);
			HScrollRange = fft->FFTInfo->HalfFFTSize * HScrollZoom -
				(WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			UP_TO_ZERO(HScrollRange);
			SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
			HScrollPos /= (1 << zDelta);
			HScrollPos -= (X - X / (1 << zDelta));
			HScrollPos = BOUND(HScrollPos, 0, HScrollRange);
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
		}
		//DbgMsg("WM_MOUSEWHEEL hpos:%d\r\n", clsWinSpect.HScrollPos);

		//HScrollPos = BOUND(HScrollPos - delta, 0, me->VScrollRang);
		//SetScrollPos(hWnd, SB_VERT, me->VScrollPos, true);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
	}
	break;
	case WM_FFT:
	{
		if (bFFTBrieflyShow == true || bFFTBrieflyLogShow == true) BrieflyBuff((CFFT*)lParam);
		PaintSpectrum((CFFT*)lParam);
	}
	break;
	case WM_SIZE:
		GetClientRect(hWnd, &WinRect);
		HScrollRange = fft->FFTInfo->HalfFFTSize * HScrollZoom - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		UP_TO_ZERO(HScrollRange);
		SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
		VScrollRange = (DBMin * -64) * VScrollZoom - (FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON);
		UP_TO_ZERO(VScrollRange);
		SetScrollRange(hWnd, SB_VERT, 0, VScrollRange, TRUE);
		InitDrawBuff();
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
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
		Paint();
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		KeyAndScroll(message, wParam, lParam);
		break;
	case WM_DESTROY:
		this->hWnd = NULL;
		DbgMsg("WM_DESTROY CWinSpectrumScanFFTShow\r\n");
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CWinSpectrumScanFFTShow::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDM_FFT_HORZ_ZOOM_INCREASE:
		if (HScrollZoom < FFT_ZOOM_MAX) {
			HScrollZoom *= 2.0;
			HScrollRange = fft->FFTInfo->HalfFFTSize * HScrollZoom - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			UP_TO_ZERO(HScrollRange);
			SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
			HScrollPos *= 2.0;
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_DECREASE:
		if (HScrollZoom * fft->FFTInfo->HalfFFTSize > WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) {
			HScrollZoom /= 2.0;
			HScrollRange = fft->FFTInfo->HalfFFTSize * HScrollZoom - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			UP_TO_ZERO(HScrollRange);
			SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
			HScrollPos /= 2.0;
			HScrollPos = BOUND(HScrollPos, 0, HScrollRange);
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
		}
		break;
	case IDM_FFT_HORZ_ZOOM_HOME:
		HScrollPos /= HScrollZoom;
		HScrollRange = fft->FFTInfo->HalfFFTSize - (WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		UP_TO_ZERO(HScrollRange);
		SetScrollRange(hWnd, SB_HORZ, 0, HScrollRange, TRUE);
		SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
		HScrollZoom = 1.0;
		break;

	case IDM_FFT_VERT_ZOOM_INCREASE:
		if (VScrollZoom < FFT_ZOOM_MAX) {
			VScrollZoom *= 2.0;
			VScrollRange = (DBMin * -64) * VScrollZoom - (FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON);
			SetScrollRange(hWnd, SB_VERT, 0, VScrollRange, TRUE);
			VScrollPos *= 2.0;
			SetScrollPos(hWnd, SB_VERT, VScrollPos, TRUE);
		}
		break;
	case IDM_FFT_VERT_ZOOM_DECREASE:
		if ((VScrollZoom * DBMin * -64.0) > FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON) {
			VScrollZoom /= 2.0;
			VScrollRange = (DBMin * -64) * VScrollZoom - (FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON);
			SetScrollRange(hWnd, SB_VERT, 0, VScrollRange, TRUE);
			VScrollPos /= 2.0;
			SetScrollPos(hWnd, SB_VERT, VScrollPos, TRUE);
		}
		break;
	case IDM_FFT_VERT_ZOOM_HOME:
		VScrollPos /= VScrollZoom;
		VScrollRange = (DBMin * -64) - (FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON);
		SetScrollRange(hWnd, SB_VERT, 0, VScrollRange, TRUE);
		SetScrollPos(hWnd, SB_VERT, VScrollPos, TRUE);
		VScrollZoom = 1.0;
		break;

	case IDM_FFT_ORIGNAL_SHOW:
		bFFTShow = !bFFTShow;
		fft->bShow = bFFTShow;
		break;
	case IDM_FFT_ORIGNAL_LOG_SHOW:
		bFFTLogShow = !bFFTLogShow;
		fft->bLogShow = bFFTLogShow;
		break;
	case IDM_FFT_BRIEFLY_SHOW:
		bFFTBrieflyShow = !bFFTBrieflyShow;
		//fft->bShow = bFFTShow;
		break;
	case IDM_FFT_BRIEFLY_LOG_SHOW:
		bFFTBrieflyLogShow = !bFFTBrieflyLogShow;
		//fft->bLogShow = bFFTLogShow;
		break;

	case IDM_FFT_SET:
		DialogBoxParam(hInst, (LPCTSTR)IDD_DLG_FFT_SET, hWnd, (DLGPROC)CWinFFT::DlgFFTSetProc, (LPARAM)this);
		break;

	case IDM_SPECTRUM_ORIGNAL_SHOW:
		SpectrumShow = SPECTRUM_SHOW::show_orignal;
	break;
	case IDM_SPECTRUM_ORIGNAL_LOG_SHOW:
		SpectrumShow = SPECTRUM_SHOW::show_log;
		break;
	case IDM_SPECTRUM_BRIEFLY_SHOW:
		SpectrumShow = SPECTRUM_SHOW::show_briefly;
		break;
	case IDM_SPECTRUM_BRIEFLY_LOG_SHOW:
		SpectrumShow = SPECTRUM_SHOW::show_briefly_log;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}


void CWinSpectrumScanFFTShow::OnMouse(void)
{
	int i = 0, n = 0;
	char t[100];
	int X = (HScrollPos + MouseX - WAVE_RECT_BORDER_LEFT) / HScrollZoom;
	X = BOUND(X, 0, (HScrollPos + WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / HScrollZoom);

	double Hz = (double)X * fft->Data->SampleRate / fft->FFTInfo->FFTSize;
	double Y = X > fft->FFTInfo->HalfFFTSize ? 0 : fft->FFTOutBuff[X];
	n += sprintf(strMouse + n, "Hz: %.03f, FFT: %s", Hz, formatKDouble(Y, 0.001, "", t));

	int Ypos = BOUND(MouseY - WAVE_RECT_BORDER_TOP, 0, WAVE_RECT_HEIGHT);
	double Ylog = 20 * (X > fft->FFTInfo->HalfFFTSize ? 0 : fft->FFTOutLogBuff[X]);
	n += sprintf(strMouse + n, " | ");
	n += sprintf(strMouse + n, "Y: %.03fdb, logFFT: %fdb", -(double)Ypos / 64 * 20, Ylog);
}

void CWinSpectrumScanFFTShow::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
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
			tbdn = GET_WM_VSCROLL_POS(wParam, lParam);
			break;
		}
		if (dn != 0)
		{
			VScrollPos = BOUND(VScrollPos + dn, 0, VScrollRange);
		}
		if (tbdn != 0)
		{
			VScrollPos = BOUND(tbdn, 0, VScrollRange);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_HORZ, VScrollPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
			DbgMsg("CWinSDRScan HScrollPos: %d, HScrollRange: %d.\r\n", VScrollPos, VScrollRange);
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

void CWinSpectrumScanFFTShow::BrieflyBuff(CFFT* fft)
{
	WaitForSingleObject(fft->hMutexDraw, INFINITE);
	double* pfft = fft->FFTOutBuff;
	double* pfftlog = fft->FFTOutLogBuff;
	double* pbrifft = fft->FFTBrieflyBuff;
	double* pbrifftlog = fft->FFTBrieflyLogBuff;
	int stepn = 1 / HScrollZoom;
	if (stepn == 0)stepn = 1;
	int N = fft->FFTInfo->HalfFFTSize / stepn;
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
		int si = i * stepn;
		pbrifft[si] = maxd;
		pbrifftlog[si] = maxdlog;
		if (mind > maxd) mind = maxd;
		if (minlogd > maxdlog) minlogd = maxdlog;
	}
	pbrifft[fft->FFTInfo->HalfFFTSize] = pfft[fft->FFTInfo->HalfFFTSize];
	pbrifft[fft->FFTInfo->HalfFFTSize + 1] = mind;// pfft[clsWaveFFT.HalfFFTSize + 1];
	pbrifftlog[fft->FFTInfo->HalfFFTSize] = pfftlog[fft->FFTInfo->HalfFFTSize];
	pbrifftlog[fft->FFTInfo->HalfFFTSize + 1] = minlogd;// pfftlog[clsWaveFFT.HalfFFTSize + 1];
	ReleaseMutex(fft->hMutexDraw);
}

void CWinSpectrumScanFFTShow::InitDrawBuff(void)
{
	WaitForSingleObject(hDrawMutex, INFINITE);
	SpectrumY = -1;
	RECT rt = { 0 };
	HDC hDC = GetDC(hWnd);
	rt.top = 0;
	rt.left = 0;
	rt.right = WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	rt.bottom = WinRect.bottom - FFTHeight;

	if (hDCFFT) DeleteObject(hDCFFT);
	if (hBMPFFT) DeleteObject(hBMPFFT);
	hDCFFT = CreateCompatibleDC(hDC);
	hBMPFFT = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hDCFFT, hBMPFFT);
	FillRect(hDCFFT, &rt, (HBRUSH)GetStockObject(BLACK_BRUSH));

	ReleaseDC(hWnd, hDC);
	ReleaseMutex(hDrawMutex);
}

void CWinSpectrumScanFFTShow::PaintSpectrum(CFFT* fft)
{
	WaitForSingleObject(hDrawMutex, INFINITE);
	if (hWnd == NULL) {
		ReleaseMutex(hDrawMutex);
		return;
	}

	double* pBuf = SpectrumShow == SPECTRUM_SHOW::show_orignal ? fft->FFTOutBuff : (SpectrumShow == SPECTRUM_SHOW::show_log ? fft->FFTOutLogBuff :
			(SpectrumShow == SPECTRUM_SHOW::show_briefly ? fft->FFTBrieflyBuff : fft->FFTBrieflyLogBuff));

	UINT32 halfFFTSize = fft->FFTInfo->HalfFFTSize;
	UINT DrawLen = WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	RECT rt;
	UINT64 color;
	UINT8 c;
	BYTE R, G = 0, C;
	COLORREF cx;

	if (SpectrumY == -1) SpectrumY = WinRect.bottom - FFTHeight - 1;

	static HFONT hFont = CreateFont(FONT_HEIGHT, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial"));
	SelectObject(hDCFFT, hFont);


	int istep = HScrollZoom < 1.0 ? ((double)1.0 / HScrollZoom) : 1;
	int X;

	double averageV = 0.0;
	double minV = DBL_MAX;
	double maxV = -1.0 * DBL_MAX;
	UINT averagei = 0;

	//UINT left = AverageRange.P0.x > HScrollPos / HScrollZoom ? AverageRange.P0.x : HScrollPos / HScrollZoom;
	//UINT right = AverageRange.P1.x < (HScrollPos + WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / HScrollZoom ?
	//	AverageRange.P1.x : (HScrollPos + WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / HScrollZoom;

	UINT left = HScrollPos / HScrollZoom;
	UINT right = (HScrollPos + WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / HScrollZoom;

	WaitForSingleObject(fft->hMutexDraw, INFINITE);

	for (int i = left; i < halfFFTSize && i < right; i += istep) {
		averageV += pBuf[i];
		minV = min(minV, pBuf[i]);
		maxV = max(maxV, pBuf[i]);
		averagei++;
	}
	averageV /= averagei;
	AverageValue = averageV;
	double top, botton;
	double fftmaxdiff = maxV - minV;
	double topToAverage = 1.0;
	double averageTobotton = 0.2;
	top = averageV + topToAverage;
	botton = averageV - averageTobotton;
	double fftmaxdiffstep = topToAverage / 3.0;
	double fftdiffstep1 = top - 1 * fftmaxdiffstep;
	double fftdiffstep2 = top - 2 * fftmaxdiffstep;
	double fftdiffstep3 = top - 3 * fftmaxdiffstep;

	//double fftmaxv;
	//double fftminv;
	//fftmaxv = pBuf[halfFFTSize];
	//fftminv = pBuf[halfFFTSize + 1];

	//int istep = HScrollZoom < 1.0 ? ((double)1.0 / HScrollZoom) : 1;
	//int X;
	//double fftmaxdiff = fftmaxv - fftminv;
	//double fftmaxdiffstep = fftmaxdiff / 4.0;
	//double fftdiffstep1 = fftmaxv - 1 * fftmaxdiffstep;
	//double fftdiffstep2 = fftmaxv - 2 * fftmaxdiffstep;
	//double fftdiffstep3 = fftmaxv - 3 * fftmaxdiffstep;


//	if (isDrawLogSpectrum == true) {

	if (SpectrumShow == SPECTRUM_SHOW::show_log || SpectrumShow == SPECTRUM_SHOW::show_briefly_log) {
		X = 0;
		for (int i = HScrollPos / HScrollZoom;
			i < halfFFTSize &&
			(i * HScrollZoom - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			i += istep)
		{
			double v = pBuf[i];
			if (v > top) v = top;
			double d;
			d = v - fftdiffstep2;
			if (d > 0) {
				C = (BYTE)((0xFF / (2 * fftmaxdiffstep)) * d);
				cx = RGB(255, 255 - C / 2, ~C);
			}
			else {
				d = v - fftdiffstep3;
				if (d > 0) {
					C = (BYTE)((0xFF / fftmaxdiffstep) * d);
					cx = RGB(C, C, 255);
				}
				else {
					//d = v - fftdiffstep3;
					//if (d > 0) {
					//	C = (BYTE)((0xFF / fftmaxdiffstep) * d);
					//	cx = RGB(0, 0, C);
					//}
					//else {
					d = v - botton;
					C = (BYTE)((0xFF / averageTobotton) * d);
					cx = RGB(0, 0, C);
					//}
				}
			}
			//SetPixel(clsWinOneSpectrum.hDCFFT, X, clsWinOneSpectrum.SpectrumY, cx);
			//X++;

			if (HScrollZoom <= 1.0) {
				SetPixel(hDCFFT, X, SpectrumY, cx);
				X++;
			}
			else {
				HPEN hPen = CreatePen(PS_SOLID, 0, cx);
				SelectObject(hDCFFT, hPen);
				MoveToEx(hDCFFT, X - HScrollZoom / 2, SpectrumY, NULL);
				LineTo(hDCFFT, X + HScrollZoom / 2, SpectrumY);
				X += HScrollZoom;
				DeleteObject(hPen);
			}
		}
	}
	else {
		X = 0;
		for (int i = HScrollPos / HScrollZoom;
			i < halfFFTSize &&
			(i * HScrollZoom - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			i += istep)
		{
			//			color = (UINT)(((double)(1.0 * 0xFFFFFFFF) / fftmaxdiff) * (pBuf[i] - fftminv));
			//			color = pBuf[y] > pWinData->FFTAvgMaxValue ? (UINT64)0xFFFFFFFF : pBuf[y] * (double)0xFFFFFFFF / pWinData->FFTAvgMaxValue;
						//color = pBuf[i] > clsWaveFFT.FFTMaxValue ? (UINT64)0xFFFFFFFF : pBuf[i] * (double)0xFFFFFFFF / clsWaveFFT.FFTMaxValue;
						//color = pBuf[y] > fftmaxv ? (UINT64)0xFFFFFF : pBuf[y] * (double)0x1FFFFF / fftmaxv;
						//c = pBuf[y] > pWinData->FFTAvgMaxValue ? 0xFF : (0xFF * pBuf[y] / pWinData->FFTAvgMaxValue);

			/*
						if (C = (BYTE)(color >> 24)) cx = RGB(255, ~C, ~C);
						else {
							if (C = (BYTE)(color >> 16)) cx = RGB(C, C, 255);
							else {
								if (C = (BYTE)(color >> 8)) cx = RGB(0, ~C, C);
								else {
									C = (BYTE)color;
									cx = RGB(0, C, 0);
								}
							}
						}
						*/
						//			color = (UINT)(((double)(1.0 * 0xFFFFFFFF) / fftmaxdiff) * (pBuf[i] - fftminv));
			color = (UINT)(((double)(1.0 * 0xFFFFFFFF) / fft->FFTMaxValue) * (pBuf[i]));
			if (C = (BYTE)(color >> 24)) cx = RGB(255, ~C, ~C);
			else {
				if (C = (BYTE)(color >> 16)) cx = RGB(C, C, 255);
				else {
					if (C = (BYTE)(color >> 8)) cx = RGB(~C, ~C, C);
					else {
						C = (BYTE)color;
						cx = RGB(C, 255, ~C);
					}
				}
			}
			/*
						double v = pBuf[i];
						double d;
						d = v - fftdiffstep1;
						if (d > 0) {
							C = (BYTE)((0xFF / fftmaxdiffstep) * d);
							cx = RGB(~C, 255, ~C);
						}
						else {
							d = v - fftdiffstep2;
							if (d > 0) {
								C = (BYTE)((0xFF / fftmaxdiffstep) * d);
								cx = RGB(C, C, 255);
							}
							else {
								d = v - fftminv;
								C = (BYTE)((0xFF / fftmaxdiffstep) * d);
								cx = RGB(0, 0, C);
							}
						}
				*/
			if (HScrollZoom <= 1.0) {
				SetPixel(hDCFFT, X, SpectrumY, cx);
				X++;
			}
			else {
				HPEN hPen = CreatePen(PS_SOLID, 0, cx);
				SelectObject(hDCFFT, hPen);
				MoveToEx(hDCFFT, X - HScrollZoom / 2, SpectrumY, NULL);
				LineTo(hDCFFT, X + HScrollZoom / 2, SpectrumY);
				X += HScrollZoom;
				DeleteObject(hPen);
			}
		}
	}
	DeleteObject(SelectObject(hDCFFT, GetStockObject(SYSTEM_FONT)));
	SpectrumY--;
	ReleaseMutex(fft->hMutexDraw);
	ReleaseMutex(hDrawMutex);
}

void CWinSpectrumScanFFTShow::SpectrumToWin(HDC hDC)
{
	WaitForSingleObject(hDrawMutex, INFINITE);
	RECT rt = WinRect;
	int spectY = SpectrumY + 1;
	int destLeft = rt.left + WAVE_RECT_BORDER_LEFT;
	int destWidth = rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	StretchBlt(hDC,
		destLeft, FFTHeight,
		destWidth, rt.bottom - FFTHeight - spectY,
		hDCFFT,
		0, spectY,
		destWidth, rt.bottom - FFTHeight - spectY,
		SRCCOPY);
	if (spectY)	StretchBlt(hDC,
		destLeft, rt.bottom - spectY,
		destWidth, spectY,
		hDCFFT,
		0, 0,
		destWidth, spectY,
		SRCCOPY);
	ReleaseMutex(hDrawMutex);

}

void CWinSpectrumScanFFTShow::PaintFFTBriefly(HDC hdc, RECT* rt, CFFT* fft)
{
	if (fft == NULL) return;
	WaitForSingleObject(fft->hMutexBuff, INFINITE);

	HPEN hPenSaved = NULL;
	double Y = 0;
	int X;
	int FFTLength = fft->FFTInfo->HalfFFTSize;
	UINT FFTDrawHeight = FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON;
	double* pBuf = fft->FFTBrieflyBuff;
	double fftvmax;
	double fftvmin;
	int i = 0;
	UINT64 maxHightPix = DBMin * -64 * VScrollZoom;

	int Xstep = HScrollZoom > 1.0 ? HScrollZoom : 1;
	int istep = HScrollZoom < 1.0 ? ((double)1.0 / HScrollZoom) : 1;
	if (pBuf && fft->bShow) {
		hPenSaved = (HPEN)SelectObject(hdc, fft->hPen);

		//int x, y;
		//x = rt->right - WAVE_RECT_BORDER_RIGHT - 100;
		//y = WAVE_RECT_BORDER_TOP + 10 + CommNum++ * 14;
		//MoveToEx(hdc, x, y + 7, NULL);
		//LineTo(hdc, x + 20, y + 7);
		//RECT r = { x + 30, y, x + 200, y + 50 };
		//DrawText(hdc, (LPCSTR)fft->Flag, strlen((char*)fft->Flag), &r, NULL);

		double scale = (double)maxHightPix / fft->FFTMaxValue;
		//double scale = FFTDrawHeight / fftvmax;
		fftvmax = pBuf[FFTLength];
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength) Y = FFTDrawHeight - pBuf[i] * scale;
		Y = BOUND(Y, 0, FFTDrawHeight);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = FFTDrawHeight - pBuf[i] * scale;
			Y = BOUND(Y, 0, FFTDrawHeight);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}

		if (SpectrumShow == SPECTRUM_SHOW::show_briefly) {
			Y = FFTDrawHeight - AverageValue * scale;
			Y = BOUND(Y, 0, FFTDrawHeight);
			MoveToEx(hdc, WAVE_RECT_BORDER_LEFT, WAVE_RECT_BORDER_TOP + Y, NULL);
			LineTo(hdc, WinRect.right - WAVE_RECT_BORDER_RIGHT, WAVE_RECT_BORDER_TOP + Y);
		}

		SelectObject(hdc, hPenSaved);
	}

	double* pLogBuf = fft->FFTBrieflyLogBuff;
	if (pLogBuf && fft->bLogShow) {
		hPenSaved = (HPEN)SelectObject(hdc, fft->hPenLog);

		//int x, y;
		//x = rt->right - WAVE_RECT_BORDER_RIGHT - 100;
		//y = WAVE_RECT_BORDER_TOP + 10 + CommNum++ * 14;
		//MoveToEx(hdc, x, y + 7, NULL);
		//LineTo(hdc, x + 20, y + 7);
		//RECT r = { x + 30, y, x + 200, y + 50 };
		//char s[100];
		//int n = sprintf(s, "%sLog", fft->Flag);
		//DrawText(hdc, (LPCSTR)s, n, &r, NULL);

		fftvmax = pLogBuf[FFTLength];
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength) Y = pLogBuf[i] * -64 * VScrollZoom - VScrollPos;
		Y = BOUND(Y, 0, FFTDrawHeight);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pLogBuf[i] * -64 * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, FFTDrawHeight);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}

		if (SpectrumShow == SPECTRUM_SHOW::show_briefly_log) {
			Y = AverageValue * -64 * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, FFTDrawHeight);
			MoveToEx(hdc, WAVE_RECT_BORDER_LEFT, WAVE_RECT_BORDER_TOP + Y, NULL);
			LineTo(hdc, WinRect.right - WAVE_RECT_BORDER_RIGHT, WAVE_RECT_BORDER_TOP + Y);
		}

		SelectObject(hdc, hPenSaved);
	}
	ReleaseMutex(fft->hMutexBuff);
}

void CWinSpectrumScanFFTShow::PaintFFT(HDC hdc, RECT* rt, CFFT* fft)
{
	if (fft == NULL) return;

	WaitForSingleObject(fft->hMutexDraw, INFINITE);

	HPEN hPenSaved = NULL;
	double Y = 0;
	int X;
	int FFTLength = fft->FFTInfo->HalfFFTSize;
	UINT FFTDrawHeight = FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON;
	double* pBuf = fft->FFTOutBuff;
	double fftvmax;
	double fftvmin;
	int i = 0;
	UINT64 maxHightPix = DBMin * -64 * VScrollZoom;

	int Xstep = HScrollZoom > 1.0 ? HScrollZoom : 1;
	int istep = HScrollZoom < 1.0 ? ((double)1.0 / HScrollZoom) : 1;
	if (pBuf && fft->bShow) {
		hPenSaved = (HPEN)SelectObject(hdc, fft->hPen);

		int x, y;
		x = rt->right - WAVE_RECT_BORDER_RIGHT - 100;
		y = WAVE_RECT_BORDER_TOP + 10 + CommNum++ * 14;
		MoveToEx(hdc, x, y + 7, NULL);
		LineTo(hdc, x + 20, y + 7);
		RECT r = { x + 30, y, x + 200, y + 50 };
		DrawText(hdc, (LPCSTR)fft->Flag, strlen((char*)fft->Flag), &r, NULL);

		double scale = (double)maxHightPix / fft->FFTMaxValue;
		fftvmax = pBuf[FFTLength];
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength) Y = FFTDrawHeight - pBuf[i] * scale;
		Y = BOUND(Y, 0, FFTDrawHeight);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = FFTDrawHeight - pBuf[i] * scale;
			Y = BOUND(Y, 0, FFTDrawHeight);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}

		if (SpectrumShow == SPECTRUM_SHOW::show_orignal) {
			Y = FFTDrawHeight - AverageValue * scale;
			Y = BOUND(Y, 0, FFTDrawHeight);
			MoveToEx(hdc, WAVE_RECT_BORDER_LEFT, WAVE_RECT_BORDER_TOP + Y, NULL);
			LineTo(hdc, WinRect.right - WAVE_RECT_BORDER_RIGHT, WAVE_RECT_BORDER_TOP + Y);
		}

		SelectObject(hdc, hPenSaved);
	}

	double* pLogBuf = fft->FFTOutLogBuff;
	if (pLogBuf && fft->bLogShow) {
		//绘制滤波 FFT Log10 信号--------------------------------------------------
		hPenSaved = (HPEN)SelectObject(hdc, fft->hPenLog);

		int x, y;
		x = rt->right - WAVE_RECT_BORDER_RIGHT - 100;
		y = WAVE_RECT_BORDER_TOP + 10 + CommNum++ * 14;
		MoveToEx(hdc, x, y + 7, NULL);
		LineTo(hdc, x + 20, y + 7);
		RECT r = { x + 30, y, x + 200, y + 50 };
		char s[100];
		int n = sprintf(s, "%sLog", fft->Flag);
		DrawText(hdc, (LPCSTR)s, n, &r, NULL);

		fftvmax = pLogBuf[FFTLength];
		i = HScrollPos / HScrollZoom;
		if (i < FFTLength) Y = pLogBuf[i] * -64 * VScrollZoom - VScrollPos;
		Y = BOUND(Y, 0, FFTDrawHeight);
		X = WAVE_RECT_BORDER_LEFT;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		for (i += istep; i < FFTLength && (i * HScrollZoom - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
		{
			X += Xstep;
			Y = pLogBuf[i] * -64 * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, FFTDrawHeight);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y);
		}

		if (SpectrumShow == SPECTRUM_SHOW::show_log) {
			Y = AverageValue * -64 * VScrollZoom - VScrollPos;
			Y = BOUND(Y, 0, FFTDrawHeight);
			MoveToEx(hdc, WAVE_RECT_BORDER_LEFT, WAVE_RECT_BORDER_TOP + Y, NULL);
			LineTo(hdc, WinRect.right - WAVE_RECT_BORDER_RIGHT, WAVE_RECT_BORDER_TOP + Y);
		}

		SelectObject(hdc, hPenSaved);
	}
	ReleaseMutex(fft->hMutexDraw);

}

void CWinSpectrumScanFFTShow::Paint(void)
{
	HDC		hDC;
	PAINTSTRUCT ps;
	RECT	rt, r;
	HPEN	hPen, hPenLighter;

	hDC = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rt);

	//rt.right = WinRect.right;
	rt.bottom = FFTHeight;

	HDC		hdc = CreateCompatibleDC(hDC);
	HBITMAP hbmp = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hdc, hbmp);

	static HFONT hFont = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial"));
	SelectObject(hdc, (HFONT)hFont);

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
	SetTextColor(hdc, COLOR_TEXT);

#define WEB_H_STEP	8
	UINT web_hoffset = WEB_H_STEP - HScrollPos % WEB_H_STEP;
	if (web_hoffset == WEB_H_STEP)web_hoffset = 0;
	UINT long_step = WEB_H_STEP * 4;
	UINT FFTDrawHeight = FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON;
	for (x = WAVE_RECT_BORDER_LEFT + web_hoffset; x <= rt.right - WAVE_RECT_BORDER_RIGHT; x += WEB_H_STEP)
	{
		UINT64 realPos = HScrollPos + x - WAVE_RECT_BORDER_LEFT;
		UINT LineOffset = realPos % long_step ? DIVSHORT : DIVLONG;
		UINT64 lineNum = realPos / WEB_H_STEP;
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP - LineOffset, NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP);

		if ((lineNum % 5) == 0) {
			if ((lineNum % 20) == 0) {
				UINT64 pos = (UINT64)(((double)realPos) / HScrollZoom);
				sprintf(s, "%lu", pos);
				r.left = x;
				r.top = WAVE_RECT_BORDER_TOP - DIVLONG - FONT_HEIGHT;
				DrawText(hdc, s, strlen(s), &r, NULL);
				//sprintf(s, "%.6fs", (double)pos / *SampleRate);
				//formatKKDouble((double)pos / *SampleRate, "s", s);
				sprintf(s, "%.03fhz", (double)(pos * fft->Data->SampleRate / fft->FFTInfo->FFTSize));
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
			//if (lineNum % 8 == 0)
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

	CommNum = 0;

	PaintFFT(hdc, &rt, fft);

	if (bFFTBrieflyShow == true || bFFTBrieflyLogShow == true) PaintFFTBriefly(hdc, &rt, fft);

	{
		static HPEN hPen = CreatePen(PS_SOLID, 0, RGB(128, 128, 0));
		HPEN hPen_old = (HPEN)SelectObject(hdc, (HPEN)hPen);
		int x;
		x = AverageRange.P0.x * HScrollZoom - HScrollPos + WAVE_RECT_BORDER_LEFT;
		if (x > WAVE_RECT_BORDER_LEFT && x < WinRect.right - WAVE_RECT_BORDER_RIGHT) DrawPoint(hdc, &AverageRange.P0, "Left");
		x = AverageRange.P1.x * HScrollZoom - HScrollPos + WAVE_RECT_BORDER_LEFT;
		if (x > WAVE_RECT_BORDER_LEFT && x < WinRect.right - WAVE_RECT_BORDER_RIGHT) DrawPoint(hdc, &AverageRange.P1, "Right");
		SelectObject(hdc, (HPEN)hPen_old);
	}

	DrawSignals(hdc);

	//---------------------------------------
	{
		//绘制
#define DRAW_TEXT_X		(WAVE_RECT_BORDER_LEFT + 10)	
#define DRAW_TEXT_Y		(WAVE_RECT_BORDER_TOP + DIVLONG + 5)

		INT textY = DRAW_TEXT_Y;
		{
			//绘制鼠标提示消息
			r.top = textY;
			r.left = DRAW_TEXT_X;
			r.right = rt.right;
			r.bottom = rt.bottom;
			SetTextColor(hdc, COLOR_ORIGNAL_FFT);
			DrawText(hdc, strMouse, strlen(strMouse), &r, NULL);
			r.top = textY += FONT_HEIGHT;
			DrawText(hdc, strPP, strlen(strPP), &r, NULL);
		}
		
		DrawPoint(hdc, &ScreenP1, "P1");
		DrawPoint(hdc, &ScreenP2, "P2");

		double FullVotage = 5.0;
		double VotagePerDIV = (FullVotage / (unsigned __int64)((UINT64)1 << (sizeof(ADC_DATA_TYPE) * 8)));
		char tstr1[100], tstr2[100];
		r.top = textY += FONT_HEIGHT * 3;
		char t1[100], t11[100], t2[100], t3[100], t4[100], t5[100];
		sprintf(s, "32pix / DIV\r\n"\
			"Data SampleRate=%s\r\n"\
			"FFTSize=%s  FFTStep=%s FFTPerSec=%.03f\r\n"\
			"SDR rf=%s\r\n"\
			"HZoom=%f HZoom=%f\r\n"
			,
			fomatKINT64(fft->Data->SampleRate, t1),
			fomatKINT64(fft->FFTInfo->FFTSize, t2), fomatKINT64(fft->FFTInfo->FFTStep, t3), fft->FFTInfo->FFTPerSec,
			formatKDouble(clsGetDataSDR.chParams->tunerParams.rfFreq.rfHz, 0.001, "", t4),
			HScrollZoom, VScrollZoom
		);
		//SetBkMode(hdc, TRANSPARENT);
		//	SetBkMode(hdc, OPAQUE); 
		//	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);

		SetTextColor(hdc, COLOR_TEXT);
		DrawText(hdc, s, strlen(s), &r, NULL);
	}

	//SelectObject(hdc, GetStockObject(SYSTEM_FONT));

	BitBlt(hDC,
		0, 0,
		rt.right, rt.bottom,
		hdc,
		0, 0,
		SRCCOPY);
	DeleteObject(hdc);
	DeleteObject(hbmp);

	SpectrumToWin(hDC);

	EndPaint(hWnd, &ps);
}

void CWinSpectrumScanFFTShow::DrawPoint(HDC hdc, POINT* P, char* flag)
{
	RECT r = { 0 };
	int x = P->x * HScrollZoom - HScrollPos + WAVE_RECT_BORDER_LEFT;
	MoveToEx(hdc, x - POINT_WIDTH, P->y - POINT_WIDTH, NULL);
	LineTo(hdc, x + POINT_WIDTH, P->y - POINT_WIDTH);
	LineTo(hdc, x + POINT_WIDTH, P->y + POINT_WIDTH);
	LineTo(hdc, x - POINT_WIDTH, P->y + POINT_WIDTH);
	LineTo(hdc, x - POINT_WIDTH, P->y - POINT_WIDTH);

	MoveToEx(hdc, x - POINT_WIDTH2, P->y, NULL);
	LineTo(hdc, x + POINT_WIDTH2, P->y);
	MoveToEx(hdc, x, P->y - POINT_WIDTH2, NULL);
	LineTo(hdc, x, P->y + POINT_WIDTH2);
	r.top = P->y - POINT_WIDTH2 / 2;
	r.left = x + POINT_WIDTH2 + 2;
	r.right = WinRect.right;
	r.bottom = WinRect.bottom;
	DrawText(hdc, flag, strlen(flag), &r, NULL);
}

void CWinSpectrumScanFFTShow::DragMoveFFT(void)
{
	if (LeftBottonDown == true && DragMousePoint.x != MouseX) {

		HScrollPos = BOUND(HScrollPos + (DragMousePoint.x - MouseX), 0, HScrollRange);
		SetScrollPos(hWnd, SB_HORZ, HScrollPos, true);

		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);

		DragMousePoint.x = MouseX;
		DragMousePoint.y = MouseY;
	}
}

void CWinSpectrumScanFFTShow::FFTProcessCallBackInit(CFFT* fft)
{
	CWinSpectrumScanFFTShow* cls = &clsWinSpectrumScanFFTShow;
	if (cls->Signals != NULL) delete[] cls->Signals;
	cls->Signals = new CWinSpectrumScanFFTShow::SIGNAL_INFO[fft->FFTInfo->HalfFFTSize];
}

#define SIGNAL_DIFF	0.15
void CWinSpectrumScanFFTShow::FFTProcessCallBack(CFFT* fft)
{
	CWinSpectrumScanFFTShow* cls = &clsWinSpectrumScanFFTShow;
	SIGNAL_INFO* si = cls->Signals;

	int N = fft->FFTInfo->HalfFFTSize;
	double *buff = fft->FFTOutLogBuff;
	double avg = 0.0;
	int maxHZ = (int)clsGetDataSDR.deviceParams->devParams->fsFreq.fsHz / 4;
	if (clsGetDataSDR.deviceParams->rxChannelA->tunerParams.bwType * 1000 / 4 > maxHZ) {
		maxHZ = maxHZ * 0.8;
	}
	else {
		maxHZ = clsGetDataSDR.deviceParams->rxChannelA->tunerParams.bwType * 1000 / 4;
	}
	N = maxHZ / ((float)AdcDataI->SampleRate / fft->FFTInfo->FFTSize);
	if (N > fft->FFTInfo->HalfFFTSize) N = fft->FFTInfo->HalfFFTSize;
	cls->AverageValueN = N;
	for (int i = 0; i < N; i++) {
		avg += buff[i];
	}
	cls->AverageValueCalcul = avg /= N;

	for (int i = 0; i < N; i++) {
		si[i].have_signal = false;
		if ((buff[i] - avg) > SIGNAL_DIFF) {
			si[i].have_signal = true;
		}
	}
}

void CWinSpectrumScanFFTShow::DrawSignals(HDC hdc)
{
	if (Signals == NULL) return;

	WaitForSingleObject(fft->hMutexDraw, INFINITE);

	HPEN hPenSaved = NULL;
	double Y = 0;
	int X;
	int FFTLength = fft->FFTInfo->HalfFFTSize;
	UINT FFTDrawHeight = FFTHeight - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON;
	int i = 0;
	UINT64 maxHightPix = DBMin * -64 * VScrollZoom;

	int Xstep = HScrollZoom > 1.0 ? HScrollZoom : 1;
	int istep = HScrollZoom < 1.0 ? ((double)1.0 / HScrollZoom) : 1;
	
	SIGNAL_INFO* si = Signals;
	static HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
	hPenSaved = (HPEN)SelectObject(hdc, hPen);

	i = HScrollPos / HScrollZoom;
	Y = (AverageValueCalcul - 1) * -64 * VScrollZoom - VScrollPos;
	Y = BOUND(Y, 0, FFTDrawHeight);
	X = WAVE_RECT_BORDER_LEFT;
	for (i += istep; i < AverageValueN && (i * HScrollZoom - HScrollPos <= WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT); i += istep)
	{
		X += Xstep;
		if (si[i].have_signal == true) {
			MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
			LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y + 100);
			//char s[100];
			//int n = sprintf(s, "%sLog", fft->Flag);
			//DrawText(hdc, (LPCSTR)s, n, &r, NULL);
		}
	}

	Y = AverageValueCalcul * -64 * VScrollZoom - VScrollPos;
	Y = BOUND(Y, 0, FFTDrawHeight);
	MoveToEx(hdc, WAVE_RECT_BORDER_LEFT, WAVE_RECT_BORDER_TOP + Y, NULL);
	LineTo(hdc, WinRect.right - WAVE_RECT_BORDER_RIGHT, WAVE_RECT_BORDER_TOP + Y);

	Y = (AverageValueCalcul + SIGNAL_DIFF) * -64 * VScrollZoom - VScrollPos;
	Y = BOUND(Y, 0, FFTDrawHeight);
	MoveToEx(hdc, WAVE_RECT_BORDER_LEFT, WAVE_RECT_BORDER_TOP + Y, NULL);
	LineTo(hdc, WinRect.right - WAVE_RECT_BORDER_RIGHT, WAVE_RECT_BORDER_TOP + Y);

	int leftN = HScrollPos / HScrollZoom;
	int rightN = (HScrollPos + (WinRect.right - WAVE_RECT_BORDER_LEFT -  WAVE_RECT_BORDER_RIGHT)) / HScrollZoom;
	if (AverageValueN >= leftN && AverageValueN <= rightN) {
		Y = 100;
		X = WAVE_RECT_BORDER_LEFT + (AverageValueN - leftN) * HScrollZoom;
		MoveToEx(hdc, X, WAVE_RECT_BORDER_TOP + Y, NULL);
		LineTo(hdc, X, WAVE_RECT_BORDER_TOP + Y + 100);
	}

	SelectObject(hdc, hPenSaved);

	ReleaseMutex(fft->hMutexDraw);
}


void CWinSpectrumScanFFTShow::P2SubP1(UINT message)
{

	POINT* P;
	P = message == WM_LBUTTONUP ? &ScreenP1 : &ScreenP2;
	P->x = MouseX;
	P->y = MouseY;
	if (P->x > WAVE_RECT_BORDER_LEFT && P->x < WinRect.right - WAVE_RECT_BORDER_RIGHT
		&& P->y > WAVE_RECT_BORDER_TOP && P->y < WAVE_RECT_BORDER_TOP + WAVE_RECT_HEIGHT) {
		if (message == WM_LBUTTONUP) P1_Use = true; else P2_Use = true;
		P->x = (P->x - WAVE_RECT_BORDER_LEFT + HScrollPos) / HScrollZoom;
		if (P->x >= fft->FFTInfo->HalfFFTSize) {
			if (message == WM_LBUTTONUP) P1_Use = false; else P2_Use = false;
		}
	}
	else {
		if (message == WM_LBUTTONUP) P1_Use = false; else P2_Use = false;
	}

	int n = 0, i = 0;
	int X;
	double Y, Ylog, YB, YBlog;
	char sX[200], sHZ[200], sY[200], sYlog[200], sYB[200], sYBlog[200];
	double P1Hz;
	double P2Hz;
	*strPP = '\0';
	bool P1Eanble = false;
	if (P1_Use == true) {
		P = &ScreenP1;
		X = P->x;
		if (X < fft->FFTInfo->HalfFFTSize) {
			Y = fft->FFTOutBuff[X];
			Ylog = fft->FFTOutLogBuff[X];
			YB = fft->FFTBrieflyBuff[X];
			YBlog = fft->FFTBrieflyLogBuff[X];
			n += sprintf(strPP + n, "P1 X=%s, Hz=%s, Y=%s, Ylog=%sdb, YB=%s, YBlog=%sdb\r\n",
				fomatKINT64(X, sX),
				formatKDouble(P1Hz = (double)X * AdcDataI->SampleRate / fft->FFTInfo->FFTSize, 0.001, "", sHZ),
				fomatKINT64(Y, sY),
				formatKDouble(Ylog * 20, 0.001, "", sYlog),
				formatKDouble(YB, 0.001, "", sYB),
				formatKDouble(YBlog * 20, 0.001, "", sYBlog)
			);
			P1Eanble = true;
		}
	}
	bool P2Eanble = false;
	if (P2_Use == true) {
		P = &ScreenP2;
		X = P->x;
		if (X < fft->FFTInfo->HalfFFTSize) {
			Y = fft->FFTOutBuff[X];
			Ylog = fft->FFTOutLogBuff[X];
			YB = fft->FFTBrieflyBuff[X];
			YBlog = fft->FFTBrieflyLogBuff[X];
			n += sprintf(strPP + n, "P2 X=%s, Hz=%s, Y=%s, Ylog=%sdb, YB=%s, YBlog=%sdb\r\n",
				fomatKINT64(X, sX),
				formatKDouble(P2Hz = (double)X * AdcDataI->SampleRate / fft->FFTInfo->FFTSize, 0.001, "", sHZ),
				fomatKINT64(Y, sY),
				formatKDouble(Ylog * 20, 0.001, "", sYlog),
				formatKDouble(YB, 0.001, "", sYB),
				formatKDouble(YBlog * 20, 0.001, "", sYBlog)
			);
			P2Eanble = true;
		}
	}
	if (P1Eanble == true && P2Eanble == true) {
		n += sprintf(strPP + n, "P2 - P1: Hz=%s", formatKDouble(P2Hz - P1Hz, 0.001, "", sYlog));
	}
}
