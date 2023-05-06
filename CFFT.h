#pragma once

#include <stdint.h>

#include "cuda_CFFT.cuh"

class CData;

namespace METHOD {

#define WM_FFT	(WM_USER + 1)

	class CFFT :public cuda_CFFT
	{
	public:
		typedef struct Complex_
		{
			double real;
			double imagin;
		} Complex;

		HANDLE hMutexBuff = NULL;
		HANDLE hMutexDraw = NULL;

		HANDLE hFFT_Thread = NULL;

		HWND hWnd = NULL;
		CData* Data;

		COLORREF Color;		
		COLORREF ColorLog;

		double* FFTBuff = NULL;
		double* FFTLogBuff = NULL;

		double* FFTOutBuff = NULL;
		double* FFTOutLogBuff = NULL;
		double* FFTBrieflyBuff = NULL;
		double* FFTBrieflyLogBuff = NULL;

		double* FFT_src = NULL;
		Complex* FFT_src_com = NULL;

		bool FFTlog = true;
		bool FFTDoing = true;
		bool FFTNext = false;
		bool bFFT_Thread_Exitted = true;

	public:
		CFFT();
		~CFFT();

		void Init(void);
		void UnInit(void);

		void FFT(UINT pos);
		void NormalFFT(void* buff, BUFF_DATA_TYPE type, uint32_t pos, UINT mask);
		int  FFT_remap(double* src, int size_n);
		void IDFT(Complex* src, Complex* dst, int size);
		void DFT(double* src, Complex* dst, int size);
		void getWN(double n, double size_n, Complex* dst);
		void Multy_Complex(Complex* src1, Complex* src2, Complex* dst);
		void Sub_Complex(Complex* src1, Complex* src2, Complex* dst);
		void Add_Complex(Complex* src1, Complex* src2, Complex* dst);

		double GetFFTMaxValue(void);

		static LPTHREAD_START_ROUTINE FFT_Thread(LPVOID lp);
		void FFT_func(void);
	};
}
