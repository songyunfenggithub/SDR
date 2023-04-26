#pragma once

// Include CUDA runtime and CUFFT
#include <cuda_runtime.h>
#include <cufft.h>

// Helper functions for CUDA
#include "device_launch_parameters.h"

#include "public.h"

class cuda_CFFT
{
public:
	char* TAG = NULL;

	UINT FFTSize, FFTStep;
	cufftDoubleComplex* cuda_FFT_CompiData = NULL;
	cufftDoubleComplex* cuda_FFT_CompoData = NULL;
	cufftDoubleComplex* cuda_FFT_d_fftData = NULL;
	cufftDoubleComplex* cuda_FFT_d_outfftData = NULL;
	cufftHandle cuda_FFT_fft_plan = 0;// cuda library function handle

	double FFTMaxValue;

public:

	cuda_CFFT();
	~cuda_CFFT();

	void cuda_FFT_Init(UINT fftsize, UINT fftstep, UINT data_bits);
	void cuda_FFT_UnInit(void);

	void cuda_FFT_Prepare_Data(void* Buff, BUFF_DATA_TYPE type, UINT pos, UINT mask);

	void cuda_FFT(void);
	void cuda_FFT(void* Buff, BUFF_DATA_TYPE type, UINT pos, UINT mask);

	void cuda_FFT_Prepare_Data_for_MaxValue(double* buff);
	double Get_FFT_Max_Value(UINT data_bits);
};