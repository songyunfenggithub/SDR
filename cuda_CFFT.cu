#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdlib.h>
#include <time.h>
#include <Windows.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "Debug.h"
#include "CData.h"
#include "cuda_CFFT.cuh"

using namespace METHOD;

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

cuda_CFFT::cuda_CFFT()
{

}

cuda_CFFT::~cuda_CFFT()
{
    cuda_FFT_UnInit();
}

void cuda_CFFT::cuda_FFT(void)
{
    float cost, s;
    s = GetTickCount();
    CUFFT_CALL(cudaMemcpy(cuda_FFT_d_fftData, cuda_FFT_CompiData, FFTInfo->FFTSize * sizeof(cufftDoubleComplex), cudaMemcpyHostToDevice));// copy data from host to device
    
    //WaitForSingleObject(cuda_FFT_hMutexBuff, INFINITE);
    CUFFT_CALL(cufftExecZ2Z(cuda_FFT_fft_plan, (cufftDoubleComplex*)cuda_FFT_d_fftData, (cufftDoubleComplex*)cuda_FFT_d_outfftData, CUFFT_FORWARD));//execute
    CUFFT_CALL(cudaDeviceSynchronize());//wait to be done
    //ReleaseMutex(cuda_FFT_hMutexBuff);

    CUFFT_CALL(cudaMemcpy(cuda_FFT_CompoData, cuda_FFT_d_outfftData, FFTInfo->FFTSize * sizeof(cufftDoubleComplex), cudaMemcpyDeviceToHost));// copy the result from device to host
    //DbgMsg("Time of cudaFFT: %fms\r\n", GetTickCount() - s);
}

void cuda_CFFT::cuda_FFT_Init(CData* data)
{
    cuda_FFT_UnInit();

    Data = data;

    if (cuda_FFT_CompiData != NULL) free(cuda_FFT_CompiData);
    cuda_FFT_CompiData = (cufftDoubleComplex*)malloc(FFTInfo->FFTSize * sizeof(cufftDoubleComplex));//allocate memory for the data in host

    if (cuda_FFT_CompoData != NULL) free(cuda_FFT_CompoData);
    cuda_FFT_CompoData = (cufftDoubleComplex*)malloc(FFTInfo->FFTSize * sizeof(cufftDoubleComplex));//allocate memory for the data in host

    if (cuda_FFT_d_fftData != NULL)  cudaFree(cuda_FFT_d_fftData);
    cudaMalloc((void**)&cuda_FFT_d_fftData, FFTInfo->FFTSize * sizeof(cufftDoubleComplex));// allocate memory for the data in device

    if (cuda_FFT_d_outfftData != NULL)  cudaFree(cuda_FFT_d_outfftData);
    cudaMalloc((void**)&cuda_FFT_d_outfftData, FFTInfo->FFTSize * sizeof(cufftDoubleComplex));// allocate memory for the data in device

    if (cuda_FFT_fft_plan != 0) cufftDestroy(cuda_FFT_fft_plan);
    cufftPlan1d(&cuda_FFT_fft_plan, FFTInfo->FFTSize, CUFFT_Z2Z, 1);//declaration

    FFTMaxValue = Get_FFT_Max_Value();
}

void cuda_CFFT::cuda_FFT_UnInit(void)
{
    if (cuda_FFT_CompiData != NULL) {
        free(cuda_FFT_CompiData);
        cuda_FFT_CompiData = NULL;
    }
    if (cuda_FFT_CompoData != NULL) {
        free(cuda_FFT_CompoData);
        cuda_FFT_CompoData = NULL;
    }
    if (cuda_FFT_d_fftData != NULL) {
        cudaFree(cuda_FFT_d_fftData);
        cuda_FFT_d_fftData = NULL;
    }
    if (cuda_FFT_d_outfftData != NULL) {
        cudaFree(cuda_FFT_d_outfftData);
        cuda_FFT_d_outfftData = NULL;
    }
    if (cuda_FFT_fft_plan != 0) {
        cufftDestroy(cuda_FFT_fft_plan);
        cuda_FFT_fft_plan = 0;
    }
    DbgMsg("Cuda_CFFT_Closed.\r\n");
}

void cuda_CFFT::cuda_FFT(UINT pos)
{
    cuda_FFT_Prepare_Data(pos);
    cuda_FFT();
}

void cuda_CFFT::cuda_FFT_Prepare_Data(UINT pos)
{
    int i;
    CData* cData = (CData*)Data;
    memset(cuda_FFT_CompiData, 0, FFTInfo->FFTSize * sizeof(cufftDoubleComplex));
    switch(cData->DataType)
    {
    case BUFF_DATA_TYPE::char_type:
    {
        char* buff = (char*)cData->Buff;
        for (i = 0; i < FFTInfo->FFTStep; i++, pos++) {
            cuda_FFT_CompiData[i].x = buff[pos & cData->Mask];
            cuda_FFT_CompiData[i].y = 0;
        }
    }
    break;
    case BUFF_DATA_TYPE::short_type:
    {
        short* buff = (short*)cData->Buff;
        for (i = 0; i < FFTInfo->FFTStep; i++, pos++) {
            cuda_FFT_CompiData[i].x = buff[pos & cData->Mask];
            cuda_FFT_CompiData[i].y = 0;
        }
    }
    break;
    case BUFF_DATA_TYPE::float_type:
    {
        float* buff = (float*)cData->Buff;
        for (i = 0; i < FFTInfo->FFTStep; i++, pos++) {
            cuda_FFT_CompiData[i].x = buff[pos & cData->Mask];
            cuda_FFT_CompiData[i].y = 0;
        }
    }
    break;
    }
}

void cuda_CFFT::cuda_FFT_Prepare_Data_for_MaxValue(double* buff)
{
    for (int i = 0; i < FFTInfo->FFTSize; i++) {
        cuda_FFT_CompiData[i].x = (double)buff[i];
        cuda_FFT_CompiData[i].y = 0;
    }
}

double cuda_CFFT::Get_FFT_Max_Value(void)
{
    double* buff = new double[FFTInfo->FFTSize];
    int i;
    double maxd = 0;
    UINT64 max = ((UINT64)1 << (Data->DataBits -1)) - 1;
    for (i = 0; i < FFTInfo->FFTSize; i++) buff[i] = (double)max * sin(2 * M_PI * i / FFTInfo->FFTSize);
    cuda_FFT_Prepare_Data_for_MaxValue(buff);
    cuda_FFT();
    int f = 1;
    maxd = sqrt(cuda_FFT_CompoData[f].x * cuda_FFT_CompoData[f].x + cuda_FFT_CompoData[f].y * cuda_FFT_CompoData[f].y);
    DbgMsg("maxvalue:%d, %lf\n", max, maxd);
    free(buff);
    return maxd;
}