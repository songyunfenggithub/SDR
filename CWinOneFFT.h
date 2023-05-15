
#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

class CScreenButton;

namespace WINS {
	namespace SPECTRUM {

#define WIN_FFT_ONE_CLASS		"WIN_FFT_ONE_CLASS"

		class CWinOneFFT
		{
		public:

			UINT	uTimerId;

			INT MouseX;
			INT MouseY;
			double Hz = 0.0;


			INT VScrollPos = 0, VScrollRange = 0;
			double VScrollZoom = 1.0;

			HWND hWnd = NULL;
			RECT WinRect;

			BOOL bFFTOrignalShow = true;
			BOOL bFFTOrignalLogShow = true;
			BOOL bFFTFilttedShow = true;
			BOOL bFFTFilttedLogShow = true;

			char strMouse[1024];

			//CMessage msgs;

			bool readyForInitBuff = false;

			RECT ButtonRect = { 0 };

			CScreenButton* rfButton;
			CScreenButton* rfStepButton;

			CScreenButton* fsButton;
			CScreenButton* decimationFactorButton;
			CScreenButton* BWButton;
			CScreenButton* IFModeButton;
			CScreenButton* LOModeButton;
			CScreenButton* LOcModeButton;
			CScreenButton* averageFilterButton;
			CScreenButton* confirmP1RfGoButton;

			POINT ScreenP1 = { 0 }, ScreenP2 = { 0 };
			bool P1_Use = false, P2_Use = false;
			char strPP[1024];

			bool LeftBottonDown = false;
			POINT DragMousePoint;

			typedef struct AVERAGE_RANGE_POINTS_STRUCT {
				POINT P0 = { -1,0 };
				POINT P1 = { 0 };
			}AVERAGE_RANGE_POINTS;
			AVERAGE_RANGE_POINTS AverageRange;
			double AverageValue = 0.0;
			double AverageValueCalcul = 0.0;
			UINT AverageValueN = 0;

			INT CommNum = 0;

		public:
			CWinOneFFT();
			~CWinOneFFT();

			void RegisterWindowsClass(void);

			void Paint(void);
			bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
			void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);
			void OnMouse(void);
			void DragMoveFFT(void);
			void GetRealClientRect(PRECT lprc);
			void BrieflyBuff(void* fft);
			void PaintFFT(HDC hdc);
			void PaintBriefly(HDC hdc);
			void DrawPoint(HDC hdc, POINT* P, char* flag);
			void P2SubP1(void);

			void SaveValue(void);
			void RestoreValue(void);

			static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

			static void rfButtonFunc(CScreenButton* button);
			static void rfStepButtonFunc(CScreenButton* button);
			static void averageFilterButtonFunc(CScreenButton* button);
			static void confirmP1RfGoButtonFunc(CScreenButton* button);
		};
	}
}
extern WINS::SPECTRUM::CWinOneFFT clsWinOneFFT;
