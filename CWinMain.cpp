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

#include "Public.h"
#include "Debug.h"

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
#include "CWinMsg.h"

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
#include "CWinSignal.h"
#include "CWinSpectrumScan.h"

#include <CL/cl.h>

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

   hWnd = CreateWindow(WIN_MAIN_CLASS, "SDR ver 1.0", WS_OVERLAPPEDWINDOW,
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

		clsWinMsg.OpenWindow();
		
		this->hWnd = hWnd;
		hMenu = GetMenu(hWnd);

		uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);

		MainInit();
		clsAnalyze.Init_Params();
	
		m_audioWin = new CWinAudio();
		m_filttedWin = new CWinFiltted();
		m_FilterWin = new CWinFilter();
		m_FilterWin->cFilter = &clsMainFilterI;

		hWndRebar = MakeToolsBar();

		m_signalWin = new CWinSignal();
		m_signalWin->Flag = "Main";

		m_signalWin->Init(AdcDataI, AdcDataIFiltted, NULL, NULL);
		m_signalWin->hWnd = CreateWindow(WIN_SIGNAL_CLASS, "信号", WS_CHILDWINDOW | WS_BORDER,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			0, 200, 500, 200, hWnd, NULL, hInst, m_signalWin);
		ShowWindow(m_signalWin->hWnd, SW_SHOW);

		hMenuFollow = GetSubMenu(hMenu, 8);
		CheckMenuRadioItem(hMenuFollow, 10, 13, 10, MF_BYPOSITION);

		CheckMenuItem(hMenu, IDM_WAVEAUTOSCROLL, (m_signalWin->bAutoScroll ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		CheckMenuItem(hMenu, IDM_WAVE_IDATA_SHOW, (m_signalWin->bDrawDataI ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_WAVE_QDATA_SHOW, (m_signalWin->bDrawDataQ ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_WAVE_IDATA_FILTTED_SHOW, (m_signalWin->bDrawDataI_Filtted ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(hMenu, IDM_WAVE_QDATA_FILTTED_SHOW, (m_signalWin->bDrawDataQ_Filtted ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

		DbgMsg("CWinMain WM_CREATE\r\n");
		break;
	case WM_TIMER:
	{
	}
	break;
	case WM_SIZE:
	{
		GetClientRect(hWnd, &WinRect);
		GetClientRect(hWndRebar, &RebarRect);
		//RebarRect.bottom += 10;
		MoveWindow(hWndRebar, 0, 0, WinRect.right, RebarRect.bottom, true);
		if(m_signalWin)	MoveWindow(m_signalWin->hWnd, 0, RebarRect.bottom, WinRect.right, WinRect.bottom - RebarRect.bottom, true);

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
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_CLOSE:
	{
		int res = ::MessageBox(NULL, "[WM_CLOSE] 是否确定退出", "提示", MB_OKCANCEL);
		if (res == IDOK) { //点击 【确定】按钮
			WaitForExit();
			break;
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
	case WM_USER:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:


		if (clsWinMsg.hWnd) 
			DestroyWindow(clsWinMsg.hWnd);

		clsMainFilterI.SaveValue();
		//clsMainFilterQ.SaveValue();
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
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
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
			clsSoundCard.OpenOut( m_signalWin->DrawInfo.iHZoom > 0 ? 
				m_signalWin->DrawInfo.dwHZoomedPos >> m_signalWin->DrawInfo.iHZoom
							 :
				m_signalWin->DrawInfo.dwHZoomedPos << -m_signalWin->DrawInfo.iHZoom
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
	case IDM_WAVEHZOOMINCREASE:
	case IDM_WAVEHZOOMDECREASE:
	case IDM_WAVEVZOOMRESET:
	case IDM_WAVEVZOOMINCREASE:
	case IDM_WAVEVZOOMDECREASE:
		PostMessage(m_signalWin->hWnd, message, wParam, lParam);
		break;
	case IDM_WAVEAUTOSCROLL:
		SendMessage(m_signalWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_WAVEAUTOSCROLL, (m_signalWin->bAutoScroll ? MF_CHECKED : MF_UNCHECKED ) | MF_BYCOMMAND);
		break;
	case IDM_WAVE_FOLLOW_IDATA:
		CheckMenuRadioItem(hMenuFollow, 10, 13, 10, MF_BYPOSITION);
		PostMessage(m_signalWin->hWnd, message, wParam, lParam);
		break;
	case IDM_WAVE_FOLLOW_QDATA:
		CheckMenuRadioItem(hMenuFollow, 10, 13, 11, MF_BYPOSITION);
		PostMessage(m_signalWin->hWnd, message, wParam, lParam);
		break;
	case IDM_WAVE_FOLLOW_FILTTED_IDATA:
		CheckMenuRadioItem(hMenuFollow, 10, 13, 12, MF_BYPOSITION);
		PostMessage(m_signalWin->hWnd, message, wParam, lParam);
		break;
	case IDM_WAVE_FOLLOW_FILTTED_QDATA:
		CheckMenuRadioItem(hMenuFollow, 10, 13, 13, MF_BYPOSITION);
		PostMessage(m_signalWin->hWnd, message, wParam, lParam);
		break;

//COMMANDS-----------------------
	case IDM_SPECTRUM_WINDOW:
		clsWinSpect.OpenWindow();
		break;
	case IDM_SPECTRUM_SCAN_WINDOW:
		clsWinSpectrumScan.OpenWindow();
		break;
	case IDM_FILTTEDWINDOW:
		m_filttedWin->OpenWindow();
		break;
	case IDM_FILTER_START_STOP:
	{
		TBBUTTONINFO tbInfo = { 0 };
		tbInfo.cbSize = sizeof(TBBUTTONINFO);
		tbInfo.dwMask = TBIF_STATE;
		SendMessage(hWndToolbar, TB_GETBUTTONINFO, (WPARAM)IDM_FILTER_START_STOP, (LPARAM)&tbInfo);
		if (TBSTATE_CHECKED & tbInfo.fsState) {
			FilterStart();
			//DbgMsg("IDM_FILTER_START_STOP TBSTATE_CHECKED\r\n");
		}
		else {
			FilterStop();
			//DbgMsg("IDM_FILTER_START_STOP\r\n");
		}
	}
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
	case IDM_MSG_WINDOW:
		clsWinMsg.OpenWindow();
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
		
	case IDM_WAVE_IDATA_SHOW:
		SendMessage(m_signalWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_WAVE_IDATA_SHOW, (m_signalWin->bDrawDataI ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_WAVE_QDATA_SHOW:
		SendMessage(m_signalWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_WAVE_QDATA_SHOW, (m_signalWin->bDrawDataQ ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_WAVE_IDATA_FILTTED_SHOW:
		SendMessage(m_signalWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_WAVE_IDATA_FILTTED_SHOW, (m_signalWin->bDrawDataI_Filtted ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_WAVE_QDATA_FILTTED_SHOW:
		SendMessage(m_signalWin->hWnd, message, wParam, lParam);
		CheckMenuItem(hMenu, IDM_WAVE_QDATA_FILTTED_SHOW, (m_signalWin->bDrawDataQ_Filtted ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
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

HWND CWinMain::MakeToolsBar(void)
{
	HWND hWndRebar = clsWinTools.CreateRebar(hWnd);
	static TBBUTTON tbb[13] = {
		{ MAKELONG(1, 0), IDM_AUDIO_WINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"Audio" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL }, 
		{ MAKELONG(2, 0), IDM_SPECTRUM_WINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE,	{0}, 0, (INT_PTR)L"Spectrum" },
		{ MAKELONG(2, 0), IDM_SPECTRUM_SCAN_WINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE,	{0}, 0, (INT_PTR)L"Spectrum Scan" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP,	{0}, 0, NULL },
		{ MAKELONG(3, 0), IDM_FILTERWINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"Filter" },
		{ MAKELONG(4, 0), IDM_FILTER_START_STOP, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_CHECK, {0}, 0, (INT_PTR)L"Filter Doing" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP, {0}, 0, NULL },
		{ MAKELONG(4, 0), IDM_SDR_WINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"SDR" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP, {0}, 0, NULL }, 
		{ MAKELONG(4, 0), IDM_MSG_WINDOW, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"MSG" },
		{ MAKELONG(0, 0), NULL, 0, TBSTYLE_SEP, {0}, 0, NULL },
		{ MAKELONG(0, 0), 0, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_DROPDOWN,	{0}, 0, (INT_PTR)L"按钮" }
	};
	CWinTools::TOOL_TIPS tips[13] = {
		{ IDM_AUDIO_WINDOW,"打开音频信号窗口." },
		{ 0, NULL },
		{ IDM_SPECTRUM_WINDOW,"打开信号频谱窗口." },
		{ IDM_SPECTRUM_SCAN_WINDOW,"打开信号频谱扫描窗口." },
		{ 0, NULL },
		{ IDM_FILTERWINDOW, "打开滤波器设计窗口." },
		{ IDM_FILTER_START_STOP, "打开 / 关闭 滤波器." },
		{ 0, NULL },
		{ IDM_SDR_WINDOW, "打开SDR设备窗口." },
		{ 0, NULL },
		{ IDM_MSG_WINDOW, "打开调试消息窗口." },
		{ 0, NULL },
		{ 0, "TBSTYLE_DROPDOWN" }
	};
	hWndToolbar = clsWinTools.CreateToolbar(hWnd, tbb, 13, tips, 13);

	// Add images
	TBADDBITMAP tbAddBmp = { 0 };
	tbAddBmp.hInst = HINST_COMMCTRL;
	tbAddBmp.nID = IDB_STD_SMALL_COLOR;
	SendMessage(hWndToolbar, TB_ADDBITMAP, 0, (WPARAM)&tbAddBmp);

	clsWinTools.CreateRebarBand(hWndRebar, "窗口", 1, 500, 0, hWndToolbar);

	//hWndTrack = CreateTrackbar(hWnd, 0, 100, 10);
	clsWinTools.CreateRebarBand(hWndRebar, "Value", 2, 0, 0, NULL);
	return hWndRebar;
}


LPTHREAD_START_ROUTINE CWinMain::FilterStopThread(LPVOID lp)
{
	clsMainFilterI.doFiltting = false;
	//clsMainFilterQ.doFiltting = false;
	while (clsMainFilterI.hThread != NULL);
	//while (clsMainFilterQ.hThread != NULL);
	return 0;
}

void CWinMain::FilterStop(void)
{
	clsMainFilterI.doFiltting = false;
	//clsMainFilterQ.doFiltting = false;
	//CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CWinMain::FilterStopThread, NULL, 0, NULL);
}

void CWinMain::FilterStart(void)
{
	while (clsMainFilterI.hThread != NULL);
	//while (clsMainFilterQ.hThread != NULL);

	UINT stepLen = clsMainFilterI.FilterSrcLen >> 2;
	clsMainFilterI.SrcData->ProcessPos = ((UINT)(clsMainFilterI.SrcData->Pos / stepLen)) * stepLen;
	clsMainFilterI.SrcData->ProcessPos &= clsMainFilterI.SrcData->Mask;
	//clsMainFilterQ.SrcData->ProcessPos = clsMainFilterI.SrcData->ProcessPos;
	
	//if (
	//	((clsMainFilterI.TargetData->Pos - clsMainFilterQ.TargetData->Pos) & clsMainFilterI.TargetData->Mask)
	//	> 
	//	clsMainFilterI.TargetData->Len / 2
	//	)
	//	clsMainFilterI.TargetData->Pos = clsMainFilterQ.TargetData->Pos;
	//else 
	//	clsMainFilterQ.TargetData->Pos = clsMainFilterI.TargetData->Pos;
	
	clsMainFilterI.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFilter::cuda_filter_thread, &clsMainFilterI, 0, NULL);
	//clsMainFilterQ.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CFilter::cuda_filter_thread, &clsMainFilterQ, 0, NULL);
}