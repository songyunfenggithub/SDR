#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

#include "CData.h"
#include "CWinOneSpectrum.h"


#define SPECTRUM_WIN_CLASS		"SPECTRUM_WIN"

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

	HANDLE  hMutexBuff;

	UINT		uTimerId;

	HWND	hWnd				= NULL;

	//UINT32  WinWidth, WinHeight;
	RECT WinRect;
	UINT32	WinWidthSpectrum = 0, WinHeightSpectrum = 0;
	UINT	PainttedFFT = 0;

	HWND hWndOneSpectrum		= NULL;
	HWND hWndOneFFT				= NULL;

	//HWND hWndSpectrumHScrollBar;
	//HWND hWndSpectrumVScrollBar;

	int WinOneSpectrumHeight = 100;

	int WinOneSpectrumHScrollPos = 0;
	int WinOneSpectrumVScrollPos = 0;
	double	WinOneSpectrumHScrollZoom = 1.0;
	double	WinOneSpectrumVScrollZoom = 1.0;

	//HWND ActivehWnd = NULL;
	HMENU hMenu;
	bool SpectrumAutoFollow = true;


	int HScrollPos = 0, VScrollPos = 0, HScrollWidth = 0;
	double HScrollZoom = 1.0;
	double VScrollZoom = 1.0;

	double* OrignalFFTBuff = NULL;
	double* OrignalFFTBuffLog = NULL;
	double* FilttedFFTBuff = NULL;
	double* FilttedFFTBuffLog = NULL;
	
	double* BrieflyOrigFFTBuff = NULL;
	double* BrieflyOrigFFTBuffLog = NULL;
	double* BrieflyFiltFFTBuff = NULL;
	double* BrieflyFiltFFTBuffLog = NULL;

	double* Buffs[4];
	double* BrieflyBuffs[4];

public:
	CWinSpectrum();
	~CWinSpectrum();

	void RegisterWindowsClass(void);
	void OpenWindow(void);

	void Init(void);
	void Paint(HWND hWnd);
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void InitBuff(void);

	VOID GetRealClientRect(HWND hWnd, PRECT lprc);

	void RestoreValue(void);
	void SaveValue(void);

	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK DlgFFTSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

extern CWinSpectrum clsWinSpect;