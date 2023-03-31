#pragma once

// Include CUDA runtime and CUFFT
#include <cuda_runtime.h>
#include <cufft.h>

// Helper functions for CUDA
#include "device_launch_parameters.h"

#include "CWaveData.h"
#include "CWaveFFT.h"

extern cufftDoubleComplex* cuda_FFT_CompoData;

void cuda_FFT(void);
void cuda_FFT(WHICHSIGNAL WhichSignal, uint32_t pos);
void cuda_FFT_Init(void);
void cuda_FFT_UnInit(void);
void cuda_FFT_Prepare_Data(WHICHSIGNAL WhichSignal, uint32_t pos);
void cuda_FFT_Prepare_Data_for_MaxValue(double* buff);