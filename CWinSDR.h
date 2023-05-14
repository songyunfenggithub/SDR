
#pragma once

#include "stdafx.h"
#include "stdint.h"
#include <commctrl.h>

#include <stdlib.h>
#include <string>

#include "CData.h"
#include "CWinOneSpectrum.h"

namespace WINS {

#define WAVE_RECT_HEIGHT				0X200
#define WAVE_RECT_BORDER_TOP		25
#define WAVE_RECT_BORDER_LEFT		20
#define WAVE_RECT_BORDER_RIGHT		60
#define WAVE_RECT_BORDER_BOTTON		25

#define WIN_SDR_CLASS		"WIN_SDR_CLASS"

#define SDR_WIDTH	0x400
#define SDR_WIN_WIDTH	800

#define SDR_ZOOM_MAX	16
	//#define SDR_ZOOM_MIN	0.16


	class CWinSDR
	{
	public:
		typedef struct FFTInfo_STRUCT {
			uint32_t i;			//in buffer position
			uint32_t adc_buf_pos;	//in filtered adc buffer position
			uint32_t fft_size;
			uint32_t fft_data_size;
			double 	 fft_max_value;
			double 	 fft_min_value;
		} FFTInfo;

		typedef struct DRAWINFO_STRUCT {
			int			iHZoom, iHOldZoom, iVZoom, iVOldZoom, iHFit, iVFit;
			DWORD		dwDataWidth;
			DWORD		dwHZoomedWidth, dwHZoomedPos;
			WORD		wHSclPos, wVSclPos, wHSclMin, wHSclMax, wVSclMin, wVSclMax;

			BOOL		fAutoScroll;
			UINT		uTimerId;
		} DRAWINFO, * PDRAWINFO;
		DRAWINFO	DrawInfo;

		HWND	hWnd = NULL;

		UINT32  WinWidth, WinHeight;
		UINT32	WinWidthSpectrum = 0, WinHeightSpectrum = 0;
		UINT	PainttedFFT = 0;

		HWND hWndOrignalSpectrum = NULL;
		HWND hWndOrignalLogSpectrum = NULL;
		HWND hWndFilttedSpectrum = NULL;
		HWND hWndFilttedLogSpectrum = NULL;

		HWND hWndSpectrumHScrollBar;
		HWND hWndSpectrumVScrollBar;

		HWND hWndFFT = NULL;

		int WinOneSpectrumHeight = 100;

		int WinOneSpectrumHScrollPos = 0;
		int WinOneSpectrumVScrollPos = 0;
		double	WinOneSpectrumHScrollZoom = 1.0;
		double	WinOneSpectrumVScrollZoom = 1.0;

		HWND ActivehWnd = NULL;
		HMENU hMenu;
		bool SpectrumAutoFollow = true;

		HWND hWndTreeView;
		HWND hDlgSDRParams;
		HWND hWndTitle;
		HWND hWndEdit;
		HWND hWndCombox;
		HWND hWndComment;


	public:
		CWinSDR();
		~CWinSDR();

		void RegisterWindowsClass(void);
		void OpenWindow(void);

		void Init(void);
		void Paint(HWND hWnd);
		void ProcessKey(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		BOOL OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		void PaintFFT(void);
		void KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		void buildTreeItems(HWND hWndTreeView, HTREEITEM hItem, int* i);

		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK DlgSDRSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	};
}
extern WINS::CWinSDR clsWinSDR;