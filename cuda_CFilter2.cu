
#include "stdafx.h"

#include <stdio.h>
#include <random>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "myDebug.h"
#include "CData.h"
#include "CWaveFilter.h"
#include "CDemodulatorAM.h"

#include "cuda_CFilter2.cuh"

#define DEMODULATOR_FILTER_SAMPLERATE_OFFSET_BIT	0x3

cuda_CFilter2 clscudaFilter2;

__global__ void
cuda_core_Filter2(const ADCDATATYPE* src, FILTEDDATATYPE* decimation_cache, const FILTER_CORE_DATA_TYPE* core, FILTER_CORE_DATA_TYPE* filtted_result,
	int stage, unsigned int decimation_factor_bit, unsigned int corelen)
{
	//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	unsigned int cache_length_mask = (CUDA_FILTER_BUFF_SRC_LENGTH >> decimation_factor_bit) - 1;
	unsigned int cache_stage_length = CUDA_FILTER_BUFF_STEP_LENGTH >> decimation_factor_bit;
	unsigned int cache_stage_step = stage * cache_stage_length;
	unsigned int src_stage_step = stage * CUDA_FILTER_BUFF_STEP_LENGTH;
	unsigned int index = blockIdx.x;
	
	decimation_cache[cache_stage_step + index] = src[src_stage_step + (index << decimation_factor_bit)];

	//对线程块中的线程进行同步
	__syncthreads();

	unsigned int i_stage_step = cache_stage_step + index - corelen;
	filtted_result[cache_stage_step + index] = 0;
	for (unsigned int i = 0; i < corelen; i++) {
		filtted_result[cache_stage_step + index] += decimation_cache[(i_stage_step + i) & cache_length_mask] * core[i];
	}
}

__global__ void
cuda_core_Filter2_Demodulator(FILTER_CORE_DATA_TYPE* filtted_result, FILTEDDATATYPE* demodulator_result,
	int stage, int src_stage_length, unsigned int decimation_factor_bit, unsigned int cache_length)
{
	//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	unsigned int cache_stage_length = src_stage_length >> decimation_factor_bit;
	unsigned int cache_stage_step = stage * cache_stage_length;

	unsigned int index = blockIdx.x;

	unsigned int n = 1 << DEMODULATOR_FILTER_SAMPLERATE_OFFSET_BIT;
	unsigned int mask = n - 1;
	unsigned int demodulator_result_index;
	float d;
	if ((index & mask) == 0) {
		demodulator_result_index = index >> DEMODULATOR_FILTER_SAMPLERATE_OFFSET_BIT;
		demodulator_result[demodulator_result_index] = 0;
		for (int i = 0; i < n; i++) {
			if ((d = filtted_result[cache_stage_step + index + i]) > 0) demodulator_result[demodulator_result_index] += d;
		}
	}
}

cuda_CFilter2::cuda_CFilter2()
{

}

cuda_CFilter2::~cuda_CFilter2()
{

}

void cuda_getThreadNum(void)
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

void cuda_CFilter2::Init(CWaveFilter::PFILTERINFO pFilterInfo)
{
	UnInit();

	cuda_getThreadNum();

	cudaError_t err = cudaSuccess;

	rootFilterInfo = pFilterInfo;

	printf("Cuda_Init [Vector addition of %d CUDA_FILTER_BUFF_SRC_LENGTH]\n", CUDA_FILTER_BUFF_SRC_LENGTH);

	if (d_SrcData != NULL) {
		err = cudaFree(d_SrcData);
		if (err != cudaSuccess) {
			printf("cudaFree d_SrcData failed!\r\n");
			//goto Error;
		}
	}
	size_t src_data_size = CUDA_FILTER_BUFF_SRC_LENGTH * sizeof(ADCDATATYPE);
	err = cudaMalloc((void**)&d_SrcData, src_data_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_SrcData failed!\r\n");
		//goto Error;
	}

	if (d_Filter_Core != NULL) {
		err = cudaFree(d_Filter_Core);
		if (err != cudaSuccess) {
			printf("cudaFree d_Filter_Core failed!\r\n");
			//goto Error;
		}
	}
	size_t filter_core_size = pFilterInfo->CoreLength * sizeof(FILTER_CORE_DATA_TYPE);
	err = cudaMalloc((void**)&d_Filter_Core, filter_core_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_Filter_Core failed!\r\n");
		//goto Error;
	}
	err = cudaMemcpy(d_Filter_Core, pFilterInfo->FilterCore, filter_core_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		printf("cudaMemcpy d_Filter_Core failed!\r\n");
		//goto Error;
	}

	if (d_Decimation_Cache != NULL) {
		err = cudaFree(d_Decimation_Cache);
		if (err != cudaSuccess) {
			printf("cudaFree d_Decimation_Cache failed!\r\n");
			//goto Error;
		}
	}

	size_t decimation_cache_size = (CUDA_FILTER_BUFF_SRC_LENGTH >> rootFilterInfo->decimationFactorBit) * sizeof(FILTEDDATATYPE);
	err = cudaMalloc((void**)&d_Decimation_Cache, decimation_cache_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_Demodulator_Result failed!\r\n");
		//goto Error;
	}

	if (d_Filtted_Result != NULL) {
		err = cudaFree(d_Filtted_Result);
		if (err != cudaSuccess) {
			printf("cudaFree d_Filtted_Result failed!\r\n");
			//goto Error;
		}
	}
	size_t filter_result_size = (CUDA_FILTER_BUFF_SRC_LENGTH >> rootFilterInfo->decimationFactorBit) * sizeof(FILTEDDATATYPE);
	err = cudaMalloc((void**)&d_Filtted_Result, filter_result_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_Filtted_Result failed!\r\n");
		//goto Error;
	}

	if (d_Demodulator_Result != NULL) {
		err = cudaFree(d_Demodulator_Result);
		if (err != cudaSuccess) {
			printf("cudaFree d_Demodulator_Result failed!\r\n");
			//goto Error;
		}
	}
	size_t demodulator_result_size = (CUDA_FILTER_BUFF_STEP_LENGTH >> rootFilterInfo->decimationFactorBit) * sizeof(FILTEDDATATYPE);
	err = cudaMalloc((void**)&d_Demodulator_Result, demodulator_result_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_Demodulator_Result failed!\r\n");
		//goto Error;
	}

	printf("Cuda Filter2 Init Done.\r\n");
}

