
#include "stdafx.h"

#include <stdio.h>
#include <random>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "public.h"
#include "myDebug.h"
#include "CData.h"
#include "CFilter.h"

#include "cuda_CFilter.cuh"

using namespace METHOD;

cuda_CFilter clscudaMainFilter;
cuda_CFilter clscudaAudioFilter;

__global__ void
cuda_Filter(const ADC_DATA_TYPE* src, const FILTER_CORE_DATA_TYPE* core, FILTTED_DATA_TYPE* result, int stage, unsigned int corelen, unsigned int srclen)
{
	//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	unsigned int index = blockIdx.x;
	unsigned int step = stage * (srclen >> 2);
	unsigned int istep = step + index - corelen;
	unsigned int srcmask = srclen - 1;
	result[index] = 0;
	for (int n = 0; n < corelen; n++) {
		result[index] += src[(istep + n) & srcmask] * core[n];
	}
}

__global__ void
cuda_Filter_short(const short* src, const FILTER_CORE_DATA_TYPE* core, FILTTED_DATA_TYPE* result, int stage, unsigned int corelen, unsigned int srclen)
{
//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	unsigned int index = blockIdx.x;
	unsigned int step = stage * (srclen >> 2);
	unsigned int istep = step + index - corelen;
	unsigned int srcmask = srclen - 1;
	result[index] = 0;
	for (int n = 0; n < corelen; n++) {
		result[index] += src[(istep + n) & srcmask] * core[n];
	}
}

__global__ void
cuda_Filter_float(const float* src, const FILTER_CORE_DATA_TYPE* core, FILTTED_DATA_TYPE* result, int stage, unsigned int corelen, unsigned int srclen)
{
	//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	unsigned int index = blockIdx.x;
	unsigned int step = stage * (srclen >> 2);
	unsigned int istep = step + index - corelen;
	unsigned int srcmask = srclen - 1;
	result[index] = 0;
	for (int n = 0; n < corelen; n++) {
		result[index] += src[(istep + n) & srcmask] * core[n];
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

void cuda_CFilter::Init(CFilter::PFILTER_INFO pFilterInfo, CData* srcData, CData* targetData, UINT srcLen)
{
	rootFilterInfo = pFilterInfo;
	SrcData = srcData;
	TargetData = targetData;
	SrcLen = srcLen;

	UnInit();

	getThreadNum();

	cudaError_t err = cudaSuccess;

	int numElements = srcLen;
	size_t src_data_size = numElements * srcData->SizeOfType;
	src_step_size = (srcLen >> 2) * srcData->SizeOfType;
	printf("Cuda_Init [Vector addition of %d elements]\n", numElements);

	thread_size = (srcLen >> 2);
	result_size = thread_size * targetData->SizeOfType;

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
	WaitForSingleObject(clsMainFilter.hCoreMutex, INFINITE);

	CData* src_Data = (CData*)SrcData;
	cudaError_t err = cudaSuccess;
	static unsigned int stage = 0;

	size_t threadsPerBlock = thread_size;
	//int blocksPerGrid = (numElements + threadsPerBlock - 1) / threadsPerBlock;
	size_t blocksPerGrid = 1;
	
	h_SrcData = (char*)src_Data->Buff + src_Data->SizeOfType * src_Data->ProcessPos;
	err = cudaMemcpy((char*)d_SrcData + stage * src_step_size, h_SrcData, src_step_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		printf("cudaMemcpy d_SrcData failed!\r\n");
	}
	
	switch (src_Data->DataType) {
	case short_type:
	{
		cuda_Filter_short << <threadsPerBlock, blocksPerGrid >> > ((short*)d_SrcData, d_Filter_Core, d_Result, stage,
			rootFilterInfo->CoreLength, SrcLen);
	}
	break;
	case float_type:
	{
		cuda_Filter_float << <threadsPerBlock, blocksPerGrid >> > ((float*)d_SrcData, d_Filter_Core, d_Result, stage,
			rootFilterInfo->CoreLength, SrcLen);
	}
	break;
	}
	err = cudaGetLastError();
	if (err != cudaSuccess) {
		printf("cudaFilter launch failed: %s\r\n", cudaGetErrorString(err));
	}
	err = cudaDeviceSynchronize();
	if (err != cudaSuccess) {
		printf("cudaDeviceSynchronize returned error code %d after launching cudaFilter!\r\n", err);
	}
	
	CData* target_Data = (CData*)TargetData;
	h_Result = (FILTTED_DATA_TYPE*)target_Data->Buff + target_Data->Pos;
	err = cudaMemcpy(h_Result, d_Result, result_size, cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		printf("cudaMemcpy h_Result failed!\r\n");
	}
	src_Data->ProcessPos += (SrcLen >> 2);
	src_Data->ProcessPos &= src_Data->Mask;
	target_Data->Pos = src_Data->ProcessPos;

	stage++;
	stage &= 0x3;
	//printf("Cuda Filtting Done.\r\n");

	ReleaseMutex(clsMainFilter.hCoreMutex);
}