
#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

#include "CMessage.h"

namespace WINS {

	namespace SPECTRUM_SCAN {

#define WIN_SPECTRUM_SCAN_CLASS		"WIN_SPECTRUM_SCAN_CLASS"

		class CWinSpectrumScan
		{
		public:

			UINT uTimerId = 0;

			HANDLE  hMutex = NULL;

			HWND hWnd = NULL;
			RECT WinRect = { 0 };

			HWND hWndRebar = NULL;
			HWND hWndToolbar = NULL;
			RECT RebarRect = { 0 };

			UINT MouseX = 0;
			UINT MouseY = 0;

			char strMouse[1024];

			int HScrollPos = 0, HScrollRange = 0;
			double HScrollZoom = 1.0;
			double VScrollZoom = 1.0;

		public:
			CWinSpectrumScan();
			~CWinSpectrumScan();

			void RegisterWindowsClass(void);
			void OpenWindow(void);

			bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
			void OnMouse(void);
			void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);
			
			HWND CWinSpectrumScan::MakeToolsBar(void);

			LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		};
	}
}
extern WINS::SPECTRUM_SCAN::CWinSpectrumScan clsWinSpectrumScan;
