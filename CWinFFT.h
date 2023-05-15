#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

#include "CMessage.h"

namespace METHOD{
	class CFFT;
}
using namespace METHOD;

class CData;

namespace WINS {

#define WIN_FFT_CLASS	"WIN_FFT_CLASS"
#define FFT_DEEP	0x10

	class CWinFFT
	{
	public:
		const char* Flag = NULL;

		HWND hWnd = NULL;
		//HMENU hMenu = NULL;
		//HMENU hMenuSpect = NULL;

		UINT uTimerId;

		INT MouseX;
		INT MouseY;

		RECT WinRect;

		char strMouse[1024] = { 0 };

		CMessage msgs;

		int VScrollPos = 0, VScrollRange = 0;
		int HScrollPos = 0, HScrollRange = 0;
		double HScrollZoom = 1.0;
		double VScrollZoom = 1.0;

		CData* Data = NULL;
		CFFT* fft = NULL;

		CData* Data2 = NULL;
		CFFT* fft2 = NULL;

		double DBMin = -24.0;

		UINT FFTHeight = 512;

		HANDLE  hDrawMutex;
		INT		SpectrumY = -1;
		HDC		hDCFFT = NULL;
		HBITMAP hBMPFFT = NULL;
		UINT CommNum = 0;

		bool isDrawBriefly = true;
		bool isSpectrumZoomedShow = true;
		bool isDrawLogSpectrum = true;
		CFFT* DrawWhich = NULL;

		typedef struct POIINT_SERIAL;
		struct POIINT_SERIAL {
			POINT P = { 0 };
			POIINT_SERIAL* next = NULL;
		};
		POIINT_SERIAL* FilterPsHead = NULL;

		typedef struct AVERAGE_RANGE_POINTS_STRUCT {
			POINT P0 = { 0 };
			POINT P1 = { 0 };
		}AVERAGE_RANGE_POINTS;
		AVERAGE_RANGE_POINTS AverageRange;

		double AverageValue = 0.0;

	public:
		CWinFFT();
		~CWinFFT();

		void Init(void);
		void UnInit(void);

		void RegisterWindowsClass(void);
		void Paint(void);
		bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
		void OnMouse(void);
		void GetRealClientRect(PRECT lprc);
		void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

		void PaintFFT(HDC hdc, RECT* rt, CFFT* fft);
		void PaintFFTBriefly(HDC hdc, RECT* rt, CFFT* fft);

		void BrieflyBuff(CFFT* fft);
		void PaintSpectrum(CFFT* fft);
		void SpectrumToWin(HDC hDC);
		void InitDrawBuff(void);

		void DrawPoint(HDC hdc, POINT* P, char* Flag);
		void DrawPoints(HDC hdc);

		void RestoreValue(void);
		void SaveValue(void);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK DlgFFTSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	};
}