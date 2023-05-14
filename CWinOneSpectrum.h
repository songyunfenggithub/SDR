
#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

namespace WINS {

#define WIN_SPECTRUM_ONE_CLASS		"WIN_SPECTRUM_ONE_CLASS"

	class CWinOneSpectrum
	{
	public:

		HWND	hWnd = NULL;
		HDC		hDCFFT = NULL;
		HBITMAP hBMPFFT = NULL;

		double  FFTMaxs = 0.0;
		UINT	FFTMaxsPos = 0;
		double  FFTAvgMaxValue = DBL_MAX;
		INT		SpectrumY = -1;
		long	PainttedFFT;
		UINT	uTimerId;

		HANDLE  hDrawMutex;

		INT MouseX;
		INT MouseY;
		double Hz = 0.0;

		UINT whichSignel = 1;
		UINT WinWidth, WinHeight;

		bool bSpectrumBrieflyShow = true;

		bool inPaintSpectrumThread = false;

		bool readyForInitBuff = false;

	public:
		CWinOneSpectrum();
		~CWinOneSpectrum();

		void RegisterWindowsClass(void);

		void isWaitingToInitBuff(void);

		void InitDrawBuff(void);
		void Paint(void);
		bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
		void GetRealClientRect(PRECT lprc);

		void PaintSpectrum(void* fft);

		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};
}
extern WINS::CWinOneSpectrum clsWinOneSpectrum;