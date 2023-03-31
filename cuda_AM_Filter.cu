
#include "stdafx.h"

#include <stdio.h>
#include <random>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "myDebug.h"
#include "CWaveData.h"
#include "CWaveFilter.h"
#include "CDemodulatorAM.h"

#include "cuda_AM_Filter.cuh"


FILTERCOREDATATYPE* d_AM_Filter_Core = NULL;
ADCDATATYPE* d_AM_SrcData = NULL;
FILTEDDATATYPE* d_AM_Filtted_Result = NULL;
FILTEDDATATYPE* d_AM_Demodulator_Result = NULL;

//FILTERCOREDATATYPE* h_Filter_Core = NULL;
ADCDATATYPE* h_AM_SrcData = NULL;
FILTEDDATATYPE* h_AM_Filtted_Result = NULL;
FILTEDDATATYPE* h_AM_Demodulator_Result = NULL;

size_t cuda_AM_thread_size;
size_t cuda_AM_filter_result_step_size;
size_t cuda_AM_demodulator_result_size;
size_t cuda_AM_src_step_size;

__global__ void
cuda_AM_Filter(const ADCDATATYPE* src, const FILTERCOREDATATYPE* core, FILTEDDATATYPE* filtted_result, FILTEDDATATYPE* demodulator_result, 
	int stage, unsigned int corelen)
{
	//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	
	__shared__ float cache[CUDA_FILTER_BUFF_SRC_LENGTH >> DEMODULATOR_AM_DECIMATION_FACTOR_BIT];
	unsigned int cache_length_mask = (CUDA_FILTER_BUFF_SRC_LENGTH >> DEMODULATOR_AM_DECIMATION_FACTOR_BIT) - 1;
	unsigned int cache_stage_length = CUDA_FILTER_BUFF_STEP_LENGTH >> DEMODULATOR_AM_DECIMATION_FACTOR_BIT;
	unsigned int cache_stage_step = stage * cache_stage_length;

	unsigned int src_stage_step = stage * CUDA_FILTER_BUFF_STEP_LENGTH;

	unsigned int index = blockIdx.x;
	unsigned int i_stage_step = cache_stage_step + index - corelen;

	cache[cache_stage_step + index] = src[src_stage_step + (index << DEMODULATOR_AM_DECIMATION_FACTOR_BIT)];

	//对线程块中的线程进行同步
	__syncthreads();

	filtted_result[index] = 0;
	for (int i = 0; i < corelen; i++) {
		filtted_result[cache_stage_step + index] += cache[(i_stage_step + i) & cache_length_mask] * core[i];
	}
	
	//对线程块中的线程进行同步
	__syncthreads();
	
	unsigned int n = 1 << DEMODULATOR_AM_FILTER_SAMPLERATE_OFFSET_BIT;
	unsigned int mask = n - 1;
	unsigned int demodulator_result_index;
	float d;
	if ((index & mask) == 0) {
		demodulator_result_index = index >> DEMODULATOR_AM_FILTER_SAMPLERATE_OFFSET_BIT;
		demodulator_result[demodulator_result_index] = 0;
		for (int i = 0; i < n; i++) {
			if((d = filtted_result[cache_stage_step + index + i]) > 0) demodulator_result[demodulator_result_index] += d;
		}
	}
}

