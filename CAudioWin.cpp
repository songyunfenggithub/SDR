
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
#include "CFFT.h"
#include "CFilter.h"
#include "cuda_CFilter.cuh"
#include "cuda_CFilter2.cuh"
#include "CAudio.h"

#include "CWinMain.h"
#include "CDemodulatorAM.h"
#include "CFilterWin.h"
#include "CFFTWin.h"
#include "CSignalWin.h"
#include "CAudioWin.h"
#include "CToolsWin.h"

using namespace WINS;
using namespace METHOD;

#define TRACK_MIN	0
#define TRACK_MAX	100

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

	m_FilterWin = new CFilterWin();
	m_FilterWin->cFilter = &clsAudioFilter;
	m_FilterWin->rootFilterInfo1 = &clsAudioFilter.rootFilterInfo1;

	m_SignalWin = new CSignalWin();
	m_SignalWin->Tag = AudioWinTag;
	AudioData = new CData();
	//AudioData->Init(SOUNDCARD_BUFF_LENGTH, short_type, SOUNDCARD_BUFF_DATA_BIT);
	AudioData->Init(DATA_BUFFER_LENGTH, short_type, SOUNDCARD_BUFF_DATA_BIT);
	//	m_SignalWin->DataOrignal = AudioData;
	m_SignalWin->DataOrignal = AdcDataFiltted;
	AudioDataFiltted = new CData();
//	AudioDataFiltted->Init(SOUNDCARD_BUFF_LENGTH, float_type, SOUNDCARD_BUFF_DATA_BIT);
	AudioDataFiltted->Init(DATA_BUFFER_LENGTH, float_type, SOUNDCARD_BUFF_DATA_BIT);
	m_SignalWin->DataFiltted = AudioDataFiltted;
	m_SignalWin->Init();

	m_Audio = &mAudio;
	m_Audio->outData = AudioData;
	m_Audio->outDataFiltted = AudioDataFiltted;

	m_FFTWin = new CFFTWin();
	m_FFTWin->Tag = AudioWinTag;
	m_FFTWin->Data = AdcDataFiltted;
	FFTInfo_Audio.FFTSize = 2048;
	FFTInfo_Audio.HalfFFTSize = FFTInfo_Audio.FFTSize / 2;
	FFTInfo_Audio.FFTStep = 2048;
	FFTInfo_Audio.AverageDeep = FFT_DEEP;
	m_FFTWin->fft->FFTInfo = &FFTInfo_Audio;
	m_FFTWin->fft->Color = RGB(0, 255, 0);
	m_FFTWin->fft->ColorLog = RGB(0, 0, 255);

	m_FFTWin->Data2 = AudioDataFiltted;
	FFTInfo_AudioFiltted.FFTSize = 2048;
	FFTInfo_AudioFiltted.HalfFFTSize = FFTInfo_AudioFiltted.FFTSize / 2;
	FFTInfo_AudioFiltted.FFTStep = 2048;
	FFTInfo_AudioFiltted.AverageDeep = FFT_DEEP;
	m_FFTWin->fft2->FFTInfo = &FFTInfo_AudioFiltted;
	m_FFTWin->fft2->Color = RGB(255, 0, 0);
	m_FFTWin->fft2->ColorLog = RGB(255, 255, 0);

	m_FFTWin->isDrawBriefly = false;
	m_FFTWin->isSpectrumZoomedShow = false;

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

		me->hWndRebar = me->MakeToolsBar();

		me->m_SignalWin->hWnd = CreateWindow(SIGNAL_WIN_CLASS, "信号", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 200, 500, 200, hWnd, NULL, hInst, me->m_SignalWin);
		ShowWindow(me->m_SignalWin->hWnd, SW_SHOW);
		me->m_FFTWin->hWnd = CreateWindow(FFT_WIN_CLASS, "FFT", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 0, 500, 200, hWnd, NULL, hInst, me->m_FFTWin);
		ShowWindow(me->m_FFTWin->hWnd, SW_SHOW);

