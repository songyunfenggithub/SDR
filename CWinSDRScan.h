
#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

#include "CMessage.h"

namespace WINS {

#define WIN_SDR_SCAN_ONE_CLASS		"WIN_SDR_SCAN_ONE_CLASS"

	class CWinSDRScan
	{
	public:

		UINT	uTimerId;

		HANDLE  hMutexUseBuff;

		UINT MouseX;
		UINT MouseY;
		double Hz = 0.0;

		UINT WinWidth;

		HWND hWnd = NULL;

		double* pCore = NULL;
		int CoreLength;

		bool FFTNeedReDraw = true;
		BOOL bFFTHold = false;

		BOOL bFFTOrignalShow = true;
		BOOL bFFTOrignalLogShow = true;
		BOOL bFFTFilttedShow = true;
		BOOL bFFTFilttedLogShow = true;

		char strMouse[1024];

		CMessage msgs;

		int HScrollPos = 0, HScrollWidth = 0;
		double HScrollZoom = 1.0;
		double VScrollZoom = 1.0;


		double* OrignalScanBuff = NULL;
		double* OrignalScanBuffLog = NULL;
		double* FilttedScanBuff = NULL;
		double* FilttedScanBuffLog = NULL;

	public:
		CWinSDRScan();
		~CWinSDRScan();

		void RegisterWindowsClass(void);
		void OpenWindow(void);

		void Paint(HWND hWnd);
		//void ProcessKey(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		BOOL OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		void OnMouse(HWND hWnd);
		void KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		void InitBuff(void);
		void GetRealClientRect(HWND hWnd, PRECT lprc);

		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};
}
extern WINS::CWinSDRScan clsWinSdrScan;