void cuda_CFilter2::UnInit(void)
{
	cudaError_t err = cudaSuccess;

	// Free device global memory
	if (d_SrcData != NULL) {
		err = cudaFree(d_SrcData);
		if (err != cudaSuccess) {
			printf("cudaFree d_SrcData failed!\r\n");
			//goto Error;
		}
		d_SrcData = NULL;
	}
	if (d_Filter_Core != NULL) {
		err = cudaFree(d_Filter_Core);
		if (err != cudaSuccess) {
			printf("cudaFree d_Filter_Core failed!\r\n");
			//goto Error;
		}
		d_Filter_Core = NULL;
	}
	if (d_Filtted_Result != NULL) {
		err = cudaFree(d_Filtted_Result);
		if (err != cudaSuccess) {
			printf("cudaFree d_Filtted_Result failed!\r\n");
			//goto Error;
		}
		d_Filtted_Result = NULL;
	}

	if (d_Decimation_Cache != NULL) {
		err = cudaFree(d_Decimation_Cache);
		if (err != cudaSuccess) {
			printf("cudaFree d_Decimation_Cache failed!\r\n");
			//goto Error;
		}
		d_Decimation_Cache = NULL;
	}
	if (d_Demodulator_Result != NULL) {
		err = cudaFree(d_Demodulator_Result);
		if (err != cudaSuccess) {
			printf("cudaFree d_Demodulator_Result failed!\r\n");
			//goto Error;
		}
		d_Demodulator_Result = NULL;
	}
	// Free host memory
	//free(h_Filter_Core);

	printf("cuda_CFilter2 UnInit Closed.\r\n");
}

void cuda_CFilter2::Filtting(void)
{
	WaitForSingleObject(clsWaveFilter.hCoreMutex, INFINITE);

	cudaError_t err = cudaSuccess;
	static unsigned int stage = 0;

	size_t threadsPerBlock = CUDA_FILTER_BUFF_STEP_LENGTH >> rootFilterInfo->decimationFactorBit;
	//int blocksPerGrid = (numElements + threadsPerBlock - 1) / threadsPerBlock;
	size_t blocksPerGrid = 1;

	h_SrcData = (ADCDATATYPE*)clsData.AdcBuff + clsData.FilttedPos;
	//printf("stage:%d\r\n", stage);
	static size_t stage_size = CUDA_FILTER_BUFF_STEP_LENGTH * sizeof(ADCDATATYPE);
	err = cudaMemcpy((char*)d_SrcData + stage * stage_size, h_SrcData, stage_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		printf("cudaMemcpy d_SrcData failed!\r\n");
	}

	//printf("CUDA kernel launch with %d blocks of %d threads\n", blocksPerGrid, threadsPerBlock);
	cuda_core_Filter2 << <threadsPerBlock, blocksPerGrid >> > (d_SrcData, d_Decimation_Cache, d_Filter_Core, d_Filtted_Result,
		stage, rootFilterInfo->decimationFactorBit, rootFilterInfo->CoreLength);
	err = cudaGetLastError();
	if (err != cudaSuccess) {
		printf("cudaFilter launch failed: %s\r\n", cudaGetErrorString(err));
	}
	// any errors encountered during the launch.
	err = cudaDeviceSynchronize();
	if (err != cudaSuccess) {
		printf("cudaDeviceSynchronize returned error code %d after launching cudaFilter!\r\n", err);
	}

	h_Filtted_Result = (FILTEDDATATYPE*)clsData.FilttedBuff + clsData.FilttedBuffPos;
	err = cudaMemcpy(h_Filtted_Result, 
		d_Filtted_Result + stage * (CUDA_FILTER_BUFF_STEP_LENGTH >> rootFilterInfo->decimationFactorBit), 
		(CUDA_FILTER_BUFF_STEP_LENGTH >> rootFilterInfo->decimationFactorBit) * sizeof(FILTEDDATATYPE), 
		cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		printf("cudaMemcpy h_Filtted_Result failed!\r\n");
	}
	UINT T;
	T = clsData.FilttedPos;
	clsData.FilttedPos = (T + CUDA_FILTER_BUFF_STEP_LENGTH) & DATA_BUFFER_MASK;
	T = clsData.FilttedBuffPos;
	clsData.FilttedBuffPos = (T + (CUDA_FILTER_BUFF_STEP_LENGTH >> rootFilterInfo->decimationFactorBit)) & DATA_BUFFER_MASK;

	/*
	h_Demodulator_Result = (FILTEDDATATYPE*)clsDemodulatorAm.AMBuff + clsDemodulatorAm.AMPos;
	err = cudaMemcpy(h_Demodulator_Result, d_Demodulator_Result, cuda_demodulator_result_size, cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		printf("cudaMemcpy h_Demodulator_Result failed!\r\n");
	}
	clsDemodulatorAm.AMPos += cuda_demodulator_result_size / sizeof(FILTEDDATATYPE);
	clsDemodulatorAm.AMPos &= DEMODULATOR_BUFF_LENGTH_MASK;
	*/

	stage++;
	stage &= 0x3;

	ReleaseMutex(clsWaveFilter.hCoreMutex);
}