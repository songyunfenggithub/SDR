#pragma once

#include <stdint.h>

#include "cuda_CFFT.cuh"

#define FFT_SIZE	0x100000
#define FFT_STEP	0x040000


#define FFT_DEEP		0x10
#define FFT_DEEP_MASK	(FFT_DEEP - 1)

#define FFT_VALUE_MAX	0xFF
#define FFT_MAX_SIZE	0xFFFF

class CFFTWin;

class CFFT:public cuda_CFFT
{

public:

	typedef struct Complex_
	{
		double real;
		double imagin;
	} Complex;

	HANDLE  hMutexBuff;

	CFFTWin *fftWin = NULL;

	//cuda_CFFT *cuda_fft = NULL;
	
	void*	DataBuff;
	UINT32* DataBuffPos = NULL;
	UINT	data_buff_data_bits = 12;
	UINT	data_buff_length_mask;
	
	double* FFTBuff =  NULL;
	double* FFTLogBuff = NULL;

	double* FFTOutBuff = NULL ;
	double* FFTOutLogBuff = NULL;

	UINT average_Deep = FFT_DEEP;
	UINT average_Deep_mask = average_Deep - 1;
	UINT average_Deep_num = 0;

	double*  FFT_src = NULL;
	Complex* FFT_src_com = NULL;

	UINT32 FFTPos = 0;
	UINT32 FFTStep = FFT_SIZE;
	UINT32 FFTSize = FFT_SIZE;
	UINT32 HalfFFTSize = FFT_SIZE / 2;
	UINT32 FFTCount = 0;
	float	FFTPerSec = 0.0;

	bool FFTlog = true;

	bool FFTDoing = true;
	
	bool FFTNext = false;

	bool FFTT_hread_Exit = false;

	double FFTMaxValue;


public:
	CFFT();
	~CFFT();

	void Init(CFFTWin* fftwin);
	void UnInit(void);

	void FFT(void* Buff, BUFF_DATA_TYPE type, uint32_t pos, UINT mask);
	void NormalFFT(void* Buff, BUFF_DATA_TYPE type, uint32_t pos, UINT mask);
	int  FFT_remap(double* src, int size_n);
	void IDFT(Complex* src, Complex* dst, int size);
	void DFT(double* src, Complex* dst, int size);
	void getWN(double n, double size_n, Complex* dst);
	void Multy_Complex(Complex* src1, Complex* src2, Complex* dst);
	void Sub_Complex(Complex* src1, Complex* src2, Complex* dst);
	void Add_Complex(Complex* src1, Complex* src2, Complex* dst);

	double GetFFTMaxValue(void);
	double Get_FFT_Max_Value(void);

	static LPTHREAD_START_ROUTINE FFT_Thread(LPVOID lp);
	void FFT_func(void);
};