#pragma once

namespace WINS {

	namespace SPECTRUM_SCAN {

#define WIN_SPECTRUM_SCAN_SHOW_CLASS		"WIN_SPECTRUM_SCAN_SHOW_CLASS"

		class CWinSpectrumScanShow
		{
		public:
			HWND hWnd = NULL;
			RECT WinRect = { 0 };

			UINT uTimerId = 0;

			INT MouseX = 0;
			INT MouseY = 0;

			int HScrollPos = 0, HScrollRange = 0;
			double HScrollZoom = 1.0;
			double VScrollZoom = 1.0;

			HANDLE  hMutex = NULL;

			CFFT* fft = NULL;


			bool bFFTShow = true;
			bool bFFTLogShow = true;
			bool bFFTBrieflyShow = true;
			bool bFFTBrieflyLogShow = true;


		public:
			CWinSpectrumScanShow();
			~CWinSpectrumScanShow();

			void RegisterWindowsClass(void);
			void OpenWindow(void);

			void Paint(void);
			BOOL OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
			void OnMouse(void);
			void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

			LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		};
	}
}

extern WINS::SPECTRUM_SCAN::CWinSpectrumScanShow clsWinSpectrumScanShow;