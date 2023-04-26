
#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "CData.h"
#include "CWaveFFT.h"
#include "CWinSpectrum.h"
#include "CWinOneSpectrum.h"

#include "CFFTWin.h"

using namespace std;

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define DIVLONG		10
#define DIVSHORT	5

CWinOneSpectrum clsWinOneSpectrum;

CWinOneSpectrum::CWinOneSpectrum()
{
	OPENCONSOLE;
	RegisterWindowsClass();
	hDrawMutex		= CreateMutex(NULL, false, "WinOneSpectrumhDrawMutex");
}

CWinOneSpectrum::~CWinOneSpectrum()
{
	//CLOSECONSOLE;
}

void CWinOneSpectrum::RegisterWindowsClass(void)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinOneSpectrum::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = SPECTRUM_ONE_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}


void CWinOneSpectrum::InitDrawBuff(void)
{
	WaitForSingleObject(hDrawMutex, INFINITE);

	SpectrumY = -1;

	RECT rt;
	HDC hDC = GetDC(hWnd);

	GetClientRect(hWnd, &rt);
	rt.top = 0;
	rt.left = 0;
	rt.right = rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	WinHeight = rt.bottom;

	if (hDCFFT) DeleteObject(hDCFFT);
	if (hBMPFFT) DeleteObject(hBMPFFT);
	hDCFFT = CreateCompatibleDC(hDC);
	hBMPFFT = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hDCFFT, hBMPFFT);
	FillRect(hDCFFT, &rt, (HBRUSH)GetStockObject(BLACK_BRUSH));

	ReleaseDC(hWnd, hDC);
	ReleaseMutex(hDrawMutex);
}

