
#include "stdafx.h"
#include "Windows.h"
#include "Winuser.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "myDebug.h"
#include "CWaveData.h"
#include "CWaveFFT.h"

#include "CFFTWin.h"
#include "CSignalWin.h"
#include "CAudioWin.h"

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

CAudioWin::CAudioWin()
{
	OPENCONSOLE;
	RegisterWindowsClass();
	Init();
}

CAudioWin::~CAudioWin()
{
	UnInit();
	CLOSECONSOLE;
}

void CAudioWin::Init(void)
{
	m_Audio = new CAudio();

	m_SignalWin = new CSignalWin();

	m_SignalWin->Init(ADC_DATA_SAMPLE_BIT, DATA_BUFFER_LENGTH);
	
	m_SignalWin->OrignalBuff = clsWaveData.AdcBuff;
	m_SignalWin->OrignalBuffPos = &clsWaveData.AdcPos;
	m_SignalWin->orignal_buff_type = short_type;

	m_SignalWin->FilttedBuff = clsWaveData.FilttedBuff;
	m_SignalWin->FilttedBuffPos = &clsWaveData.FilttedPos;
	m_SignalWin->filtted_buff_type = float_type;

	m_FFTWin = new CFFTWin();
	m_FFTWin->buff_type = BUFF_DATA_TYPE::short_type;
	m_FFTWin->DataBuff = clsWaveData.AdcBuff;
	m_FFTWin->DataBuffPos = &clsWaveData.AdcPos;
	m_FFTWin->data_buff_data_bits = 12;
	m_FFTWin->data_buff_length_mask = DATA_BUFFER_MASK;
}

void CAudioWin::UnInit(void)
{
	if (m_Audio != NULL) free(m_Audio);
	if (m_FFTWin != NULL) free(m_FFTWin);
	if (m_SignalWin != NULL) free(m_SignalWin);
}

void CAudioWin::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CAudioWin::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(long);
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_AUDIO;
	wcex.lpszClassName = AUDIO_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

/*
typedef struct tagMyData
{
	void* p;
	// Define creation data here. 
} MYDATA;


typedef struct tagMyDlgData
{
	SHORT   cbExtra;
	MYDATA  myData;
} MYDLGDATA, UNALIGNED* PMYDLGDATA;

MYDLGDATA mydata;
*/

void CAudioWin::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(AUDIO_WIN_CLASS, "声频窗口", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 1400, 1000, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

LRESULT CALLBACK CAudioWin::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	CAudioWin* me;
	
	HGLOBAL hMemProp;
	void* lpMem;
	hMemProp = (HGLOBAL)GetProp(hWnd, "H");
	if (hMemProp) {
		lpMem = GlobalLock(hMemProp);
		//(void*)me = *(void*)lpMem;
		memcpy(&me, lpMem, sizeof(UINT64));
		//me = (CAudioWin*)(*lpMem);
		GlobalUnlock(hMemProp);
	}

	switch (message)
	{
	case WM_CREATE:
	{
		OPENCONSOLE;

		//PMYDLGDATA pMyDlgdata = (PMYDLGDATA)(((LPCREATESTRUCT)lParam)->lpCreateParams);
		//me = (CAudioWin*)pMyDlgdata->myData.p;
		me = (CAudioWin*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
		HGLOBAL hMemProp;
		void* lpMem;
		hMemProp = GlobalAlloc(GPTR, sizeof(INT64));
		lpMem = GlobalLock(hMemProp);
		memcpy(lpMem, &me, sizeof(UINT64));
		GlobalUnlock(hMemProp);
		SetProp(hWnd, "H", hMemProp);

		me->hWnd = hWnd;
		me->hMenu = GetMenu(hWnd);


		//uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		//KillTimer(hWnd, 0);//DrawInfo.uTimerId);

		me->m_SignalWin->hWnd = CreateWindow(SIGNAL_WIN_CLASS, "信号", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 200, 500, 200, hWnd, NULL, hInst, me->m_SignalWin);
		ShowWindow(me->m_SignalWin->hWnd, SW_SHOW);
		me->m_FFTWin->hWnd = CreateWindow(FFT_WIN_CLASS, "FFT", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 0, 500, 200, hWnd, NULL, hInst, me->m_FFTWin);
		ShowWindow(me->m_FFTWin->hWnd, SW_SHOW);
	}
	break;
	case WM_TIMER:
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;
	case WM_SIZE:
	{
		GetClientRect(hWnd, &me->WinRect);
		MoveWindow(me->m_SignalWin->hWnd, 0, 0, me->WinRect.right, me->SignalWinHeight, true);
		MoveWindow(me->m_FFTWin->hWnd, 0, me->SignalWinHeight, me->WinRect.right, me->WinRect.bottom - me->SignalWinHeight, true);
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
		return me->OnCommand(message, wParam, lParam);
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
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
		break;
	case WM_DESTROY:
		DbgMsg("CAudioWin WM_DESTROY\r\n");
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

BOOL CAudioWin::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
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

	case IDM_SPECTRUM_PAUSE_BREAK:
		break;
	case IDM_FFT_SET:
		break;
	case IDM_SPECTRUM_HORZ_ZOOM_INCREASE:
		break;
	case IDM_SPECTRUM_HORZ_ZOOM_DECREASE:
		break;
	case IDM_SPECTRUM_HORZ_ZOOM_HOME:
		break;
	case IDM_SPECTRUM_VERT_ZOOM_INCREASE:
		break;
	case IDM_SPECTRUM_VERT_ZOOM_DECREASE:
		break;
	case IDM_SPECTRUM_VERT_ZOOM_HOME:
		break;
	case IDM_SPECTRUM_FOLLOW:
		break;
	case IDM_FFT_HORZ_ZOOM_INCREASE:
	case IDM_FFT_HORZ_ZOOM_DECREASE:
	case IDM_FFT_HORZ_ZOOM_HOME:
	case IDM_FFT_VERT_ZOOM_INCREASE:
	case IDM_FFT_VERT_ZOOM_DECREASE:
	case IDM_FFT_VERT_ZOOM_HOME:
		PostMessage(m_FFTWin->hWnd, message, wParam, lParam);
		break;
	case IDM_EXIT:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}