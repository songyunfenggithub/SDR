#pragma once

#include "stdafx.h"
#include "stdint.h"

#include "CFilter.h"


#define PMT_WIN_CLASS	"PMT_WIN"

#define PMT_ZOOM_MAX		16


class CWinPMT
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


	double PMTMax = 1.0;

	BOOL bPMTShow = true;
	BOOL bPMTLogShow = true;
	BOOL bFFTHold = false;

	UINT MouseX, MouseY;

public:
	CWinPMT();
	~CWinPMT();

	void RegisterWindowsClass(void);

	void Paint(HWND hWnd);
	BOOL OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void GetRealClientRect(HWND hWnd, PRECT lprc);
	void KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void Init(void);
	void OnMouse(HWND hWnd);
	void OpenWindow(void);

	static LPTHREAD_START_ROUTINE FilterCoreAnalyse(LPVOID lp);

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

extern CWinPMT clsWinPMT;