void Cuda_AM_ReInitFilterCore(CWaveFilter::PFILTERINFO pFilterInfo)
{
	cudaError_t err = cudaSuccess;

	if (d_AM_Filter_Core != NULL) {
		err = cudaFree(d_AM_Filter_Core);
		if (err != cudaSuccess) {
			printf("cudaFree d_AM_Filter_Core failed!\r\n");
			//goto Error;
		}
	}
	//if(h_Filter_Core != NULL)free(h_Filter_Core);

	size_t filter_core_size = pFilterInfo->CoreLength * sizeof(FILTERCOREDATATYPE);
	// Allocate the host input vector Core
	//h_Filter_Core = (FILTERCOREDATATYPE*)malloc(filter_core_size);

	//memcpy(h_Filter_Core, clsWaveFilter.FilterCore, filter_core_size);

	// Allocate the device input vector Filter_Core
	err = cudaMalloc((void**)&d_AM_Filter_Core, filter_core_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_AM_Filter_Core failed!\r\n");
		//goto Error;
	}
	err = cudaMemcpy(d_AM_Filter_Core, pFilterInfo->FilterCore, filter_core_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		printf("cudaMemcpy d_AM_Filter_Core failed!\r\n");
		//goto Error;
	}
	if (d_AM_Demodulator_Result != NULL) {
		err = cudaFree(d_AM_Demodulator_Result);
		if (err != cudaSuccess) {
			printf("cudaFree d_AM_Demodulator_Result failed!\r\n");
			//goto Error;
		}
	}
	cuda_AM_demodulator_result_size = (clsWaveData.AdcSampleRate >> (DEMODULATOR_AM_FILTER_SAMPLERATE_OFFSET_BIT + DEMODULATOR_AM_DECIMATION_FACTOR_BIT)) * sizeof(ADCDATATYPE);
	err = cudaMalloc((void**)&d_AM_Demodulator_Result, cuda_AM_demodulator_result_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_AM_Demodulator_Result failed!\r\n");
		//goto Error;
	}

	printf("Cuda ReInit AM Filter Core Done.\r\n");

}

