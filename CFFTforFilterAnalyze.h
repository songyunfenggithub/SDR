#pragma once

#include <stdint.h>

namespace METHOD {
	class cuda_CFFT;
}
using namespace METHOD;

namespace METHOD {

	class CFFTforFilterAnalyze
	{

	public:

		////////////////////////////////////////////////////////////////////
		//定义一个复数结构体
		///////////////////////////////////////////////////////////////////
		typedef struct Complex_STRUCT
		{
			double real;
			double imagin;
		} Complex;

	public:

	public:
		CFFTforFilterAnalyze();
		~CFFTforFilterAnalyze();

		void setInput1(double* data, int  n);
		void setInput(double* data, int  n);
		int  FFT_remap(double* src, int size_n);
		void IDFT(Complex* src, Complex* dst, int size);
		void DFT(double* src, Complex* dst, int size);
		void getWN(double n, double size_n, Complex* dst);
		void Multy_Complex(Complex* src1, Complex* src2, Complex* dst);
		void Sub_Complex(Complex* src1, Complex* src2, Complex* dst);
		void Add_Complex(Complex* src1, Complex* src2, Complex* dst);

		void FFT_for_FilterCore_Analyze(void* buff, void* fw);
	};
}

extern METHOD::CFFTforFilterAnalyze clsFilterFFT;
