#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdlib.h>
#include <time.h>
#include <Windows.h>

#include "public.h"
#include "CData.h"
#include "myDebug.h"
#include "CWaveFFT.h"
#include "CFilter.h"

#include "cuda_FFT.cuh"

//using namespace WINS;
//using namespace DEVICES;

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

cufftDoubleComplex* cuda_FFT_CompiData_filtted = NULL;
cufftDoubleComplex* cuda_FFT_CompoData_filtted = NULL;
cufftDoubleComplex* cuda_FFT_d_fftData_filtted = NULL;
cufftDoubleComplex* cuda_FFT_d_outfftData_filtted = NULL;
cufftHandle cuda_FFT_fft_plan_filtted = 0;// cuda library function handle

void cuda_FFT(WHICHSIGNAL WhichSignal)
{
    float cost, s;
    s = GetTickCount();
    if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
        CUFFT_CALL(cudaMemcpy(cuda_FFT_d_fftData, cuda_FFT_CompiData, clsWaveFFT.FFTSize * sizeof(cufftDoubleComplex), cudaMemcpyHostToDevice));// copy data from host to device
        //WaitForSingleObject(cuda_FFT_hMutexBuff, INFINITE);
        CUFFT_CALL(cufftExecZ2Z(cuda_FFT_fft_plan, (cufftDoubleComplex*)cuda_FFT_d_fftData, (cufftDoubleComplex*)cuda_FFT_d_outfftData, CUFFT_FORWARD));//execute
        CUFFT_CALL(cudaDeviceSynchronize());//wait to be done
        //ReleaseMutex(cuda_FFT_hMutexBuff);
        CUFFT_CALL(cudaMemcpy(cuda_FFT_CompoData, cuda_FFT_d_outfftData, clsWaveFFT.FFTSize * sizeof(cufftDoubleComplex), cudaMemcpyDeviceToHost));// copy the result from device to host
        //printf("Time of cudaFFT: %fms\r\n", GetTickCount() - s);
    }
    else {
        UINT n = (clsWaveFFT.FFTSize >> clsFilter.rootFilterInfo.decimationFactorBit);
        CUFFT_CALL(cudaMemcpy(cuda_FFT_d_fftData_filtted, cuda_FFT_CompiData_filtted, n * sizeof(cufftDoubleComplex), cudaMemcpyHostToDevice));// copy data from host to device
        //WaitForSingleObject(cuda_FFT_hMutexBuff, INFINITE);
        CUFFT_CALL(cufftExecZ2Z(cuda_FFT_fft_plan_filtted, (cufftDoubleComplex*)cuda_FFT_d_fftData_filtted, (cufftDoubleComplex*)cuda_FFT_d_outfftData_filtted, CUFFT_FORWARD));//execute
        CUFFT_CALL(cudaDeviceSynchronize());//wait to be done
        //ReleaseMutex(cuda_FFT_hMutexBuff);
        CUFFT_CALL(cudaMemcpy(cuda_FFT_CompoData_filtted, cuda_FFT_d_outfftData_filtted, n * sizeof(cufftDoubleComplex), cudaMemcpyDeviceToHost));// copy the result from device to host
        //printf("Time of cudaFFT: %fms\r\n", GetTickCount() - s);
    }
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

    FFTSize = clsWaveFFT.FFTSize >> clsFilter.rootFilterInfo.decimationFactorBit;
    if (cuda_FFT_CompiData_filtted != NULL) free(cuda_FFT_CompiData_filtted);
    cuda_FFT_CompiData_filtted = (cufftDoubleComplex*)malloc(FFTSize * sizeof(cufftDoubleComplex));//allocate memory for the data in host

    if (cuda_FFT_CompoData_filtted != NULL) free(cuda_FFT_CompoData_filtted);
    cuda_FFT_CompoData_filtted = (cufftDoubleComplex*)malloc(FFTSize * sizeof(cufftDoubleComplex));//allocate memory for the data in host

    if (cuda_FFT_d_fftData_filtted != NULL) cudaFree(cuda_FFT_d_fftData_filtted);
    cudaMalloc((void**)&cuda_FFT_d_fftData_filtted, FFTSize * sizeof(cufftDoubleComplex));// allocate memory for the data in device

    if (cuda_FFT_d_outfftData_filtted != NULL)  cudaFree(cuda_FFT_d_outfftData_filtted);
    cudaMalloc((void**)&cuda_FFT_d_outfftData_filtted, FFTSize * sizeof(cufftDoubleComplex));// allocate memory for the data in device

    if (cuda_FFT_fft_plan_filtted != 0) cufftDestroy(cuda_FFT_fft_plan_filtted);
    cufftPlan1d(&cuda_FFT_fft_plan_filtted, FFTSize >> clsFilter.rootFilterInfo.decimationFactorBit, CUFFT_Z2Z, 1);//declaration
}

