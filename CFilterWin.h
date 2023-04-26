#pragma once

#include "stdafx.h"
#include "stdint.h"

#include "CWaveFilter.h"


#define FILTER_WIN_CLASS "FILTERCORE_WIN"

class CFilterWin
{
public:

	CWaveFilter::PFILTERINFO rootFilterInfo = NULL, pFilterInfo = NULL;

	HWND	hWnd = NULL;
	HDC		hdcCache = NULL;
	HBITMAP hbmpCache = NULL;
	UINT32  WinWidth, WinHeight;

	HMENU hMenuMain = NULL;
	HMENU hMenuFilterItems = NULL;

	int HOriginalWidth = 0;
	int HScrollPos = 0, HScrollWidth = 0;
	double HScrollZoom = 1.0;

	FILTER_CORE_DATA_TYPE* pCore = NULL;

	bool filterCoreShow = true;
	bool filterCoreSpectrumShow = true;
	bool filterCoreSpectrumLogShow = true;

	typedef struct {
		CFilterWin* pFilterWin;
		CWaveFilter::PFILTERINFO pFilterInfo;
	}CORE_ANALYSE_DATA;

	CORE_ANALYSE_DATA CoreAnalyse = { 0 };

	HANDLE hCoreAnalyseMutex;	//定义互斥对象句柄

	UINT   CoreAnalyseFFTLength = 0;
	double *CoreAnalyseFFTBuff = NULL;
	double *CoreAnalyseFFTLogBuff = NULL;

public:
	CFilterWin();
	~CFilterWin();

	void RegisterWindowsClass(void);
	void OpenWindow(void);

	void Paint(void);
	bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
	void GetRealClientRect(PRECT lprc);
	void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);
	void InitFilterCoreAnalyse(CWaveFilter::PFILTERINFO pFilterInfo);
	void set_CoreAnalyse_root_Filter(CWaveFilter::PFILTERINFO pFilterInfo);

	static LPTHREAD_START_ROUTINE FilterCoreAnalyse(LPVOID lp);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK DlgFilterCoreProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};