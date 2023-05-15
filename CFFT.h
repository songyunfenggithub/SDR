#pragma once

#include <stdint.h>

#include "cuda_CFFT.cuh"

class CData;

namespace METHOD {

#define WM_FFT	(WM_USER + 1)

	class CFFT :public cuda_CFFT
	{
	public:
		
		typedef void (*FFTProcessCallBack)(CFFT* fft);

		typedef struct Complex_
		{
			double real;
			double imagin;
		} Complex;

		HANDLE hMutexBuff = NULL;
		HANDLE hMutexDraw = NULL;

		const char* Flag = NULL;

		HWND hWnd = NULL;
		CData* Data;

		HPEN hPen = NULL;
		HPEN hPenLog = NULL;

		double* FFTBuff = NULL;
		double* FFTLogBuff = NULL;

		double* FFTOutBuff = NULL;
		double* FFTOutLogBuff = NULL;
		double* FFTBrieflyBuff = NULL;
		double* FFTBrieflyLogBuff = NULL;

		double* FFT_src = NULL;
		Complex* FFT_src_com = NULL;

		HANDLE hThread = NULL;
		bool FFTlog = true;
		bool FFTDoing = true;
		bool FFTNext = false;
		bool Thread_Exit = true;

		bool bShow = true;
		bool bLogShow = true;

		FFTProcessCallBack FFT_Process_CallBack = NULL;
		FFTProcessCallBack FFT_Process_CallBackInit = NULL;

	public:
		CFFT(const char* flag, FFT_INFO *fftInfo);
		~CFFT();

		void Init(UINT fftsize, UINT fftstep, UINT averagedeep);
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
		void FFT_Thread_func(void);

		void RestoreValue(void);
		void SaveValue(void);
	};
}
