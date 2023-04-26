
#include "stdafx.h"

#include <stdio.h>
#include <random>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "myDebug.h"
#include "CData.h"
#include "CWaveFilter.h"

#include "cuda_CFilter.cuh"

cuda_CFilter clscudaFilter;

__global__ void
cuda_Filter(const ADCDATATYPE* src, const FILTER_CORE_DATA_TYPE* core, FILTEDDATATYPE* result, int stage, unsigned int corelen)
{
//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	unsigned int index = blockIdx.x;
	unsigned int step = stage * CUDA_FILTER_BUFF_STEP_LENGTH;
	unsigned int istep = step + index - corelen;
	result[index] = 0;
	for (int n = 0; n < corelen; n++) {
		result[index] += src[(istep + n) & CUDA_FILTER_BUFF_SRC_LENGTH_MASK] * core[n];
	}
}

void cuda_CFilter::getThreadNum(void)
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

void cuda_CFilter::Init(CWaveFilter::PFILTERINFO pFilterInfo)
{
	rootFilterInfo = pFilterInfo;

	UnInit();

	getThreadNum();

	cudaError_t err = cudaSuccess;

	int numElements = CUDA_FILTER_BUFF_SRC_LENGTH;
	size_t src_data_size = numElements * sizeof(ADCDATATYPE);
	src_step_size = CUDA_FILTER_BUFF_STEP_LENGTH * sizeof(ADCDATATYPE);
	printf("Cuda_Init [Vector addition of %d elements]\n", numElements);

	thread_size = CUDA_FILTER_BUFF_STEP_LENGTH;
	result_size = thread_size * sizeof(FILTEDDATATYPE);

	err = cudaMalloc((void**)&d_SrcData, src_data_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_SrcData failed!\r\n");
	}
	err = cudaMalloc((void**)&d_Result, result_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_Result failed!\r\n");
	}
	
	if (d_Filter_Core != NULL) {
		err = cudaFree(d_Filter_Core);
		if (err != cudaSuccess) {
			printf("cudaFree d_Filter_Core failed!\r\n");
		}
	}
	size_t filter_core_size = pFilterInfo->CoreLength * sizeof(FILTER_CORE_DATA_TYPE);
	err = cudaMalloc((void**)&d_Filter_Core, filter_core_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc d_Filter_Core failed!\r\n");
	}
	err = cudaMemcpy(d_Filter_Core, pFilterInfo->FilterCore, filter_core_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		printf("cudaMemcpy d_Filter_Core failed!\r\n");
	}

	printf("Cuda Init Done.\r\n");
}

void cuda_CFilter::UnInit(void)
{

	cudaError_t err = cudaSuccess;
	
	// Free device global memory
	if (d_SrcData != NULL) {
		err = cudaFree(d_SrcData);
		if (err != cudaSuccess) {
			printf("cudaFree d_SrcData failed!\r\n");
		}
		d_SrcData = NULL;
	}
	if (d_Filter_Core != NULL) {
		err = cudaFree(d_Filter_Core);
		if (err != cudaSuccess) {
			printf("cudaFree d_Filter_Core failed!\r\n");
		}
		d_Filter_Core = NULL;
	}
	if (d_Result != NULL) {
		err = cudaFree(d_Result);
		if (err != cudaSuccess) {
			printf("cudaFree d_Result failed!\r\n");
		}
		d_Result = NULL;
	}
	// Free host memory
	//free(h_Filter_Core);

	DbgMsg("cuda_CFilter UnInit Closed.\r\n");
}

void cuda_CFilter::Filtting(void)
{
	WaitForSingleObject(clsWaveFilter.hCoreMutex, INFINITE);

	cudaError_t err = cudaSuccess;
	static unsigned int stage = 0;

	size_t threadsPerBlock = thread_size;
	//int blocksPerGrid = (numElements + threadsPerBlock - 1) / threadsPerBlock;
	size_t blocksPerGrid = 1;

	h_SrcData = (ADCDATATYPE*)clsData.AdcBuff + clsData.FilttedPos;
	err = cudaMemcpy((char*)d_SrcData + stage * src_step_size, h_SrcData, src_step_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		printf("cudaMemcpy d_SrcData failed!\r\n");
	}

	cuda_Filter << <threadsPerBlock, blocksPerGrid >> > (d_SrcData, d_Filter_Core, d_Result, stage,
		rootFilterInfo->CoreLength);
	err = cudaGetLastError();
	if (err != cudaSuccess) {
		printf("cudaFilter launch failed: %s\r\n", cudaGetErrorString(err));
	}
	err = cudaDeviceSynchronize();
	if (err != cudaSuccess) {
		printf("cudaDeviceSynchronize returned error code %d after launching cudaFilter!\r\n", err);
	}
	
	h_Result = (FILTEDDATATYPE*)clsData.FilttedBuff + clsData.FilttedBuffPos;
	err = cudaMemcpy(h_Result, d_Result, result_size, cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		printf("cudaMemcpy h_Result failed!\r\n");
	}
	clsData.FilttedPos += CUDA_FILTER_BUFF_STEP_LENGTH;
	clsData.FilttedPos &= DATA_BUFFER_MASK;
	clsData.FilttedBuffPos = clsData.FilttedPos;

	stage++;
	stage &= 0x3;
	//printf("Cuda Filtting Done.\r\n");

	ReleaseMutex(clsWaveFilter.hCoreMutex);
}