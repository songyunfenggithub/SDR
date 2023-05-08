#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

namespace DEVICES {
	class CAudio;
}
using namespace DEVICES;

class CDemodulatorAM;

namespace WINS {

#define AUDIO_WIN_CLASS		"AUDIO_WIN_CLASS"

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

	class CFFTWin;
	class CSignalWin;
	class CToolsWin;
	class CFilterWin;

	class CAudioWin
	{
	public:
		CAudio* m_Audio = NULL;
		CFFTWin* m_FFTWin = NULL;
		CSignalWin* m_SignalWin = NULL;
		CFilterWin* m_FilterWin = NULL;

		CDemodulatorAM* m_DemodulatorAM = NULL;
		bool bDemodulatorAM = false;
		HWND hWndRebar = NULL;
		HWND hWnd = NULL;
		HMENU hMenu = NULL;
		RECT WinRect;
		UINT SignalWinHeight = 256;

		HWND hWndTrack = NULL;

	public:
		CAudioWin();
		~CAudioWin();

		void Init(void);
		void UnInit(void);

		void RegisterWindowsClass(void);
		void OpenWindow(void);
		bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);

		HWND MakeToolsBar(void);
		HWND CreateTrackbar(HWND hwndDlg, UINT iMin, UINT iMax, UINT pos);
		VOID TBNotifications(WPARAM wParam);

		void BuildAudioFilter(void);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK DlgFilterCoreProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	};
}