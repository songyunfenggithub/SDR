#pragma once

class CScreenButton;

namespace WINS {

	namespace SPECTRUM_SCAN {

#define WIN_SPECTRUM_SCAN_SET_CLASS		"WIN_SPECTRUM_SCAN_SET_CLASS"

		class CWinSpectrumScanSet
		{
	
		public:
			HWND hWnd = NULL;
			RECT WinRect = { 0 };

			RECT ButtonRect = { 0 };
			HWND hWndFFTSize = NULL;
			UINT FFTSize = 0;

			UINT uTimerId = 0;

			INT MouseX = 0;
			INT MouseY = 0;

			int HScrollPos = 0, HScrollRange = 0;
			double HScrollZoom = 1.0;
			double VScrollZoom = 1.0;

			HANDLE  hMutex = NULL;

			CScreenButton* ButtonRFStart;
			CScreenButton* ButtonRFStop;
			CScreenButton* ButtonAverageDeep;

		public:
			CWinSpectrumScanSet();
			~CWinSpectrumScanSet();

			void RegisterWindowsClass(void);
			void OpenWindow(void);

			void Paint(void);
			BOOL OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
			void OnMouse(void);
			void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

			void SaveValue(void);
			void RestoreValue(void);

			static void ButtonRFStartFunc(CScreenButton* button);
			static void ButtonRFStopFunc(CScreenButton* button);
			static void ButtonAverageDeepFunc(CScreenButton* button);

			LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		};

	}
}

extern WINS::SPECTRUM_SCAN::CWinSpectrumScanSet clsWinSpectrumScanSet;
