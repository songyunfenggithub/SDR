
#include "stdafx.h"
#include "Windows.h"
#include "Winuser.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "Debug.h"
#include "CFilter.h"
#include "CData.h"
#include "CFFT.h"

#include "CAudio.h"
#include "CWinFFT.h"
#include "CWinSignal.h"
#include "CWinFiltted.h"

using namespace WINS;
using namespace METHOD;

CWinFiltted::CWinFiltted()
{
	OPENCONSOLE_SAVED;
	Init();
}

CWinFiltted::~CWinFiltted()
{
	UnInit();
	//CLOSECONSOLE;
}

const char FilttedWinTag[] = "CFilttedWin";

void CWinFiltted::Init(void)
{
	RegisterWindowsClass();
	
	m_SignalWin = new CWinSignal();
	m_SignalWin->Flag = FilttedWinTag;
	m_SignalWin->Init(AdcDataIFiltted, NULL, NULL, NULL);

	m_FFTWin = new CWinFFT();
	m_FFTWin->Flag = FilttedWinTag;

	//m_FFTWin->buff_type = BUFF_DATA_TYPE::float_type;
	//m_FFTWin->DataBuff = AdcDataI->FilttedBuff;
	//m_FFTWin->DataBuffPos = &AdcDataI->FilttedBuffPos;
	//m_FFTWin->data_buff_data_bits = ADC_DATA_SAMPLE_BIT;
	//m_FFTWin->data_buff_length_mask = DATA_BUFFER_MASK;

	//m_FFTWin->SampleRate = &clsFilter.rootFilterInfo.SampleRate;

}

void CWinFiltted::UnInit(void)
{
	if (m_FFTWin != NULL) free(m_FFTWin);
	if (m_SignalWin != NULL) free(m_SignalWin);
}

void CWinFiltted::AM_Demodulator(void)
{

}

void CWinFiltted::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinFiltted::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(long);
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_FILTTED;
	wcex.lpszClassName = WIN_CFILTTED_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinFiltted::OpenWindow(void)
{
	if (hWnd == NULL) {
			hWnd = CreateWindow(WIN_CFILTTED_CLASS, "滤波后信号 （Filtted Signal）", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 1400, 1000, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

LRESULT CALLBACK CWinFiltted::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CWinFiltted* me = (CWinFiltted*)get_WinClass(hWnd);
	switch (message)
	{
	case WM_CREATE:
	{
		OPENCONSOLE_SAVED;
		me = (CWinFiltted*)set_WinClass(hWnd, lParam);

		me->hWnd = hWnd;
		HMENU hMenu = GetMenu(hWnd);
		//me->m_SignalWin->hMenu = hMenu;
		//me->m_FFTWin->hMenu = hMenu;
		//me->m_FFTWin->hMenuSpect = GetSubMenu(hMenu, 3);

		//uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		//KillTimer(hWnd, 0);//DrawInfo.uTimerId);

		me->m_FFTWin->Data = AdcDataIFiltted;
		//	FFTInfo_Audio.FFTSize = 2048;
		//	FFTInfo_Audio.HalfFFTSize = FFTInfo_Audio.FFTSize / 2;
		//	FFTInfo_Audio.FFTStep = 2048;
		me->m_FFTWin->fft->FFTInfo = &FFTInfo_Filtted;

		me->m_SignalWin->hWnd = CreateWindow(WIN_SIGNAL_CLASS, "信号", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 200, 500, 200, hWnd, NULL, hInst, me->m_SignalWin);
		ShowWindow(me->m_SignalWin->hWnd, SW_SHOW);
		me->m_FFTWin->hWnd = CreateWindow(WIN_FFT_CLASS, "FFT", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 0, 500, 200, hWnd, NULL, hInst, me->m_FFTWin);
		ShowWindow(me->m_FFTWin->hWnd, SW_SHOW);
	}
	break;
	case WM_SIZE:
	{
		GetClientRect(hWnd, &me->WinRect);
		MoveWindow(me->m_SignalWin->hWnd, 0, 0, me->WinRect.right, me->SignalWinHeight, true);
		MoveWindow(me->m_FFTWin->hWnd, 0, me->SignalWinHeight, me->WinRect.right, me->WinRect.bottom - me->SignalWinHeight, true);
	}
	break;
	case WM_CHAR:
		DbgMsg("WM_CHAR:%c\r\n", wParam);
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
		return me->OnCommand(message, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
		break;
	case WM_DESTROY:
		DbgMsg("CFilttedWin WM_DESTROY\r\n");
		DestroyWindow(me->m_SignalWin->hWnd);
		DestroyWindow(me->m_FFTWin->hWnd);
		me->hWnd = NULL;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

BOOL CWinFiltted::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{

	case IDM_WAVEHZOOMRESET:
	case IDM_WAVEHZOOMINCREASE:
	case IDM_WAVEHZOOMDECREASE:
	case IDM_WAVEVZOOMRESET:
	case IDM_WAVEVZOOMINCREASE:
	case IDM_WAVEVZOOMDECREASE:
	case IDM_WAVEAUTOSCROLL:
	case IDM_WAVE_FOLLOW_ORIGNAL:
		PostMessage(m_SignalWin->hWnd, message, wParam, lParam);
		break;

	case IDM_SPECTRUM_ORIGNAL_SHOW:
	case IDM_SPECTRUM_ORIGNAL_LOG_SHOW:
	case IDM_SPECTRUM_ZOOMED_SHOW:

	case IDM_FFT_HORZ_ZOOM_INCREASE:
	case IDM_FFT_HORZ_ZOOM_DECREASE:
	case IDM_FFT_HORZ_ZOOM_HOME:
	case IDM_FFT_VERT_ZOOM_INCREASE:
	case IDM_FFT_VERT_ZOOM_DECREASE:
	case IDM_FFT_VERT_ZOOM_HOME:
	case IDM_FFT_SET:
		PostMessage(m_FFTWin->hWnd, message, wParam, lParam);
		break;
	case IDM_EXIT:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}