void cuda_FFT_UnInit(void)
{
    clsWaveFFT.FFTDoing = false;
    while(clsWaveFFT.FFTThreadExit == false);

    if (cuda_FFT_CompiData != NULL) free(cuda_FFT_CompiData);
    if (cuda_FFT_CompoData != NULL) free(cuda_FFT_CompoData);
    if (cuda_FFT_d_fftData != NULL)  cudaFree(cuda_FFT_d_fftData);
    if (cuda_FFT_d_outfftData != NULL)  cudaFree(cuda_FFT_d_outfftData);
    if (cuda_FFT_fft_plan != 0) cufftDestroy(cuda_FFT_fft_plan);

    if (cuda_FFT_CompiData_filtted != NULL) free(cuda_FFT_CompiData_filtted);
    if (cuda_FFT_CompoData_filtted != NULL) free(cuda_FFT_CompoData_filtted);
    if (cuda_FFT_d_fftData_filtted != NULL) cudaFree(cuda_FFT_d_fftData_filtted);
    if (cuda_FFT_d_outfftData_filtted != NULL) cudaFree(cuda_FFT_d_outfftData_filtted);
    if (cuda_FFT_fft_plan_filtted != 0) cufftDestroy(cuda_FFT_fft_plan_filtted);
    DbgMsg("cuda_FFT_UnInit Closed.\r\n");
}

void cuda_FFT(WHICHSIGNAL WhichSignal, uint32_t pos)
{
    cuda_FFT_Prepare_Data(WhichSignal, pos);
    cuda_FFT(WhichSignal);
}

void cuda_FFT_Prepare_Data(WHICHSIGNAL WhichSignal, uint32_t pos)
{
    int i;
    if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
        memset(cuda_FFT_CompiData, 0, clsWaveFFT.FFTSize * sizeof(cufftDoubleComplex));
        switch (AdcData->DataType) {
        case short_type:
        {
            short* Buff = (short*)AdcData->Buff;
            for (i = 0; i < clsWaveFFT.FFTStep; i++, pos++) {
                cuda_FFT_CompiData[i].x = (double)Buff[pos & AdcData->Mask];
                cuda_FFT_CompiData[i].y = 0;
            }
        }
        break;
        case float_type:
        {
            float* Buff = (float*)AdcData->Buff;
            for (i = 0; i < clsWaveFFT.FFTStep; i++, pos++) {
                cuda_FFT_CompiData[i].x = (double)Buff[pos & AdcData->Mask];
                cuda_FFT_CompiData[i].y = 0;
            }
        }
        break;
        }
    }
    else {
        UINT n = clsWaveFFT.FFTStep >> clsFilter.rootFilterInfo.decimationFactorBit;
        memset(cuda_FFT_CompiData_filtted, 0, n * sizeof(cufftDoubleComplex));
        switch (AdcData->DataType) {
        case short_type:
        {
            short* Buff = (short*)AdcDataFiltted->Buff;
            for (i = 0; i < n; i++, pos++) {
                cuda_FFT_CompiData_filtted[i].x = (double)Buff[pos & AdcDataFiltted->Mask];
                cuda_FFT_CompiData_filtted[i].y = 0;
            }
        }
        break;
        case float_type:
        {
            float* Buff = (float*)AdcDataFiltted->Buff;
            for (i = 0; i < n; i++, pos++) {
                cuda_FFT_CompiData_filtted[i].x = (double)Buff[pos & AdcDataFiltted->Mask];
                cuda_FFT_CompiData_filtted[i].y = 0;
            }
        }
        break;
        }
    }
}

void cuda_FFT_Prepare_Data_for_MaxValue(WHICHSIGNAL WhichSignal, double *buff)
{
    UINT fftsize;
    if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
        fftsize = clsWaveFFT.FFTStep;
        for (int i = 0; i < fftsize; i++) {
            cuda_FFT_CompiData[i].x = (double)buff[i];
            cuda_FFT_CompiData[i].y = 0;
        }
    }
    else {
        fftsize = clsWaveFFT.FFTStep >> clsFilter.rootFilterInfo.decimationFactorBit;
        for (int i = 0; i < fftsize; i++) {
            cuda_FFT_CompiData_filtted[i].x = (double)buff[i];
            cuda_FFT_CompiData_filtted[i].y = 0;
        }
    }
}
