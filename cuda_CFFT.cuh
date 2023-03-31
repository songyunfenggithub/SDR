#pragma once

// Include CUDA runtime and CUFFT
#include <cuda_runtime.h>
#include <cufft.h>

// Helper functions for CUDA
#include "device_launch_parameters.h"

#include "public.h"

class CFFT;

class cuda_CFFT
{
public:
	char* TAG = NULL;

	CFFT *fft = NULL;
	UINT FFTSize, FFTStep;

	cufftDoubleComplex* cuda_FFT_CompiData = NULL;
	cufftDoubleComplex* cuda_FFT_CompoData = NULL;
	cufftDoubleComplex* cuda_FFT_d_fftData = NULL;
	cufftDoubleComplex* cuda_FFT_d_outfftData = NULL;
	cufftHandle cuda_FFT_fft_plan = 0;// cuda library function handle

public:

	void cuda_FFT_Init(CFFT* fft);
	void cuda_FFT_UnInit(void);

	void cuda_FFT_Prepare_Data(void* Buff, BUFF_DATA_TYPE type, UINT pos, UINT mask);

	void cuda_FFT(void);
	void cuda_FFT(void* Buff, BUFF_DATA_TYPE type, UINT pos, UINT mask);

	void cuda_FFT_Prepare_Data_for_MaxValue(double* buff);
};