//		clsAudioFilter.SrcData = AudioData;
		clsAudioFilter.SrcData = AdcDataFiltted;
		clsAudioFilter.TargetData = AudioDataFiltted;
		clsAudioFilter.set_cudaFilter(&clscudaAudioFilter, &clscudaAudioFilter2, NULL, CUDA_FILTER_AUDIO_BUFF_SRC_LENGTH);
		clsAudioFilter.ParseCoreDesc();
		clsAudioFilter.scale = &me->m_Audio->Am_zoom;

		me->m_Audio->SampleRate = &AdcDataFiltted->SampleRate;

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFilter::cuda_filter_thread, &clsAudioFilter, 0, NULL);
	}
	break;
	case WM_NOTIFY:
		clsToolsWin.DoNotify(hWnd, message, wParam, lParam);
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
		me->m_DemodulatorAM->AM_Demodulator_Doing = false;
		//while (me->m_DemodulatorAM->h_AM_Demodulator_Thread != NULL);

		me->m_Audio->StopOut();
		me->m_DemodulatorAM->AM_Demodulator_Audio_Out_Doing = false;
		//while (me->m_DemodulatorAM->h_AM_Demodulator_Thread_Audio_Out != NULL);
		clsAudioFilter.doFiltting = false;

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
			m_DemodulatorAM->h_AM_Demodulator_Thread = 
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDemodulatorAM::AM_Demodulator_Thread, m_DemodulatorAM, 0, NULL);
		}
		break;
	case IDM_DEMODULATOR_FM:
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
			m_DemodulatorAM->AM_Demodulator_Audio_Out_Doing = false;
			while (m_DemodulatorAM->h_AM_Demodulator_Thread_Audio_Out != NULL);
			m_DemodulatorAM->h_AM_Demodulator_Thread_Audio_Out =
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDemodulatorAM::AM_Demodulator_Thread_Audio_Out, m_DemodulatorAM, 0, NULL);
		}
	}
	break;
	case IDM_STOPPLAY:
		m_Audio->StopOut();
		break;
	case IDM_AUDIO_BUILD_FILTER:
		BuildAudioFilter();
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
	case IDM_SPECTRUM_FILTTED_SHOW:
	case IDM_SPECTRUM_FILTTED_LOG_SHOW:
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

void CAudioWin::BuildAudioFilter(void)
{
	CFFTWin::POIINT_SERIAL*cp2, * cp = m_FFTWin->FilterPsHead;
	char str[2000];
	int n = sprintf(str, "1023, 0, 0; ");
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

LRESULT CALLBACK CAudioWin::DlgFilterCoreProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CAudioWin* me = (CAudioWin*)get_DlgWinClass(hDlg);
	switch (message)
	{
	case WM_INITDIALOG:
		me = (CAudioWin*)set_DlgWinClass(hDlg, lParam);

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
					//GetDlgItemInt(hDlg, IDC_EDIT2, NULL, TRUE) : AdcData->SampleRate;

				//clsMainFilter.doFiltting = false;
				//while (clsMainFilter.cuda_Filter_exit == false);

				//clsMainFilter.rootFilterInfo.decimationFactorBit = decimationFactor;
				//AdcDataFiltted->SampleRate = AdcData->SampleRate / (1 << clsMainFilter.rootFilterInfo.decimationFactorBit);

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

HWND CAudioWin::MakeToolsBar(void)
{
	HWND hWndRebar = clsToolsWin.CreateRebar(hWnd);
	static TBBUTTON tbb[5] = {
		{ MAKELONG(TOOLS_BTN_PLAY_STOP_ID, 0), IDM_STARTPLAY, TBSTATE_ENABLED,
		BTNS_AUTOSIZE | TBSTYLE_CHECK,
		{0}, 0, (INT_PTR)L"Play" },
		{ MAKELONG(0, 0), NULL, 0,
		TBSTYLE_SEP, 
		{0}, 0, NULL }, // Separator
		{ MAKELONG(TOOLS_BTN_DEMODULATOR_AM_ID, 0), IDM_DEMODULATOR_AM, TBSTATE_ENABLED,
		BTNS_AUTOSIZE | TBSTYLE_CHECK,
		{0}, 0, (INT_PTR)L"AM" },
		{ MAKELONG(TOOLS_BTN_DEMODULATOR_FM_ID, 0), IDM_DEMODULATOR_FM, TBSTATE_ENABLED,
		BTNS_AUTOSIZE | TBSTYLE_CHECK,
		{0}, 0, (INT_PTR)L"FM" },
		{ MAKELONG(0, 0), 0, TBSTATE_ENABLED,
		BTNS_AUTOSIZE | TBSTYLE_DROPDOWN,
		{0}, 0, (INT_PTR)L"下拉" }
	};
	CToolsWin::TOOL_TIPS tips[5] = {
		{ IDM_STARTPLAY,"开始 / 停止 播放音频信号." },
		{ 0, NULL }, // Separator
		{ IDM_DEMODULATOR_AM,"开启 / 关闭 调幅解码." },
		{ IDM_DEMODULATOR_FM, "开启 / 关闭 调频解码." },
		{ 0, "TBSTYLE_DROPDOWN" }
	};
	HWND hToolBar = clsToolsWin.CreateToolbar(hWnd, tbb, 5, tips, 5);

	// Add images
	TBADDBITMAP tbAddBmp = { 0 };
	tbAddBmp.hInst = HINST_COMMCTRL;
	tbAddBmp.nID = IDB_STD_SMALL_COLOR;
	SendMessage(hToolBar, TB_ADDBITMAP, 0, (WPARAM)&tbAddBmp);

	clsToolsWin.CreateRebarBand(hWndRebar, "BTN", 1, 500, 0, hToolBar);

	hWndTrack = CreateTrackbar(hWnd, 0, 100, 10);
	clsToolsWin.CreateRebarBand(hWndRebar, "Value", 2, 0, 0, hWndTrack);
	return hWndRebar;
}

HWND CAudioWin::CreateTrackbar(HWND hWnd, UINT iMin, UINT iMax, UINT pos)
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

VOID CAudioWin::TBNotifications(WPARAM wParam)
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