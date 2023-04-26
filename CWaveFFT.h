#pragma once

#include <stdint.h>

#include "CWaveFilter.h"

#define FFT_SIZE	0x100000
#define FFT_STEP	0x040000


#define FFT_DEEP		0x10
#define FFT_DEEP_MASK	(FFT_DEEP - 1)

#define FFT_VALUE_MAX 0xFF
#define FFT_MAX_SIZE 0xFFFF

class cuda_CFFT;

class CWaveFFT
{

public:

	////////////////////////////////////////////////////////////////////
	//定义一个复数结构体
	///////////////////////////////////////////////////////////////////
	typedef struct Complex_
	{
		double real;
		double imagin;
	} Complex;

public:

	HANDLE  hMutexBuff;

	double* FFTOrignalBuff =  NULL;
	double* FFTOrignalLogBuff = NULL;
	double* FFTFilttedBuff = NULL;
	double* FFTFilttedLogBuff = NULL;

	double* FFTOBuff = NULL ;
	double* FFTOLogBuff = NULL;
	double* FFTFBuff = NULL;
	double* FFTFLogBuff = NULL;

	UINT FFTOrignalBuffNum = 0;
	UINT FFTFilttedBuffNum = 0;
	UINT FFTDeep = FFT_DEEP;

	double*  FFT_src = NULL;
	Complex* FFT_src_com = NULL;

	UINT32 FFTPos = 0;
	UINT32 FFTFilttedPos = 0;

	UINT32 FFTStep = FFT_SIZE;
	UINT32 FFTSize = FFT_SIZE;
	UINT32 HalfFFTSize = FFT_SIZE / 2;
	UINT32 FFTCount = 0;
	float	FFTPerSec = 0.0;
	UINT32 FFTOrignalCount = 0;
	UINT32 FFTFilttedCount = 0;

	UINT32 FFTProcessingPos = 0;
	UINT32 FFTReadyPos = 0;
	UINT32 FFTPaintedPos = 0;
	
	bool FFTlog = true;

	bool FFTReady = false;

	bool FFTDoing = true;

	bool FFTThreadExit = false;

	double FFTMaxValue;
	double FFTMaxValue_Filtted;

	int ForwardSignal = 1;
	int ForwardFFTlog = 1;

	cuda_CFFT* orignal_FFT = NULL;
	cuda_CFFT* filtted_FFT = NULL;

public:
	CWaveFFT();
	~CWaveFFT();

	void LoadSettings(void);
	void SaveSettings(void);

	void InitAllBuff(UINT fftsize, UINT fftstep);
	void InitBuff(void);
	void setInput1(double* data, int  n);
	void setInput(double* data, int  n);
	void FFT(WHICHSIGNAL WhichSignal, UINT pos);
	void NormalFFT(WHICHSIGNAL WhichSignal, UINT pos);
	void FFT_orignal(WHICHSIGNAL WhichSignal, uint32_t pos);
	int  FFT_remap(double* src, int size_n);
	void IDFT(Complex* src, Complex* dst, int size);
	void DFT(double* src, Complex* dst, int size);
	void getWN(double n, double size_n, Complex* dst);
	void Multy_Complex(Complex* src1, Complex* src2, Complex* dst);
	void Sub_Complex(Complex* src1, Complex* src2, Complex* dst);
	void Add_Complex(Complex* src1, Complex* src2, Complex* dst);

	double GetFFTMaxValue(void);

	void FFT_for_FilterCore_Analyze(FILTER_CORE_DATA_TYPE* pbuf, CFilterWin* pFilterWin);

	static LPTHREAD_START_ROUTINE FFT_Thread(LPVOID lp);
	void FFT_func(void);
};

extern CWaveFFT clsWaveFFT;