#pragma once

#include "stdafx.h"
#include "stdint.h"

#include "CWaveFilter.h"


#define FILTER_WIN_CLASS "FILTERCORE_WIN"

class CWinFilter
{
public:

	CWaveFilter::PFILTERINFO rootFilterInfo, pFilterInfo;

	HWND	hWnd = NULL;
	HDC		hdcCache = NULL;
	HBITMAP hbmpCache;
	UINT32  WinWidth, WinHeight;

	HMENU hMenuMain;
	HMENU hMenuFilterItems;

	int HOriginalWidth = 0;
	int HScrollPos = 0, HScrollWidth = 0;
	double HScrollZoom = 1.0;

	FILTERCOREDATATYPE* pCore = NULL;

	bool filterCoreShow = true;
	bool filterCoreSpectrumShow = true;
	bool filterCoreSpectrumLogShow = true;
public:
	CWinFilter();
	~CWinFilter();

	void RegisterWindowsClass(void);
	void OpenWindow(void);

	void Paint(HWND hWnd);
	BOOL OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void GetRealClientRect(HWND hWnd, PRECT lprc);
	void KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void InitFilterCoreAnalyse(CWaveFilter::PFILTERINFO pFilterInfo);
	VOID set_CoreAnalyse_root_Filter(CWaveFilter::PFILTERINFO pFilterInfo);

	static LPTHREAD_START_ROUTINE FilterCoreAnalyse(LPVOID lp);

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK DlgFilterCoreProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

extern CWinFilter clsWinFilter;