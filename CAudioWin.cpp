
#include "stdafx.h"
#include "Windows.h"
#include "Winuser.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "myDebug.h"
#include "CData.h"
#include "CWaveFFT.h"

#include "CWinMain.h"
#include "CDemodulatorAM.h"
#include "CAudio.h"
#include "CFFTWin.h"
#include "CSignalWin.h"
#include "CAudioWin.h"

CAudioWin::CAudioWin()
{
	OPENCONSOLE;
	Init();
}

CAudioWin::~CAudioWin()
{
	UnInit();
	//CLOSECONSOLE;
}

const char AudioWinTag[] = "CAudioWin";

void CAudioWin::Init(void)
{
	RegisterWindowsClass();

	m_Audio = &mAudio;

	m_SignalWin = new CSignalWin();
	m_SignalWin->Tag = AudioWinTag;
	m_SignalWin->Init(SOUNDCARD_BUFF_DATA_BIT, SOUNDCARD_BUFF_LENGTH);
	m_SignalWin->OrignalBuff = m_Audio->outBuff;
	m_SignalWin->OrignalBuffPos = &m_Audio->outBuffPos;
	m_SignalWin->orignal_buff_type = short_type;
	m_SignalWin->SampleRate = &m_Audio->SampleRate;

	m_SignalWin->FilttedBuff = m_Audio->inBuff;
	m_SignalWin->FilttedBuffPos = &m_Audio->inBuffPos;
	m_SignalWin->filtted_buff_type = short_type;

	m_FFTWin = new CFFTWin();
	m_FFTWin->Tag = AudioWinTag;
	m_FFTWin->buff_type = BUFF_DATA_TYPE::short_type;
	m_FFTWin->DataBuff = m_Audio->outBuff;
	m_FFTWin->DataBuffPos = &m_Audio->outBuffPos;
	m_FFTWin->data_buff_data_bits = SOUNDCARD_BUFF_DATA_BIT;
	m_FFTWin->data_buff_length_mask = SOUNDCARD_BUFF_LENGTH_MASK;
	m_FFTWin->SampleRate = &m_Audio->SampleRate;
	m_FFTWin->FFTSize = 2048;
	m_FFTWin->HalfFFTSize = m_FFTWin->FFTSize / 2;
	m_FFTWin->FFTStep = 2048;


	m_DemodulatorAM = new CDemodulatorAM();
}

void CAudioWin::UnInit(void)
{
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
	CAudioWin* me = (CAudioWin*)get_WinClass(hWnd);
	
	switch (message)
	{
	case WM_CREATE:
	{
		OPENCONSOLE;
		me = (CAudioWin*)set_WinClass(hWnd, lParam);

		me->hWnd = hWnd;
		me->hMenu = GetMenu(hWnd);
		me->m_SignalWin->hMenu = me->hMenu;
		me->m_FFTWin->hMenu = me->hMenu;
		me->m_FFTWin->hMenuSpect = GetSubMenu(me->hMenu, 8);

		me->m_SignalWin->hWnd = CreateWindow(SIGNAL_WIN_CLASS, "信号", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 200, 500, 200, hWnd, NULL, hInst, me->m_SignalWin);
		ShowWindow(me->m_SignalWin->hWnd, SW_SHOW);
		me->m_FFTWin->hWnd = CreateWindow(FFT_WIN_CLASS, "FFT", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
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
		me->m_DemodulatorAM->AM_Demodulator_Doing = false;
		while (me->m_DemodulatorAM->h_AM_Demodulator_Thread != NULL);

		DestroyWindow(me->m_SignalWin->hWnd);
		DestroyWindow(me->m_FFTWin->hWnd);
		me->hWnd = NULL;

		DbgMsg("CAudioWin WM_DESTROY\r\n");
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

bool CAudioWin::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDM_DEMODULATOR_AM:
		m_DemodulatorAM->AM_Demodulator_Doing = false;
		bDemodulatorAM = !bDemodulatorAM;
		CheckMenuItem(hMenu, IDM_SPECTRUM_ZOOMED_SHOW,
			(bDemodulatorAM ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		if (bDemodulatorAM == true) {
			while (m_DemodulatorAM->h_AM_Demodulator_Thread != NULL);
			m_DemodulatorAM->h_AM_Demodulator_Thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDemodulatorAM::AM_Demodulator_Thread, m_DemodulatorAM, 0, NULL);
		}
		break;
	case IDM_STARTPLAY:
		m_Audio->StartOpenOut();
		break;
	case IDM_STOPPLAY:
		m_Audio->StopOpenOut();
		break;
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