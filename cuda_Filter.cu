
#include "stdafx.h"

#include <stdio.h>
#include <random>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "CWaveData.h"
#include "CWaveFilter.h"

#include "cuda_Filter.cuh"


FILTERCOREDATATYPE* cuda_Filter_d_Filter_Core = NULL;
ADCDATATYPE* cuda_Filter_d_SrcData = NULL;
FILTEDDATATYPE* cuda_Filter_d_Result = NULL;

//FILTERCOREDATATYPE* h_Filter_Core = NULL;
ADCDATATYPE* cuda_Filter_h_SrcData = NULL;
FILTEDDATATYPE* cuda_Filter_h_Result = NULL;

size_t cuda_Filter_thread_size;
size_t cuda_Filter_result_size;
size_t cuda_Filter_src_step_size;

CWaveFilter::PFILTERINFO cuda_Filter_rootFilterInfo = NULL;

__global__ void
cuda_Filter(const ADCDATATYPE* src, const FILTERCOREDATATYPE* core, FILTEDDATATYPE* result, int stage, unsigned int corelen)
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

void Cuda_ReInitFilterCore(CWaveFilter::PFILTERINFO pFilterInfo)
{
	cudaError_t err = cudaSuccess;

	cuda_Filter_rootFilterInfo = pFilterInfo;

	if (cuda_Filter_d_Filter_Core != NULL) {
		err = cudaFree(cuda_Filter_d_Filter_Core);
		if (err != cudaSuccess) {
			printf("cudaFree cuda_Filter_d_Filter_Core failed!\r\n");
			//goto Error;
		}
	}
	//if(h_Filter_Core != NULL)free(h_Filter_Core);

	size_t filter_core_size = pFilterInfo->CoreLength * sizeof(FILTERCOREDATATYPE);
	// Allocate the host input vector Core
	//h_Filter_Core = (FILTERCOREDATATYPE*)malloc(filter_core_size);

	//memcpy(h_Filter_Core, clsWaveFilter.FilterCore, filter_core_size);

	// Allocate the device input vector Filter_Core
	err = cudaMalloc((void**)&cuda_Filter_d_Filter_Core, filter_core_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc cuda_Filter_d_Filter_Core failed!\r\n");
		//goto Error;
	}
	err = cudaMemcpy(cuda_Filter_d_Filter_Core, pFilterInfo->FilterCore, filter_core_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		printf("cudaMemcpy cuda_Filter_d_Filter_Core failed!\r\n");
		//goto Error;
	}

	printf("Cuda ReInit Filter Core Done.\r\n");

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

void Cuda_Filter_Init(void)
{
	
	cuda_getThreadNum();

	// Error code to check return values for CUDA calls
	cudaError_t err = cudaSuccess;

	// Print the vector length to be used, and compute its size
	int numElements = CUDA_FILTER_BUFF_SRC_LENGTH;
	size_t src_data_size = numElements * sizeof(ADCDATATYPE);
	cuda_Filter_src_step_size = CUDA_FILTER_BUFF_STEP_LENGTH * sizeof(ADCDATATYPE);
	printf("Cuda_Init [Vector addition of %d elements]\n", numElements);

	// Allocate the host input vector A
	//cuda_Filter_h_SrcData = (ADCDATATYPE*)malloc(src_data_size);
	
	cuda_Filter_thread_size = CUDA_FILTER_BUFF_STEP_LENGTH;
	cuda_Filter_result_size = cuda_Filter_thread_size * sizeof(FILTEDDATATYPE);
	// Allocate the host output vector Result
	//float* cuda_Filter_h_Result = (float*)malloc(cuda_Filter_result_size);

	// Allocate the device input vector A
	err = cudaMalloc((void**)&cuda_Filter_d_SrcData, src_data_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc cuda_Filter_d_SrcData failed!\r\n");
		//goto Error;
	}
	// Allocate the device output vector C
	err = cudaMalloc((void**)&cuda_Filter_d_Result, cuda_Filter_result_size);
	if (err != cudaSuccess) {
		printf("cudaMalloc cuda_Filter_d_Result failed!\r\n");
		//goto Error;
	}
	
	printf("Cuda Init Done.\r\n");
}

void Cuda_Filter_UnInit(void)
{

	while (clsWaveFilter.cud_Filter_exit == false);

	cudaError_t err = cudaSuccess;
	
	// Free device global memory
	err = cudaFree(cuda_Filter_d_SrcData);
	if (err != cudaSuccess) {
		printf("cudaFree cuda_Filter_d_SrcData failed!\r\n");
		//goto Error;
	}
	err = cudaFree(cuda_Filter_d_Filter_Core);
	if (err != cudaSuccess) {
		printf("cudaFree cuda_Filter_d_Filter_Core failed!\r\n");
		//goto Error;
	}
	err = cudaFree(cuda_Filter_d_Result);
	if (err != cudaSuccess) {
		printf("cudaFree cuda_Filter_d_Result failed!\r\n");
		//goto Error;
	}

	// Free host memory
	//free(h_Filter_Core);

	printf("Cuda_Filter_Closed.\r\n");
}

void cuda_Filtting(void)
{
	cudaError_t err = cudaSuccess;
	static unsigned int stage = 0;

	size_t threadsPerBlock = cuda_Filter_thread_size;
	//int blocksPerGrid = (numElements + threadsPerBlock - 1) / threadsPerBlock;
	size_t blocksPerGrid = 1;

	cuda_Filter_h_SrcData = (ADCDATATYPE*)clsWaveData.AdcBuff + clsWaveData.FilttedPos;
	//printf("stage:%d\r\n", stage);
	err = cudaMemcpy((char*)cuda_Filter_d_SrcData + stage * cuda_Filter_src_step_size, cuda_Filter_h_SrcData, cuda_Filter_src_step_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		printf("cudaMemcpy cuda_Filter_d_SrcData failed!\r\n");
	}

	WaitForSingleObject(clsWaveFilter.hCoreMutex, INFINITE);

	//printf("CUDA kernel launch with %d blocks of %d threads\n", blocksPerGrid, threadsPerBlock);
	cuda_Filter << <threadsPerBlock, blocksPerGrid >> > (cuda_Filter_d_SrcData, cuda_Filter_d_Filter_Core, cuda_Filter_d_Result, stage,
		cuda_Filter_rootFilterInfo->CoreLength);
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

	cuda_Filter_h_Result = (FILTEDDATATYPE*)clsWaveData.FilttedBuff + clsWaveData.FilttedPos;
	err = cudaMemcpy(cuda_Filter_h_Result, cuda_Filter_d_Result, cuda_Filter_result_size, cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		printf("cudaMemcpy cuda_Filter_h_Result failed!\r\n");
	}
	clsWaveData.FilttedPos += CUDA_FILTER_BUFF_STEP_LENGTH;
	clsWaveData.FilttedPos &= DATA_BUFFER_MASK;

	stage++;
	stage &= 0x3;
	//printf("Cuda Filtting Done.\r\n");
}