
#pragma once

namespace METHOD {
	class CFFT;
}
using namespace METHOD;

namespace WINS {

	namespace SPECTRUM_SCAN {

#define WIN_SPECTRUM_SCAN_FFT_SHOW_CLASS		"WIN_SPECTRUM_SCAN_FFT_SHOW_CLASS"

		class CWinSpectrumScanFFTShow
		{
		public:
			HWND hWnd = NULL;
			RECT WinRect = { 0 };

			UINT uTimerId = 0;

			INT MouseX = 0;
			INT MouseY = 0;


			int VScrollPos = 0, VScrollRange = 0;
			int HScrollPos = 0, HScrollRange = 0;
			double HScrollZoom = 1.0;
			double VScrollZoom = 1.0;

			HANDLE  hDrawMutex = NULL;
			INT		SpectrumY = -1;
			HDC		hDCFFT = NULL;
			HBITMAP hBMPFFT = NULL;
			UINT CommNum = 0;

			CFFT* fft = NULL;

			UINT FFTHeight = 512;

			bool bFFTShow = true;
			bool bFFTLogShow = true;
			bool bFFTBrieflyShow = true;
			bool bFFTBrieflyLogShow = true;

			typedef enum SPECTRUM_SHOW_ENUM {
				show_orignal = 0,
				show_log,
				show_briefly,
				show_briefly_log
			}SPECTRUM_SHOW;
			SPECTRUM_SHOW SpectrumShow = show_briefly_log;

			double DBMin = -24.0;

			typedef struct AVERAGE_RANGE_POINTS_STRUCT {
				POINT P0 = { 0 };
				POINT P1 = { 0 };
			}AVERAGE_RANGE_POINTS;
			AVERAGE_RANGE_POINTS AverageRange;
			double AverageValue = 0.0;
			double AverageValueCalcul = 0.0;
			UINT AverageValueN = 0;

			POINT ScreenP1 = { 0 }, ScreenP2 = { 0 };
			bool P1_Use = false, P2_Use = false;
			char strPP[2048];

			char strMouse[1024] = { 0 };

			bool LeftBottonDown = false;
			POINT DragMousePoint;

			typedef struct SIGNAL_INFO_STRUCT {
				bool have_signal = false;
				UINT pos = 0;
				UINT signal_num = 0;
				UINT signal_width = 0;
			}SIGNAL_INFO;

			SIGNAL_INFO *Signals = NULL;

		public:
			CWinSpectrumScanFFTShow();
			~CWinSpectrumScanFFTShow();

			void RegisterWindowsClass(void);

			void Paint(void);
			bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
			void OnMouse(void);
			void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

			void BrieflyBuff(CFFT* fft);
			void InitDrawBuff(void);
			void PaintSpectrum(CFFT* fft);
			void SpectrumToWin(HDC hDC);
			void PaintFFTBriefly(HDC hdc, RECT* rt, CFFT* fft);
			void PaintFFT(HDC hdc, RECT* rt, CFFT* fft);
			void DrawPoint(HDC hdc, POINT* P, char* flag);
			void DragMoveFFT(void);
			void DrawSignals(HDC hdc);
			void P2SubP1(UINT message);

			LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

			static void FFTProcessCallBackInit(CFFT* fft);
			static void FFTProcessCallBack(CFFT* fft);
		};
	}
}

extern WINS::SPECTRUM_SCAN::CWinSpectrumScanFFTShow clsWinSpectrumScanFFTShow;