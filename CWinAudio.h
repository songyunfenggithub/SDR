#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

namespace DEVICES {
	class CAudio;
}
using namespace DEVICES;

namespace DEMODULATOR {
	class CAM;
	class CFM;
}using namespace DEMODULATOR;

namespace WINS {

#define WIN_AUDIO_CLASS		"WIN_AUDIO_CLASS"

#define SPECTRUM_WIDTH	0x400
#define SPECTRUM_WIN_WIDTH	800

#define SPECTRUM_ZOOM_MAX	16
	//#define SPECTRUM_ZOOM_MIN	0.16

#define TOOLS_BAR_HEIGHT			30

#define WAVE_RECT_HEIGHT			0X200
#define WAVE_RECT_BORDER_TOP		25
#define WAVE_RECT_BORDER_LEFT		20
#define WAVE_RECT_BORDER_RIGHT		60
#define WAVE_RECT_BORDER_BOTTON		25

#define TIMEOUT		200

	class CWinFFT;
	class CWinSignal;
	class CWinTools;
	class CWinFilter;

	class CWinAudio
	{
	public:
		CAudio* m_Audio = NULL;
		CWinFFT* m_FFTWin = NULL;
		CWinSignal* m_SignalWin = NULL;
		CWinFilter* m_FilterWin = NULL;

		CAM* m_AM = NULL;
		bool bAM = false;
		CFM* m_FM = NULL;
		bool bFM = false;

		HWND hWnd = NULL;
		HWND hWndRebar = NULL;
		HWND hWndToolBar = NULL;

		HMENU hMenu = NULL;
		HMENU hMenuFollow = NULL;
		HMENU hMenuSpectrum = NULL;

		RECT WinRect = { 0 };
		UINT SignalWinHeight = 256;

		HWND hWndTrack = NULL;

	public:
		CWinAudio();
		~CWinAudio();

		void Init(void);
		void UnInit(void);

		void RegisterWindowsClass(void);
		void OpenWindow(void);
		bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);

		HWND MakeToolsBar(void);
		HWND CreateTrackbar(HWND hwndDlg, UINT iMin, UINT iMax, UINT pos);
		VOID TBNotifications(WPARAM wParam);
		BOOL DoNotify(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void BuildAudioFilter(void);
		void ClearAudioFilterPoints(void);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK DlgFilterCoreProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK DlgFFTSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	};
}