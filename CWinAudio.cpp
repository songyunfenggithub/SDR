
#include "stdafx.h"
#include "Windows.h"
#include "Winuser.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "Public.h"
#include "Debug.h"
#include "CData.h"
#include "CFFT.h"
#include "CFilter.h"
#include "cuda_CFilter.cuh"
#include "cuda_CFilter2.cuh"
#include "CAudio.h"

#include "CWinMain.h"
#include "CAM.h"
#include "CFM.h"
#include "CWinFilter.h"
#include "CWinFFT.h"
#include "CWinSignal.h"
#include "CWinAudio.h"
#include "CWinTools.h"

using namespace WINS;
using namespace METHOD;

#define TRACK_MIN	0
#define TRACK_MAX	100

#define AUDIO_COLOR					::COLOR_PEN::Pen_Green
#define AUDIO_COLOR_LOG				::COLOR_PEN::Pen_Yellow
#define AUDIO_COLOR_FILTTED			::COLOR_PEN::Pen_Red
#define AUDIO_COLOR_FILTTED_LOG		::COLOR_PEN::Pen_Blue


CWinAudio::CWinAudio()
{
	OPENCONSOLE_SAVED;
	Init();
}

CWinAudio::~CWinAudio()
{
	UnInit();
	//CLOSECONSOLE;
}

const char AudioWinTag[] = "CAudioWin";

void CWinAudio::Init(void)
{
	RegisterWindowsClass();

	m_Audio = &mAudio;

	m_FilterWin = new CWinFilter();
	m_FilterWin->cFilter = &clsAudioFilter;

	m_SignalWin = new CWinSignal();
	m_SignalWin->Flag = AudioWinTag;
	
	m_FFTWin = new CWinFFT();
	m_FFTWin->Flag = AudioWinTag;

	m_AM = new CAM();
	m_FM = new CFM();
}

void CWinAudio::UnInit(void)
{
	if (m_FFTWin != NULL) free(m_FFTWin);
	if (m_SignalWin != NULL) free(m_SignalWin);
}

