#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

namespace WINS {

	class CWinFFT;
	class CWinSignal;

#define WIN_CFILTTED_CLASS		"WIN_CFILTTED_CLASS"

#define SPECTRUM_WIDTH	0x400
#define SPECTRUM_WIN_WIDTH	800

#define SPECTRUM_ZOOM_MAX	16
	//#define SPECTRUM_ZOOM_MIN	0.16

#define WAVE_RECT_HEIGHT			0X200
#define WAVE_RECT_BORDER_TOP		25
#define WAVE_RECT_BORDER_LEFT		20
#define WAVE_RECT_BORDER_RIGHT		60
#define WAVE_RECT_BORDER_BOTTON		25

#define TIMEOUT		200


	class CWinFiltted
	{
	public:
		CWinFFT* m_FFTWin = NULL;
		CWinSignal* m_SignalWin = NULL;

		HWND hWnd = NULL;
		RECT WinRect;
		UINT SignalWinHeight = 256;

	public:
		CWinFiltted();
		~CWinFiltted();

		void Init(void);
		void UnInit(void);

		void RegisterWindowsClass(void);
		void OpenWindow(void);
		BOOL OnCommand(UINT message, WPARAM wParam, LPARAM lParam);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		void AM_Demodulator(void);
	};
}