LRESULT CALLBACK CWinOneSpectrum::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinOneSpectrum.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinOneSpectrum::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rt;

	switch (message)
	{
	case WM_CREATE:
		{
			OPENCONSOLE;

			clsWinOneSpectrum.hWnd = hWnd;

			uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
			//KillTimer(hWnd, 0);
		}
		break;
	case WM_CHAR:
		printf("WinOneSpectrum WM_CHAR\r\n");
		PostMessage(clsWinSpect.hWnd, message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		//clsWinSpect.ActivehWnd = NULL;
		break;
	case WM_MOUSEMOVE:
		MouseX = GET_X_LPARAM(lParam);
		MouseY = GET_Y_LPARAM(lParam);
		Hz = ((double)(MouseX - WAVE_RECT_BORDER_LEFT) / clsWinSpect.HScrollZoom + clsWinSpect.HScrollPos) * clsData.AdcSampleRate / clsWaveFFT.FFTSize;
		//OnMouse(hWnd);
		break;

	case WM_TIMER:
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;

	case WM_SIZE:
		InitDrawBuff();
		GetRealClientRect(&rt);
		WinWidth = rt.right;
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
	case WM_DESTROY:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CWinOneSpectrum::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
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

void CWinOneSpectrum::Paint(void)
{
	HDC		hDC;
	PAINTSTRUCT ps;
	RECT	rt;

	WaitForSingleObject(hDrawMutex, INFINITE);

	hDC = BeginPaint(hWnd, &ps);

	rt = ps.rcPaint;

	int spectY = SpectrumY + 1;
	int destLeft = rt.left + WAVE_RECT_BORDER_LEFT;
	int destWidth = rt.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	StretchBlt(hDC,
		destLeft, 0,
		destWidth, rt.bottom - spectY,
		hDCFFT,
		0, spectY,
		destWidth, rt.bottom - spectY,
		SRCCOPY);
	if(spectY)
	StretchBlt(hDC,
		destLeft, rt.bottom - spectY,
		destWidth, spectY,
		hDCFFT,
		0, 0,
		destWidth, spectY,
		SRCCOPY);

	//printf("%d,%d,%d,%d\r\n", rt.top, rt.left, rt.right, rt.bottom);

	DeleteObject(hDC);

	EndPaint(hWnd, &ps);

	ReleaseMutex(hDrawMutex);

}

#define FFT_MAX_SAMPLING 16

void CWinOneSpectrum::PaintSpectrum(void)
{
	WaitForSingleObject(clsWinOneSpectrum.hDrawMutex, INFINITE);
	WaitForSingleObject(clsWinSpect.hMutexBuff, INFINITE);

	double* pBuf = clsWinOneSpectrum.bSpectrumZoomedShow == true ? 
		clsWinSpect.BrieflyBuffs[clsWinOneSpectrum.whichSignel] : clsWinSpect.Buffs[clsWinOneSpectrum.whichSignel];

	UINT32 halfFFTSize = clsWaveFFT.HalfFFTSize;
	UINT DrawLen = clsWinOneSpectrum.WinWidth - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
	RECT rt;
	UINT64 color;
	UINT8 c;
	BYTE R, G = 0, C;
	COLORREF cx;

	double fftmaxv;
	double fftminv;


	if (clsWinOneSpectrum.SpectrumY == -1) clsWinOneSpectrum.SpectrumY = clsWinOneSpectrum.WinHeight - 1;

	SelectObject(clsWinOneSpectrum.hDCFFT, CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));

	fftmaxv = pBuf[halfFFTSize];
	fftminv = pBuf[halfFFTSize + 1];
	if ((clsWinOneSpectrum.whichSignel & 1) == 0) {

		if (clsWinOneSpectrum.FFTMaxsPos++ == FFT_MAX_SAMPLING)
		{
			clsWinOneSpectrum.FFTAvgMaxValue = clsWinOneSpectrum.FFTMaxs / FFT_MAX_SAMPLING;
			clsWinOneSpectrum.FFTMaxsPos = 0;
			clsWinOneSpectrum.FFTMaxs = 0.0;
			//printf("pWinData->FFTAvgMaxValue %f\r\n", pWinData->FFTAvgMaxValue);
		}
		clsWinOneSpectrum.FFTMaxs += fftmaxv;
	}

	HBRUSH hbr;
	//hbr = CreateSolidBrush(RGB(c, c, 0));
	//rt.top = y * 2;
	//rt.left = pWinData->x * 2;
	//rt.bottom = rt.top + 2;
	//rt.right = rt.left + 2;
	//FillRect(hdc, &rt, hbr);
	//DeleteObject(hbr);
	int istep = clsWinSpect.HScrollZoom < 1.0 ? ((double)1.0 / clsWinSpect.HScrollZoom) : 1;
	int X;

	double averageV = 0.0;
	double minV = DBL_MAX;
	double maxV = -1.0 * DBL_MAX;
	UINT averagei = 0;


	for (int i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
		i < halfFFTSize &&
		(i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= clsWinOneSpectrum.WinWidth - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
		i += istep) {
		averageV += pBuf[i];
		minV = min(minV, pBuf[i]);
		maxV = max(maxV, pBuf[i]);
		averagei++;
	}
	averageV /= averagei;
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

	if (clsWinOneSpectrum.whichSignel & 1) {
		X = 0;
		for (int i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
			i < halfFFTSize &&
			(i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= clsWinOneSpectrum.WinWidth - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
			i += istep)
		{
			/*
			UINT vcolor = (UINT)(((double)(1.0*0xFFFFFFFF) / fftmaxdiff) * (pBuf[i] - fftminv));
			//color = BOUND((UINT64)0xFFFFFFFF + (UINT64)0xFFFFFFFF / 10 * pBuf[i], 0, (UINT64)0xFFFFFFFF); // adc最大幅度为 2的32次方，也就是10的10次方.
			if (C = (BYTE)(vcolor >> 24)) cx = RGB(255, ~C, ~C);
			else {
				if (C = (BYTE)(vcolor >> 16)) cx = RGB(C, C, 255);
				else {
					if (C = (BYTE)(vcolor >> 8)) cx = RGB(0, ~C, C);
					else {
						C = (BYTE)vcolor;
						cx = RGB(0, C, 0);
					}
				}
			}
			*/
			
			//c = BOUND(0xFF + 0xFF / 10 * pBuf[i], 0, 0xFF); // adc最大幅度为 2的32次方，也就是10的10次方.

			 //c = (BYTE)(0xFF / fftmaxdiff) * (pBuf[i] - fftminv);
			
			//SetPixel(hDCFFT, X, SpectrumY, RGB(~c, ~c, 255));
//			SetPixel(hDCFFT, X, SpectrumY, RGB(~c, ~c, c));
			//SetPixel(hDCFFT, X, SpectrumY, cx);


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

			if (clsWinSpect.HScrollZoom <= 1.0) {
				SetPixel(clsWinOneSpectrum.hDCFFT, X, clsWinOneSpectrum.SpectrumY, cx);
				X++;
			}
			else {
				HPEN hPen = CreatePen(PS_SOLID, 0, cx);
				SelectObject(clsWinOneSpectrum.hDCFFT, hPen);
				MoveToEx(clsWinOneSpectrum.hDCFFT, X - clsWinSpect.HScrollZoom / 2, clsWinOneSpectrum.SpectrumY, NULL);
				LineTo(clsWinOneSpectrum.hDCFFT, X + clsWinSpect.HScrollZoom / 2, clsWinOneSpectrum.SpectrumY);
				X += clsWinSpect.HScrollZoom;
				DeleteObject(hPen);
			}
		}
	}
	else {
		X = 0;

//		for (UINT32 y = 0; y < FFTLen; y++)
		for (int i = clsWinSpect.HScrollPos / clsWinSpect.HScrollZoom;
			i < halfFFTSize &&
			(i * clsWinSpect.HScrollZoom - clsWinSpect.HScrollPos <= clsWinOneSpectrum.WinWidth - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
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
			color = (UINT)(((double)(1.0 * 0xFFFFFFFF) / clsWaveFFT.FFTMaxValue) * (pBuf[i]));
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
			//SetPixel(clsWinOneSpectrum.hDCFFT, X, clsWinOneSpectrum.SpectrumY, cx);
			//X++;
			if (clsWinSpect.HScrollZoom <= 1.0) {
				SetPixel(clsWinOneSpectrum.hDCFFT, X, clsWinOneSpectrum.SpectrumY, cx);
				X++;
			}
			else {
				HPEN hPen = CreatePen(PS_SOLID, 0, cx);
				SelectObject(clsWinOneSpectrum.hDCFFT, hPen);
				MoveToEx(clsWinOneSpectrum.hDCFFT, X - clsWinSpect.HScrollZoom / 2, clsWinOneSpectrum.SpectrumY, NULL);
				LineTo(clsWinOneSpectrum.hDCFFT, X + clsWinSpect.HScrollZoom / 2, clsWinOneSpectrum.SpectrumY);
				X += clsWinSpect.HScrollZoom;
				DeleteObject(hPen);
			}
		}
	}

	DeleteObject(SelectObject(clsWinOneSpectrum.hDCFFT, GetStockObject(SYSTEM_FONT)));

	clsWinOneSpectrum.SpectrumY--;
	clsWinOneSpectrum.PainttedFFT++;

	clsWinOneSpectrum.inPaintSpectrumThread = false;

	ReleaseMutex(clsWinSpect.hMutexBuff);
	ReleaseMutex(clsWinOneSpectrum.hDrawMutex);

}

void CWinOneSpectrum::GetRealClientRect(PRECT lprc)
{
	DWORD dwStyle;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	GetClientRect(hWnd, lprc);
	if (dwStyle & WS_HSCROLL)
		lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (dwStyle & WS_VSCROLL)
		lprc->right += GetSystemMetrics(SM_CXVSCROLL);
}
