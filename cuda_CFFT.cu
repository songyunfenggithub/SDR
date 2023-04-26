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

#include "cuda_CFFT.cuh"

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
    CUFFT_CALL(cudaMemcpy(cuda_FFT_d_fftData, cuda_FFT_CompiData, FFTSize * sizeof(cufftDoubleComplex), cudaMemcpyHostToDevice));// copy data from host to device
    
    //WaitForSingleObject(cuda_FFT_hMutexBuff, INFINITE);
    CUFFT_CALL(cufftExecZ2Z(cuda_FFT_fft_plan, (cufftDoubleComplex*)cuda_FFT_d_fftData, (cufftDoubleComplex*)cuda_FFT_d_outfftData, CUFFT_FORWARD));//execute
    CUFFT_CALL(cudaDeviceSynchronize());//wait to be done
    //ReleaseMutex(cuda_FFT_hMutexBuff);

    CUFFT_CALL(cudaMemcpy(cuda_FFT_CompoData, cuda_FFT_d_outfftData, FFTSize * sizeof(cufftDoubleComplex), cudaMemcpyDeviceToHost));// copy the result from device to host
    //printf("Time of cudaFFT: %fms\r\n", GetTickCount() - s);
}

void cuda_CFFT::cuda_FFT_Init(UINT fftsize, UINT fftstep, UINT data_bits)
{
    cuda_FFT_UnInit();

    FFTSize = fftsize;
    FFTStep = fftstep;

    if (cuda_FFT_CompiData != NULL) free(cuda_FFT_CompiData);
    cuda_FFT_CompiData = (cufftDoubleComplex*)malloc(FFTSize * sizeof(cufftDoubleComplex));//allocate memory for the data in host

    if (cuda_FFT_CompoData != NULL) free(cuda_FFT_CompoData);
    cuda_FFT_CompoData = (cufftDoubleComplex*)malloc(FFTSize * sizeof(cufftDoubleComplex));//allocate memory for the data in host

    if (cuda_FFT_d_fftData != NULL)  cudaFree(cuda_FFT_d_fftData);
    cudaMalloc((void**)&cuda_FFT_d_fftData, FFTSize * sizeof(cufftDoubleComplex));// allocate memory for the data in device

    if (cuda_FFT_d_outfftData != NULL)  cudaFree(cuda_FFT_d_outfftData);
    cudaMalloc((void**)&cuda_FFT_d_outfftData, FFTSize * sizeof(cufftDoubleComplex));// allocate memory for the data in device

    if (cuda_FFT_fft_plan != 0) cufftDestroy(cuda_FFT_fft_plan);
    cufftPlan1d(&cuda_FFT_fft_plan, FFTSize, CUFFT_Z2Z, 1);//declaration

    FFTMaxValue = Get_FFT_Max_Value(data_bits);
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
    printf("Cuda_CFFT_Closed.\r\n");
}

void cuda_CFFT::cuda_FFT(void* Buff, BUFF_DATA_TYPE type, UINT pos, UINT mask)
{
    cuda_FFT_Prepare_Data(Buff, type, pos, mask);
    cuda_FFT();
}

void cuda_CFFT::cuda_FFT_Prepare_Data(void* Buff, BUFF_DATA_TYPE type, UINT pos, UINT mask)
{
    int i;
    memset(cuda_FFT_CompiData, 0, FFTSize * sizeof(cufftDoubleComplex));
    switch(type)
    {
    case BUFF_DATA_TYPE::char_type:
    {
        char* buff = (char*)Buff;
        for (i = 0; i < FFTStep; i++, pos++) {
            cuda_FFT_CompiData[i].x = buff[pos & mask];
            cuda_FFT_CompiData[i].y = 0;
        }
    }
    break;
    case BUFF_DATA_TYPE::short_type:
    {
        short* buff = (short*)Buff;
        for (i = 0; i < FFTStep; i++, pos++) {
            cuda_FFT_CompiData[i].x = buff[pos & mask];
            cuda_FFT_CompiData[i].y = 0;
        }
    }
    break;
    case BUFF_DATA_TYPE::float_type:
    {
        float* buff = (float*)Buff;
        for (i = 0; i < FFTStep; i++, pos++) {
            cuda_FFT_CompiData[i].x = buff[pos & mask];
            cuda_FFT_CompiData[i].y = 0;
        }
    }
    break;

    }
}

void cuda_CFFT::cuda_FFT_Prepare_Data_for_MaxValue(double* buff)
{
    for (int i = 0; i < FFTSize; i++) {
        cuda_FFT_CompiData[i].x = (double)buff[i];
        cuda_FFT_CompiData[i].y = 0;
    }
}

double cuda_CFFT::Get_FFT_Max_Value(UINT data_bits)
{
    double* buff = new double[FFTSize];
    int i;
    double maxd = 0;
    UINT64 max = ((UINT64)1 << (data_bits - 1)) - 1;
    for (i = 0; i < FFTSize; i++) buff[i] = (double)max * sin(2 * M_PI * i / FFTSize);
    cuda_FFT_Prepare_Data_for_MaxValue(buff);
    cuda_FFT();
    int f = 1;
    maxd = sqrt(cuda_FFT_CompoData[f].x * cuda_FFT_CompoData[f].x + cuda_FFT_CompoData[f].y * cuda_FFT_CompoData[f].y);
    printf("maxvalue:%d, %lf\n", max, maxd);
    free(buff);
    return maxd;
}