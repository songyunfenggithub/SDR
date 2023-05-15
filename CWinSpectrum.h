#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

namespace METHOD {
	class CFFT;
}
using namespace METHOD;

namespace WINS {

	namespace SDR_SET {
		class CWinSDRSet;
	}
	using namespace SDR_SET;

	namespace SPECTRUM {

#define WIN_SPECTRUM_CLASS		"WIN_SPECTRUM_CLASS"

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

		class CWinSpectrum
		{
		public:

			//HANDLE hMutexBuff = NULL;

			UINT uTimerId;

			HWND hWnd = NULL;
			RECT WinRect;

			HWND hWndOneSpectrum = NULL;
			HWND hWndOneFFT = NULL;
			HWND hWndRebar = NULL;
			HWND hWndFreqToolbar = NULL;
			HWND hWndSDRToolbar = NULL;

			HWND hSDRSet = NULL;

			HMENU hMenu = NULL;
			HMENU hMenuShow = NULL;

			int HScrollPos = 0, VScrollPos = 0, HScrollRange = 0;
			double HScrollZoom = 1.0;
			double VScrollZoom = 1.0;

			CFFT* FFTOrignal = NULL;
			CFFT* FFTFiltted = NULL;

			bool bFFTHold = false;

		public:
			CWinSpectrum();
			~CWinSpectrum();

			void RegisterWindowsClass(void);
			void OpenWindow(void);

			void Init(void);
			void Paint(void);
			bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
			void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

			void GetRealClientRect(PRECT lprc);

			HWND MakeReBar(void);
			bool DoNotify(UINT msg, WPARAM wParam, LPARAM lParam);
			void ToolsbarSetFilterCenterFreq(void);
			void ToolsbarSetAMFreqAdd(void);
			void ToolsbarSetAMFreqSub(void);
			void ToolsbarSetFMFreqAdd(void);
			void ToolsbarSetFMFreqSub(void);

			void RestoreValue(void);
			void SaveValue(void);

			LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			static LRESULT CALLBACK DlgFFTSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
		};
	}
}
extern WINS::SPECTRUM::CWinSpectrum clsWinSpect;