void cuda_AM_getThreadNum(void)
{
	cudaDeviceProp prop;
	int count;

	cudaGetDeviceCount(&count);
	printf("gpu num %d\n", count);
	cudaGetDeviceProperties(&prop, 0);
	printf("max thread num: %d\n", prop.maxThreadsPerBlock);
	printf("max grid dimensions: %d, %d, %d)\n",
		prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
}

void Cuda_AM_Filter_Init(void)
{

	cuda_AM_getThreadNum();

	// Error code to check return values for CUDA calls
	cudaError_t err = cudaSuccess;

	// Print the vector length to be used, and compute its size
	cuda_AM_src_step_size = CUDA_FILTER_BUFF_STEP_LENGTH * sizeof(ADCDATATYPE);
	printf("Cuda_Init [Vector addition of %d CUDA_FILTER_BUFF_SRC_LENGTH]\n", CUDA_FILTER_BUFF_SRC_LENGTH);

	// Allocate the host input vector A
	//h_AM_SrcData = (ADCDATATYPE*)malloc(src_data_size);

	cuda_AM_thread_size = CUDA_FILTER_BUFF_STEP_LENGTH >> DEMODULATOR_AM_DECIMATION_FACTOR_BIT;
	cuda_AM_filter_result_step_size = CUDA_FILTER_BUFF_STEP_LENGTH >> DEMODULATOR_AM_DECIMATION_FACTOR_BIT * sizeof(FILTEDDATATYPE);
	// Allocate the host output vector Result
	//float* h_AM_Result = (float*)malloc(cuda_AM_result_size);

	// Allocate the device input vector A
	size_t src_data_size = CUDA_FILTER_BUFF_SRC_LENGTH * sizeof(ADCDATATYPE);
	err = cudaMalloc((void**)&d_AM_SrcData, src_data_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_AM_SrcData failed!\r\n");
		//goto Error;
	}
	// Allocate the device output vector C
	cuda_AM_filter_result_step_size = (CUDA_FILTER_BUFF_STEP_LENGTH >> DEMODULATOR_AM_DECIMATION_FACTOR_BIT) * sizeof(ADCDATATYPE);
	size_t filter_result_size = (CUDA_FILTER_BUFF_SRC_LENGTH >> DEMODULATOR_AM_DECIMATION_FACTOR_BIT) * sizeof(ADCDATATYPE);
	err = cudaMalloc((void**)&d_AM_Filtted_Result, filter_result_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_AM_Filtted_Result failed!\r\n");
		//goto Error;
	}

	Cuda_AM_ReInitFilterCore(clsDemodulatorAm.pFilterInfo);

	printf("Cuda Demodulator AM Init Done.\r\n");
}

void Cuda_AM_Filter_UnInit(void)
{

	while (clsWaveFilter.cud_Filter_exit == false);

	cudaError_t err = cudaSuccess;

	// Free device global memory
	err = cudaFree(d_AM_SrcData);
	if (err != cudaSuccess) {
		printf("cudaFree d_AM_SrcData failed!\r\n");
		//goto Error;
	}
	err = cudaFree(d_AM_Filter_Core);
	if (err != cudaSuccess) {
		printf("cudaFree d_AM_Filter_Core failed!\r\n");
		//goto Error;
	}
	err = cudaFree(d_AM_Filtted_Result);
	if (err != cudaSuccess) {
		printf("cudaFree d_AM_Filtted_Result failed!\r\n");
		//goto Error;
	}
	err = cudaFree(d_AM_Demodulator_Result);
	if (err != cudaSuccess) {
		printf("cudaFree d_AM_Demodulator_Result failed!\r\n");
		//goto Error;
	}

	// Free host memory
	//free(h_Filter_Core);

	printf("Cuda_Filter_Closed.\r\n");
}

void cuda_AM_Filtting(void)
{
	cudaError_t err = cudaSuccess;
	static unsigned int stage = 0;

	size_t threadsPerBlock = cuda_AM_thread_size;
	//int blocksPerGrid = (numElements + threadsPerBlock - 1) / threadsPerBlock;
	size_t blocksPerGrid = 1;

	h_AM_SrcData = (ADCDATATYPE*)clsWaveData.AdcBuff + clsWaveData.FilttedPos;
	//printf("stage:%d\r\n", stage);
	err = cudaMemcpy((char*)d_AM_SrcData + stage * cuda_AM_src_step_size, h_AM_SrcData, cuda_AM_src_step_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		printf("cudaMemcpy d_AM_SrcData failed!\r\n");
	}

	WaitForSingleObject(clsWaveFilter.hCoreMutex, INFINITE);

	//printf("CUDA kernel launch with %d blocks of %d threads\n", blocksPerGrid, threadsPerBlock);
	cuda_AM_Filter << <threadsPerBlock, blocksPerGrid >> > (d_AM_SrcData, d_AM_Filter_Core, h_AM_Filtted_Result, d_AM_Demodulator_Result, 
		stage, clsDemodulatorAm.pFilterInfo->CoreLength);
	err = cudaGetLastError();
	if (err != cudaSuccess) {
		printf("cudaFilter launch failed: %s\r\n", cudaGetErrorString(err));
	}
	// any errors encountered during the launch.
	err = cudaDeviceSynchronize();
	if (err != cudaSuccess) {
		printf("cudaDeviceSynchronize returned error code %d after launching cudaFilter!\r\n", err);
	}

	ReleaseMutex(clsWaveFilter.hCoreMutex);

	h_AM_Filtted_Result = (FILTEDDATATYPE*)clsDemodulatorAm.FilttedBuff + clsDemodulatorAm.FilttedPos;
	err = cudaMemcpy(h_AM_Filtted_Result, d_AM_Filtted_Result + stage * cuda_AM_filter_result_step_size, cuda_AM_filter_result_step_size, cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		printf("cudaMemcpy h_AM_Filtted_Result failed!\r\n");
	}
	clsDemodulatorAm.FilttedPos += CUDA_FILTER_BUFF_STEP_LENGTH;
	clsDemodulatorAm.FilttedPos &= DEMODULATOR_AM_FILTTED_BUFF_LENGTH_MASK;

	h_AM_Demodulator_Result = (FILTEDDATATYPE*)clsDemodulatorAm.AMBuff + clsDemodulatorAm.AMPos;
	err = cudaMemcpy(h_AM_Demodulator_Result, d_AM_Demodulator_Result, cuda_AM_demodulator_result_size, cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		printf("cudaMemcpy h_AM_Demodulator_Result failed!\r\n");
	}
	clsDemodulatorAm.AMPos += cuda_AM_demodulator_result_size / sizeof(FILTEDDATATYPE);
	clsDemodulatorAm.AMPos &= DEMODULATOR_AM_BUFF_LENGTH_MASK;

	stage++;
	stage &= 0x3;
}