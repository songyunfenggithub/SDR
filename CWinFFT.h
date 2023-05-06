
#pragma once

#include "stdafx.h"
#include "stdint.h"

#include "CFilter.h"


#define FFT_WIN_CLASS	"FFT_WIN"

#define FFT_ZOOM_MAX		16

class CWinFFT
{
public:
	typedef struct tagFFTInfo
	{
		uint32_t i;				//in buffer position
		uint32_t adc_buf_pos;	//in filtered adc buffer position
		uint32_t fft_size;
		uint32_t fft_data_size;
		double 	 fft_max_value;
		double 	 fft_min_value;
	} FFTInfo;

	typedef struct tagDRAWINFO
	{

		int			iHZoom, iHOldZoom, iVZoom, iVOldZoom, iHFit, iVFit;
		DWORD		dwDataWidth;
		DWORD		dwHZoomedWidth, dwHZoomedPos;
		WORD		wHSclPos, wVSclPos, wHSclMin, wHSclMax, wVSclMin, wVSclMax;

		BOOL		fAutoScroll;
		UINT		uTimerId;

	} DRAWINFO, * PDRAWINFO;

	DRAWINFO	DrawInfo;

	HANDLE  hMutexUseBuff;

	HWND	hWnd;
	HMENU	hMenu;

	UINT32  WinWidth, WinHeight;

	HMENU hMenuMain;
	HMENU hMenuFilterItems;

	int HScrollPos = 0, HScrollWidth = 0;
	double HScrollZoom = 1.0;
	double VScrollZoom = 1.0;

	double* pCore = NULL;
	int CoreLength;
	
	bool FFTNeedReDraw = true;
	bool OrignalFFTBuffReady = false;
	bool FilttedFFTBuffReady = false;
	double* OrignalFFTBuff		= NULL;
	double* OrignalFFTBuffLog	= NULL;
	double* FilttedFFTBuff		= NULL;
	double* FilttedFFTBuffLog	= NULL;

	BOOL bFFTOrignalShow = true;
	BOOL bFFTOrignalLogShow = true;
	BOOL bFFTFilttedShow = true;
	BOOL bFFTFilttedLogShow = true;
	BOOL bFFTHold = false;

	UINT MouseX, MouseY;

public:
	CWinFFT();
	~CWinFFT();

	void RegisterWindowsClass(void);

	void Paint(HWND hWnd);
	BOOL OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void GetRealClientRect(HWND hWnd, PRECT lprc);
	void KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void Init(void);
	void OnMouse(HWND hWnd);

//	static LPTHREAD_START_ROUTINE FilterCoreAnalyse(LPVOID lp);

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

extern CWinFFT clsWinFFT;