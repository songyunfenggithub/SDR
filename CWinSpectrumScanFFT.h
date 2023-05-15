#pragma once

namespace METHOD {
	class CFFT;
}
using namespace METHOD;

namespace WINS {

	namespace SPECTRUM_SCAN {

#define WIN_SPECTRUM_SCAN_FFT_CLASS		"WIN_SPECTRUM_SCAN_FFT_CLASS"

		class CWinSpectrumScanFFT
		{
		public:
			HWND hWnd = NULL;
			RECT WinRect = { 0 };

			HWND hWndRebar = NULL;
			HWND hWndToolbar = NULL;
			RECT RebarRect = { 0 };

			HMENU hMenu = NULL;
			HMENU hMenuFFT = NULL;
			HMENU hMenuSpectrum = NULL;

			UINT uTimerId = 0;

		public:
			CWinSpectrumScanFFT();
			~CWinSpectrumScanFFT();

			void RegisterWindowsClass(void);
			void OpenWindow(void);

			bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
			void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

			HWND MakeToolsBar(void);

			LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		};
	}
}

extern WINS::SPECTRUM_SCAN::CWinSpectrumScanFFT clsWinSpectrumScanFFT;