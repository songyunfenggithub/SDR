#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include "CWaveData.h"
#include "CWaveFFT.h"

#include "cuda_FFT.cuh"


#define CUFFT_CALL( call )                                                                                             \
    {                                                                                                                  \
        auto status = static_cast<cufftResult>( call );                                                                \
        if ( status != CUFFT_SUCCESS )                                                                                 \
            fprintf( stderr,                                                                                           \
                     "ERROR: CUFFT call \"%s\" in line %d of file %s failed "                                          \
                     "with "                                                                                           \
                     "code (%d).\n",                                                                                   \
                     #call,                                                                                            \
                     __LINE__,                                                                                         \
                     __FILE__,                                                                                         \
                     status );                                                                                         \
    }

cufftDoubleComplex* cuda_FFT_CompiData = NULL;
cufftDoubleComplex* cuda_FFT_CompoData = NULL;
cufftDoubleComplex* cuda_FFT_d_fftData = NULL;
cufftDoubleComplex* cuda_FFT_d_outfftData = NULL;
cufftHandle cuda_FFT_fft_plan = 0;// cuda library function handle

void cuda_FFT(void)
{
    float cost, s;
    s = GetTickCount();
    CUFFT_CALL(cudaMemcpy(cuda_FFT_d_fftData, cuda_FFT_CompiData, clsWaveFFT.FFTSize * sizeof(cufftDoubleComplex), cudaMemcpyHostToDevice));// copy data from host to device
    //WaitForSingleObject(cuda_FFT_hMutexBuff, INFINITE);
    CUFFT_CALL(cufftExecZ2Z(cuda_FFT_fft_plan, (cufftDoubleComplex*)cuda_FFT_d_fftData, (cufftDoubleComplex*)cuda_FFT_d_outfftData, CUFFT_FORWARD));//execute
    CUFFT_CALL(cudaDeviceSynchronize());//wait to be done
    //ReleaseMutex(cuda_FFT_hMutexBuff);
    CUFFT_CALL(cudaMemcpy(cuda_FFT_CompoData, cuda_FFT_d_outfftData, clsWaveFFT.FFTSize * sizeof(cufftDoubleComplex), cudaMemcpyDeviceToHost));// copy the result from device to host
    //printf("Time of cudaFFT: %fms\r\n", GetTickCount() - s);
}

void cuda_FFT_Init(void)
{
    UINT FFTSize = clsWaveFFT.FFTSize;
    if(cuda_FFT_CompiData != NULL) free(cuda_FFT_CompiData);
    cuda_FFT_CompiData = (cufftDoubleComplex*)malloc(FFTSize * sizeof(cufftDoubleComplex));//allocate memory for the data in host

    if (cuda_FFT_CompoData != NULL) free(cuda_FFT_CompoData);
    cuda_FFT_CompoData = (cufftDoubleComplex*)malloc(FFTSize * sizeof(cufftDoubleComplex));//allocate memory for the data in host
      
    if(cuda_FFT_d_fftData != NULL)  cudaFree(cuda_FFT_d_fftData);
    cudaMalloc((void**)&cuda_FFT_d_fftData, FFTSize * sizeof(cufftDoubleComplex));// allocate memory for the data in device

    if (cuda_FFT_d_outfftData != NULL)  cudaFree(cuda_FFT_d_outfftData);
    cudaMalloc((void**)&cuda_FFT_d_outfftData, FFTSize * sizeof(cufftDoubleComplex));// allocate memory for the data in device

    if(cuda_FFT_fft_plan != 0) cufftDestroy(cuda_FFT_fft_plan);
    cufftPlan1d(&cuda_FFT_fft_plan, FFTSize, CUFFT_Z2Z, 1);//declaration
}

void cuda_FFT_UnInit(void)
{
    while(clsWaveFFT.FFTThreadExit == false);

    if (cuda_FFT_CompiData != NULL) free(cuda_FFT_CompiData);
    if (cuda_FFT_CompoData != NULL) free(cuda_FFT_CompoData);
    if (cuda_FFT_d_fftData != NULL)  cudaFree(cuda_FFT_d_fftData);
    if (cuda_FFT_d_outfftData != NULL)  cudaFree(cuda_FFT_d_outfftData);
    if (cuda_FFT_fft_plan != 0) cufftDestroy(cuda_FFT_fft_plan);
    printf("Cuda_FFT_Closed.\r\n");
}

void cuda_FFT(WHICHSIGNAL WhichSignal, uint32_t pos)
{
    cuda_FFT_Prepare_Data(WhichSignal, pos);
    cuda_FFT();
}

void cuda_FFT_Prepare_Data(WHICHSIGNAL WhichSignal, uint32_t pos)
{
    int i;
    memset(cuda_FFT_CompiData, 0, clsWaveFFT.FFTSize * sizeof(cufftDoubleComplex));
    if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
        for (i = 0; i < clsWaveFFT.FFTStep; i++, pos++) {
            cuda_FFT_CompiData[i].x = (double)clsWaveData.AdcBuff[pos & DATA_BUFFER_MASK];
            cuda_FFT_CompiData[i].y = 0;
        }
    }
    else {
        for (i = 0; i < clsWaveFFT.FFTStep; i++, pos++) {
            cuda_FFT_CompiData[i].x = (double)clsWaveData.FilttedBuff[pos & DATA_BUFFER_MASK];
            cuda_FFT_CompiData[i].y = 0;
        }
    }
}

void cuda_FFT_Prepare_Data_for_MaxValue(double *buff)
{
    for (int i = 0; i < clsWaveFFT.FFTSize; i++) {
        cuda_FFT_CompiData[i].x = (double)buff[i];
        cuda_FFT_CompiData[i].y = 0;
    }
}