void CWinAudio::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinAudio::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(long);
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_AUDIO;
	wcex.lpszClassName = WIN_AUDIO_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinAudio::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(WIN_AUDIO_CLASS, "声频窗口", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 1400, 1000, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

LRESULT CALLBACK CWinAudio::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CWinAudio* me = (CWinAudio*)get_WinClass(hWnd);

	switch (message)
	{
	case WM_CREATE:
	{
		OPENCONSOLE_SAVED;
		me = (CWinAudio*)set_WinClass(hWnd, lParam);

		me->hWnd = hWnd;
		me->hMenu = GetMenu(hWnd);
		//me->m_SignalWin->hMenu = me->hMenu;
		//me->m_FFTWin->hMenu = me->hMenu;
		//me->m_FFTWin->hMenuSpect = GetSubMenu(me->hMenu, 8);

		me->m_SignalWin->Init(me->m_Audio->outData, me->m_Audio->outDataFiltted, NULL, NULL);
		
		me->m_Audio->outData->hPen = Pens[AUDIO_COLOR];
		me->m_Audio->outDataFiltted->hPen = Pens[AUDIO_COLOR_FILTTED];

		me->m_FFTWin->Data = me->m_Audio->outData;
		FFTInfo_Audio.FFTSize = 2048;
		FFTInfo_Audio.HalfFFTSize = FFTInfo_Audio.FFTSize / 2;
		FFTInfo_Audio.FFTStep = 2048;
		FFTInfo_Audio.AverageDeep = FFT_DEEP;
		me->m_FFTWin->fft = new CFFT("Audio", &FFTInfo_Audio);
		me->m_FFTWin->fft->hPen = Pens[AUDIO_COLOR];
		me->m_FFTWin->fft->hPenLog = Pens[AUDIO_COLOR_LOG];

		me->m_FFTWin->Data2 = me->m_Audio->outDataFiltted;
		FFTInfo_AudioFiltted.FFTSize = 2048;
		FFTInfo_AudioFiltted.HalfFFTSize = FFTInfo_AudioFiltted.FFTSize / 2;
		FFTInfo_AudioFiltted.FFTStep = 2048;
		FFTInfo_AudioFiltted.AverageDeep = FFT_DEEP;
		me->m_FFTWin->fft2 = new CFFT("Audio_F", &FFTInfo_AudioFiltted);
		me->m_FFTWin->fft2->hPen = Pens[AUDIO_COLOR_FILTTED];
		me->m_FFTWin->fft2->hPenLog = Pens[AUDIO_COLOR_FILTTED_LOG];

		me->m_FFTWin->isDrawBriefly = false;
		me->m_FFTWin->isSpectrumZoomedShow = false;

		clsAudioFilter.SrcData = me->m_Audio->outData;
		clsAudioFilter.TargetData = me->m_Audio->outDataFiltted;
		clsAudioFilter.set_cudaFilter(&clscudaAudioFilter, &clscudaAudioFilter2, NULL, CUDA_FILTER_AUDIO_BUFF_SRC_LENGTH);
		clsAudioFilter.ParseCoreDesc();
		clsAudioFilter.Scale = &me->m_Audio->Am_zoom;

		me->m_Audio->SampleRate = &AdcDataIFiltted->SampleRate;

		clsAudioFilter.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFilter::cuda_filter_thread, &clsAudioFilter, 0, NULL);

		me->hWndRebar = me->MakeToolsBar();

		me->m_SignalWin->hWnd = CreateWindow(WIN_SIGNAL_CLASS, "信号", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 200, 500, 200, hWnd, NULL, hInst, me->m_SignalWin);
		ShowWindow(me->m_SignalWin->hWnd, SW_SHOW);
		me->m_FFTWin->hWnd = CreateWindow(WIN_FFT_CLASS, "FFT", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 0, 500, 200, hWnd, NULL, hInst, me->m_FFTWin);
		ShowWindow(me->m_FFTWin->hWnd, SW_SHOW);

		me->hMenuFollow = GetSubMenu(me->hMenu, 6);
		CheckMenuRadioItem(me->hMenuFollow, 10, 11, 10, MF_BYPOSITION);

		me->hMenuSpectrum = GetSubMenu(me->hMenu, 8);
		CheckMenuRadioItem(me->hMenuSpectrum, 0, 3, 1, MF_BYPOSITION);

		CheckMenuItem(me->hMenu, IDM_WAVEAUTOSCROLL, (me->m_SignalWin->bAutoScroll ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		CheckMenuItem(me->hMenu, IDM_WAVE_IDATA_SHOW, (me->m_SignalWin->bDrawDataI ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(me->hMenu, IDM_WAVE_IDATA_FILTTED_SHOW, (me->m_SignalWin->bDrawDataI_Filtted ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		CheckMenuItem(me->hMenu, IDM_FFT_ORIGNAL_SHOW, (me->m_FFTWin->fft->bShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(me->hMenu, IDM_FFT_ORIGNAL_LOG_SHOW, (me->m_FFTWin->fft->bLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(me->hMenu, IDM_FFT_FILTTED_SHOW, (me->m_FFTWin->fft2->bShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(me->hMenu, IDM_FFT_FILTTED_LOG_SHOW, (me->m_FFTWin->fft2->bLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		me->bAM = false;
		me->bFM = false;

	}
	break;
	case WM_NOTIFY:
		me->DoNotify(hWnd, message, wParam, lParam);
		break;
	case WM_HSCROLL:
		me->TBNotifications(wParam);
		break;
	case WM_SIZE:
	{
		GetClientRect(hWnd, &me->WinRect);
		RECT rt;
		GetClientRect(me->hWndRebar, &rt);
		DbgMsg("rt.bottom: %d\r\n", rt.bottom);
		MoveWindow(me->hWndRebar, 0, 0, me->WinRect.right, rt.bottom, true);
		MoveWindow(me->m_SignalWin->hWnd, 0, rt.bottom, me->WinRect.right, me->SignalWinHeight, true);
		MoveWindow(me->m_FFTWin->hWnd, 0, me->SignalWinHeight + rt.bottom, me->WinRect.right, me->WinRect.bottom - me->SignalWinHeight - rt.bottom, true);
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
		me->m_AM->Doing = false;
		//while (me->m_DemodulatorAM->h_AM_Demodulator_Thread != NULL);

		me->m_Audio->StopOut();

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

bool CWinAudio::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDM_DEMODULATOR_AM:
		m_AM->Doing = false;
		bAM = !bAM;
		CheckMenuItem(hMenu, IDM_SPECTRUM_ZOOMED_SHOW, (bAM ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		if (bAM == true) {
			while (m_AM->hThread != NULL);
			m_AM->hThread =	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CAM::AM_Demodulator_Thread, m_AM, 0, NULL);
		}
		break;
	case IDM_DEMODULATOR_FM:
		m_FM->Doing = false;
		bFM = !bFM;
		CheckMenuItem(hMenu, IDM_SPECTRUM_ZOOMED_SHOW,
			(bFM ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		if (bFM == true) {
			while (m_FM->hThread != NULL);
			m_FM->hThread =
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFM::Demodulator_Thread, m_FM, 0, NULL);
		}
		break;
	case IDM_STARTPLAY:
	{
		DbgMsg("IDM_STARTPLAY\r\n");
		//break;
		if (m_Audio->boutOpened == true) {
			m_Audio->StopOut();
		}
		else {
			m_Audio->StartOut();
			m_Audio->Doing = false;
			while (m_Audio->hThread != NULL);
			m_Audio->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CAudio::Thread_Audio_Out, m_Audio, 0, NULL);
		}
	}
	break;
	case IDM_STOPPLAY:
		m_Audio->StopOut();
		break;
	case IDM_AUDIO_BUILD_FILTER:
		BuildAudioFilter();
		break;
	case IDM_AUDIO_CLEAR_FILTER_POINTS:
		ClearAudioFilterPoints();
		break;

	case IDM_AM_MOVE_FREQUENCY:
	{
		m_AM->AmMode = CAM::AM_DEMODULATOR_MODE::Am_Move_Frequency_Mode;
		if (bAM) {
			bAM = false;
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_DEMODULATOR_AM, NULL);
		}
		char str[100];
		GetMenuString(LoadMenu(hInst, MAKEINTRESOURCE(IDC_MENU_POPUP_AM)), IDM_AM_MOVE_FREQUENCY, str, 100, MF_BYCOMMAND);
		clsWinTools.RelabelButton(hWndToolBar, IDM_DEMODULATOR_AM, str);
	}
	break;
	case IDM_AM_IQ:
	{
		m_AM->AmMode = CAM::AM_DEMODULATOR_MODE::Am_IQ_Mode;
		if (bAM) {
			bAM = false;
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_DEMODULATOR_AM, NULL);
		}
		char str[100];
		GetMenuString(LoadMenu(hInst, MAKEINTRESOURCE(IDC_MENU_POPUP_AM)), IDM_AM_IQ, str, 100, MF_BYCOMMAND);
		clsWinTools.RelabelButton(hWndToolBar, IDM_DEMODULATOR_AM, str);
	}
	break;
	case IDM_AM_GET_ENVELOPE:
	{
		m_AM->AmMode = CAM::AM_DEMODULATOR_MODE::Am_Get_Envelope_Mode;
		if (bAM) {
			bAM = false;
			SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_DEMODULATOR_AM, NULL);
		}
		char str[100];
		GetMenuString(LoadMenu(hInst, MAKEINTRESOURCE(IDC_MENU_POPUP_AM)), IDM_AM_GET_ENVELOPE, str, 100, MF_BYCOMMAND);
		clsWinTools.RelabelButton(hWndToolBar, IDM_DEMODULATOR_AM, str);
	}
	break;

	case IDM_WAVEHZOOMRESET:
	case IDM_WAVEHZOOMINCREASE:
	case IDM_WAVEHZOOMDECREASE:
	case IDM_WAVEVZOOMRESET:
	case IDM_WAVEVZOOMINCREASE:
	case IDM_WAVEVZOOMDECREASE:
		PostMessage(m_SignalWin->hWnd, message, wParam, lParam);
		break;
	case IDM_WAVEAUTOSCROLL:
		SendMessage(m_SignalWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_WAVEAUTOSCROLL, (m_SignalWin->bAutoScroll ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_WAVE_FOLLOW_IDATA:
		CheckMenuRadioItem(hMenuFollow, 10, 11, 10, MF_BYPOSITION);
		PostMessage(m_SignalWin->hWnd, message, wParam, lParam);
		break;
	case IDM_WAVE_FOLLOW_FILTTED_IDATA:
		CheckMenuRadioItem(hMenuFollow, 10, 11, 11, MF_BYPOSITION);
		PostMessage(m_SignalWin->hWnd, message, wParam, lParam);
		break;

	case IDM_WAVE_IDATA_SHOW:
		SendMessage(m_SignalWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_WAVE_IDATA_SHOW, (m_SignalWin->bDrawDataI ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_WAVE_IDATA_FILTTED_SHOW:
		SendMessage(m_SignalWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_WAVE_IDATA_FILTTED_SHOW, (m_SignalWin->bDrawDataI_Filtted ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;

	case IDM_SPECTRUM_ORIGNAL_SHOW:
		SendMessage(m_FFTWin->hWnd, message, wParam, lParam);
		CheckMenuRadioItem(hMenuSpectrum, 0, 3, 0, MF_BYPOSITION);
		break;
	case IDM_SPECTRUM_ORIGNAL_LOG_SHOW:
		SendMessage(m_FFTWin->hWnd, message, wParam, lParam);
		CheckMenuRadioItem(hMenuSpectrum, 0, 3, 1, MF_BYPOSITION);
		break;
	case IDM_SPECTRUM_FILTTED_SHOW:
		SendMessage(m_FFTWin->hWnd, message, wParam, lParam);
		CheckMenuRadioItem(hMenuSpectrum, 0, 3, 2, MF_BYPOSITION);
		break;
	case IDM_SPECTRUM_FILTTED_LOG_SHOW:
		SendMessage(m_FFTWin->hWnd, message, wParam, lParam);
		CheckMenuRadioItem(hMenuSpectrum, 0, 3, 3, MF_BYPOSITION);
		break;
	case IDM_SPECTRUM_ZOOMED_SHOW:
		SendMessage(m_FFTWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_SPECTRUM_ZOOMED_SHOW, (m_FFTWin->isSpectrumZoomedShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;

	case IDM_FFT_HORZ_ZOOM_INCREASE:
	case IDM_FFT_HORZ_ZOOM_DECREASE:
	case IDM_FFT_HORZ_ZOOM_HOME:
	case IDM_FFT_VERT_ZOOM_INCREASE:
	case IDM_FFT_VERT_ZOOM_DECREASE:
	case IDM_FFT_VERT_ZOOM_HOME:
		PostMessage(m_FFTWin->hWnd, message, wParam, lParam);
		break;

	case IDM_FFT_ORIGNAL_SHOW:
		SendMessage(m_FFTWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_SHOW, (m_FFTWin->fft->bShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_ORIGNAL_LOG_SHOW:
		SendMessage(m_FFTWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_FFT_ORIGNAL_LOG_SHOW, (m_FFTWin->fft->bLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_FILTTED_SHOW:
		SendMessage(m_FFTWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_SHOW, (m_FFTWin->fft2->bShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_FILTTED_LOG_SHOW:
		SendMessage(m_FFTWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_FFT_FILTTED_LOG_SHOW, (m_FFTWin->fft2->bLogShow ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_FFT_SET:
		DialogBoxParam(hInst, (LPCTSTR)IDD_DLG_FFT_SET, hWnd, (DLGPROC)DlgFFTSetProc, (LPARAM)this);
		break;
	case IDM_AUDIO_FILTER_SET:
		m_FilterWin->OpenWindow();
		break;
	case IDM_EXIT:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

void CWinAudio::BuildAudioFilter(void)
{
	CWinFFT::POIINT_SERIAL*cp2, * cp = m_FFTWin->FilterPsHead;
	char str[2000];
	int n = sprintf(str, "513, 0, 0; ");
	if (cp != NULL) cp2 = cp->next;
	float HzScale = (float)m_FFTWin->Data2->SampleRate / m_FFTWin->fft2->FFTInfo->FFTSize;
	while (cp2 != NULL) {
		n += sprintf(str + n, "2, %u, %u; ", (UINT)(HzScale * (cp->P.x + (cp2->P.x - cp->P.x) / 2)), (UINT)(HzScale *(cp2->P.x - cp->P.x)));
		cp2 = cp2->next;
		if (cp2 != NULL) {
			cp = cp2;
			cp2 = cp2->next;
		}
	}
	str[n - 2] = '\0';
	DbgMsg("Filter Desc: %s\r\n", str);
	if (clsAudioFilter.CheckCoreDesc(str) == false) return;
	clsAudioFilter.setFilterCoreDesc(&clsAudioFilter.rootFilterInfo1, str);
	clsAudioFilter.ParseCoreDesc();
}

void CWinAudio::ClearAudioFilterPoints(void)
{
	CWinFFT::POIINT_SERIAL *tp, *cp = m_FFTWin->FilterPsHead;
	if (cp == NULL) return;
	cp = cp->next;
	if (cp == NULL) return;
	while (cp->next != NULL) {
		tp = cp;
		cp = cp->next;
		free(tp);
	}
	m_FFTWin->FilterPsHead->next = cp;
}

LRESULT CALLBACK CWinAudio::DlgFilterCoreProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CWinAudio* me = (CWinAudio*)get_DlgWinClass(hDlg);
	switch (message)
	{
	case WM_INITDIALOG:
		me = (CWinAudio*)set_DlgWinClass(hDlg, lParam);

		//DWORD dwPos = GetDlgItemInt(hDlg, IDC_EDITGOTO, 0, 0);
		//SetDlgItemInt(hDlg, IDC_EDIT_PLAY_STOP_POSITION,	clsSoundCard.dwPlayStopPosition, TRUE);
		//SetDlgItemText(hDlg, IDC_EDIT1, me->rootFilterInfo->CoreDescStr);
		//SetDlgItemInt(hDlg, IDC_EDIT2, me->rootFilterInfo->decimationFactorBit, TRUE);
		//CheckDlgButton(hDlg, IDC_CHECK1, TRUE);
		//SetDlgItemText(hDlg, IDC_STATIC1, filterComment);
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if (LOWORD(wParam) == IDOK)
			{
				//char s[8192];
				//GetDlgItemText(hDlg, IDC_EDIT1, s, FILTER_CORE_DESC_MAX_LENGTH);
				//UINT decimationFactor = GetDlgItemInt(hDlg, IDC_EDIT2, NULL, false);
				//IsDlgButtonChecked(hDlg, IDC_CHECK1) == true ?
					//GetDlgItemInt(hDlg, IDC_EDIT2, NULL, TRUE) : AdcDataI->SampleRate;

				//clsMainFilter.doFiltting = false;
				//while (clsMainFilter.cuda_Filter_exit == false);

				//clsMainFilter.rootFilterInfo.decimationFactorBit = decimationFactor;
				//AdcDataIFiltted->SampleRate = AdcDataI->SampleRate / (1 << clsMainFilter.rootFilterInfo.decimationFactorBit);

				//FFTInfo_Filtted.FFTSize = FFTInfo_Signal.FFTSize >> decimationFactor;
				//FFTInfo_Filtted.HalfFFTSize = FFTInfo_Signal.HalfFFTSize >> decimationFactor;
				//FFTInfo_Filtted.FFTStep = FFTInfo_Signal.FFTStep >> decimationFactor;
				//clsWinSpect.FFTFiltted->Init();

				//clsMainFilter.setFilterCoreDesc(me->rootFilterInfo, s);
				//clsMainFilter.ParseCoreDesc(me->rootFilterInfo);
				//me->set_CoreAnalyse_root_Filter(me->rootFilterInfo);
			}
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

#define TOOLS_BTN_PLAY_STOP_ID			0

#define TOOLS_BTN_DEMODULATOR_AM_ID		1
#define TOOLS_BTN_DEMODULATOR_FM_ID		2

#define TOOLS_BTN_AUDIO_VLAUE_TRACKBAR_ID			3

HWND CWinAudio::MakeToolsBar(void)
{
	hWndRebar = clsWinTools.CreateRebar(hWnd);
	static TBBUTTON tbb[8] = {
		{ MAKELONG(1, 0), IDM_STARTPLAY, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_CHECK, {0}, 0, (INT_PTR)L"Play" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP, {0}, 0, NULL }, 
		{ MAKELONG(2, 0), IDM_DEMODULATOR_AM, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_CHECK | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"AM_Move_Frequency" },
		{ MAKELONG(2, 0), IDM_DEMODULATOR_FM, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_CHECK,	{0}, 0, (INT_PTR)L"FM" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP, {0}, 0, NULL }, 
		{ MAKELONG(3, 0), IDM_AUDIO_BUILD_FILTER, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"建立滤波器" },
		{ MAKELONG(3, 0), IDM_AUDIO_CLEAR_FILTER_POINTS, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"清除滤波器设置点" },
		{ MAKELONG(0, 0), 0, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN,	{0}, 0, (INT_PTR)L"下拉" }
	};
	CWinTools::TOOL_TIPS tips[8] = {
		{ IDM_STARTPLAY,"开始 / 停止 播放音频信号." },
		{ 0, NULL },
		{ IDM_DEMODULATOR_AM,"开启 / 关闭 调幅解码." },
		{ IDM_DEMODULATOR_FM, "开启 / 关闭 调频解码." },
		{ 0, NULL },
		{ IDM_AUDIO_BUILD_FILTER,"建立滤波器." },
		{ IDM_AUDIO_CLEAR_FILTER_POINTS, "清除滤波器设置点." },
		{ 0, "TBSTYLE_DROPDOWN" }
	};
	hWndToolBar = clsWinTools.CreateToolbar(hWnd, tbb, 8, tips, 8);

	// Add images
	TBADDBITMAP tbAddBmp = { 0 };
	tbAddBmp.hInst = HINST_COMMCTRL;
	tbAddBmp.nID = IDB_STD_SMALL_COLOR;
	SendMessage(hWndToolBar, TB_ADDBITMAP, 0, (WPARAM)&tbAddBmp);

	clsWinTools.CreateRebarBand(hWndRebar, "BTN", 1, 500, 0, hWndToolBar);

	hWndTrack = CreateTrackbar(hWnd, 0, 100, 10);
	clsWinTools.CreateRebarBand(hWndRebar, "Value", 2, 0, 0, hWndTrack);
	return hWndRebar;
}

HWND CWinAudio::CreateTrackbar(HWND hWnd, UINT iMin, UINT iMax, UINT pos)
{
	hWndTrack = CreateWindowEx(
		0, // no extended styles     Using Trackbar Controls
		TRACKBAR_CLASS, // class name
		"Trackbar Control", // title (caption)
		WS_CHILD
		| WS_VISIBLE
		//| TBS_NOTICKS
		| TBS_AUTOTICKS
		//| TBS_ENABLESELRANGE
		, // style
		10, 100, // position
		1000, 16, // size
		hWnd, // parent window
		(HMENU)TOOLS_BTN_AUDIO_VLAUE_TRACKBAR_ID, // control identifier
		hInst, // instance
		NULL // no WM_CREATE parameter
	);
	SendMessage(hWndTrack, TBM_SETRANGE,
		(WPARAM)TRUE, // redraw flag
		(LPARAM)MAKELONG(iMin, iMax)); // min. & max. positions
	SendMessage(hWndTrack, TBM_SETPAGESIZE,
		0, (LPARAM)5); // new page size
	SendMessage(hWndTrack, TBM_SETSEL,
		(WPARAM)FALSE, // redraw flag
		(LPARAM)MAKELONG(iMin, iMax));
	SendMessage(hWndTrack, TBM_SETPOS,
		(WPARAM)TRUE, // redraw flag
		(LPARAM)pos);
	SendMessage(hWndTrack, TBM_SETTICFREQ,
		(WPARAM)5, // redraw flag
		(LPARAM)0);

	//SetFocus(hWndTrack);
	return hWndTrack;
}

VOID CWinAudio::TBNotifications(WPARAM wParam)
{
	DWORD dwPos; 
	switch (LOWORD(wParam)) {
	case TB_THUMBTRACK:
		dwPos = SendMessage(hWndTrack, TBM_GETPOS, 0, 0);
		if (dwPos > TRACK_MAX)
			SendMessage(hWndTrack, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)TRACK_MAX);
		else if (dwPos < TRACK_MIN)
			SendMessage(hWndTrack, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)TRACK_MIN);

		DbgMsg("TBNotifications:%d\r\n", dwPos);

		m_Audio->Am_zoom = dwPos;
		
		break;
	default:
		break;
	}
}

BOOL CWinAudio::DoNotify(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define lpnm    ((LPNMHDR)lParam)
	switch (lpnm->code)
	{
	case TBN_DROPDOWN:
	{
#define lpnmTB  ((LPNMTOOLBAR)lParam)
		// Get the coordinates of the button.
		RECT rc = { 0 };
		SendMessage(lpnmTB->hdr.hwndFrom, TB_GETRECT, (WPARAM)lpnmTB->iItem, (LPARAM)&rc);
		// Convert to screen coordinates.
		MapWindowPoints(lpnmTB->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);
		// Get the menu.
		HMENU hMenuLoaded = LoadMenu(hInst, MAKEINTRESOURCE(lpnmTB->iItem == IDM_DEMODULATOR_AM ? IDC_MENU_POPUP_AM : IDC_MENUMAIN));
		// Get the submenu for the first menu item.
		HMENU hPopupMenu = GetSubMenu(hMenuLoaded, 0);
		// Set up the pop-up menu.
		// In case the toolbar is too close to the bottom of the screen,
		// set rcExclude equal to the button rectangle and the menu will appear above
		// the button, and not below it.
		TPMPARAMS tpm = { 0 };
		tpm.cbSize = sizeof(TPMPARAMS);
		tpm.rcExclude = rc;
		// Show the menu and wait for input. Using Toolbar Controls Windows common controls demo(CppWindowsCommonControls)
		// If the user selects an item, its WM_COMMAND is sent.
		TrackPopupMenuEx(hPopupMenu,
			TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
			rc.left, rc.bottom, hWnd, &tpm);
		DestroyMenu(hMenuLoaded);

		DbgMsg("TBN_DROPDOWN %d, %d\r\n", lpnmTB->iItem, IDM_DEMODULATOR_AM);
		return FALSE;
	}
	}
	return FALSE;
}

LRESULT CALLBACK CWinAudio::DlgFFTSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CWinAudio* me = (CWinAudio*)get_DlgWinClass(hDlg);

	switch (message)
	{
	case WM_INITDIALOG:
		me = (CWinAudio*)set_DlgWinClass(hDlg, lParam);
		//DWORD dwPos = GetDlgItemInt(hDlg, IDC_EDITGOTO, 0, 0);
		//SetDlgItemInt(hDlg, IDC_EDIT_PLAY_STOP_POSITION,	clsSoundCard.dwPlayStopPosition, TRUE);
		SetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, me->m_FFTWin->fft->FFTInfo->FFTSize, false);
		SetDlgItemInt(hDlg, IDC_EDIT_FFT_STEP, me->m_FFTWin->fft->FFTInfo->FFTStep, false);
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if (LOWORD(wParam) == IDOK)
			{
				UINT fftsize = GetDlgItemInt(hDlg, IDC_EDIT_FFT_SIZE, NULL, false);
				UINT fftstep = GetDlgItemInt(hDlg, IDC_EDIT_FFT_STEP, NULL, false);
				int bit;
				for (bit = 0; fftsize > (1 << bit); bit++);
				fftsize = 1 << bit;
				for (bit = 0; fftstep > (1 << bit); bit++);
				fftstep = 1 << bit;

				int halffftsize = fftsize / 2;

				me->m_FFTWin->HScrollPos = (me->m_FFTWin->HScrollPos * (halffftsize / me->m_FFTWin->fft->FFTInfo->HalfFFTSize));
				me->m_FFTWin->HScrollRange = halffftsize * me->m_FFTWin->HScrollZoom - (me->m_FFTWin->WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT);
				UP_TO_ZERO(me->m_FFTWin->HScrollRange);
				SetScrollRange(me->m_FFTWin->hWnd, SB_HORZ, 0, me->m_FFTWin->HScrollRange, false);
				SetScrollPos(me->m_FFTWin->hWnd, SB_HORZ, me->m_FFTWin->HScrollPos, true);

				me->m_FFTWin->fft->Init(fftsize, fftstep, me->m_FFTWin->fft->FFTInfo->AverageDeep);
				me->m_FFTWin->fft2->Init(fftsize, fftstep, me->m_FFTWin->fft2->FFTInfo->AverageDeep);
			}
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
