
#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>


#define SPECTRUM_ONE_WIN_CLASS		"SPECTRUM_ONE_WIN"

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

	UINT whichSignel = 0;
	UINT WinWidth, WinHeight;

	bool bSpectrumZoomedShow = false;

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

	static void PaintSpectrum(void);

	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

extern CWinOneSpectrum clsWinOneSpectrum;