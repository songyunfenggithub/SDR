
#include "stdafx.h"

#include <stdio.h>
#include <random>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "Debug.h"
#include "CData.h"
#include "CFilter.h"
#include "CAM.h"

#include "cuda_CFilter_AM.cuh"

using namespace METHOD;

cuda_CFilter_AM clscudaMainFilter_AM;

__global__ void
cuda_core_Filter_AM(
	const ADC_DATA_TYPE* src, int stage, unsigned int srclen,
	FILTTED_DATA_TYPE* decimation_cache1, const CFilter::FILTER_CORE_DATA_TYPE* core1, unsigned int decimation_factor_bit1, unsigned int corelen1,
	FILTTED_DATA_TYPE* decimation_cache2, const CFilter::FILTER_CORE_DATA_TYPE* core2, unsigned int decimation_factor_bit2, unsigned int corelen2,
	CFilter::FILTER_CORE_DATA_TYPE* filtted_result1,
	CFilter::FILTER_CORE_DATA_TYPE* filtted_result2,
	float scale
)
{
	//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	unsigned int cache1_length_mask = (srclen >> decimation_factor_bit1) - 1;
	unsigned int cache1_stage_length = (srclen>> 2) >> decimation_factor_bit1;
	unsigned int cache1_stage_step = stage * cache1_stage_length;
	unsigned int src_stage_step = stage * (srclen >> 2);
	unsigned int index = blockIdx.x;
	decimation_cache1[cache1_stage_step + index] = src[src_stage_step + (index << decimation_factor_bit1)];
	__syncthreads();
	unsigned int index_mask = (1 << decimation_factor_bit2) - 1;
	unsigned int i_stage_step = cache1_stage_step + index - corelen1;
	float d = 0;
	if (!(index & index_mask)) 
	{
		for (unsigned int i = 0; i < corelen1; i++) {
			d += decimation_cache1[(i_stage_step + i) & cache1_length_mask] * core1[i];
		}
		filtted_result1[cache1_stage_step + index] = d;
	}
	__syncthreads();
	unsigned int cache2_length_mask = (srclen >> (decimation_factor_bit1 + decimation_factor_bit2)) - 1;
	unsigned int cache2_stage_length = (srclen >> 2) >> (decimation_factor_bit1 + decimation_factor_bit2);
	unsigned int cache2_stage_step = stage * cache2_stage_length;
	unsigned int index2 = index >> decimation_factor_bit2;
	if(!(index & index_mask))
		decimation_cache2[cache2_stage_step + index2] = filtted_result1[cache1_stage_step + index];
	__syncthreads();
	i_stage_step = cache2_stage_step + index2 - corelen2;
	d = 0;
	for (unsigned int i = 0; i < corelen2; i++) {
		d += decimation_cache2[(i_stage_step + i) & cache2_length_mask] * core2[i];
	}
	filtted_result2[cache2_stage_step + index2] = d * scale;
}

__global__ void
cuda_core_Filter_AM_Demodulator(CFilter::FILTER_CORE_DATA_TYPE* filtted_result, FILTTED_DATA_TYPE* demodulator_result,
	int stage, int src_stage_length, unsigned int decimation_factor_bit, unsigned int cache_length)
{
	//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	unsigned int cache_stage_length = src_stage_length >> decimation_factor_bit;
	unsigned int cache_stage_step = stage * cache_stage_length;

	unsigned int index = blockIdx.x;

	unsigned int n = 1 << 2;
	unsigned int mask = n - 1;
	unsigned int demodulator_result_index;
	float d;
	if ((index & mask) == 0) {
		demodulator_result_index = index >> 2;
		demodulator_result[demodulator_result_index] = 0;
		for (int i = 0; i < n; i++) {
			if ((d = filtted_result[cache_stage_step + index + i]) > 0) demodulator_result[demodulator_result_index] += d;
		}
	}
}

cuda_CFilter_AM::cuda_CFilter_AM()
{

}

cuda_CFilter_AM::~cuda_CFilter_AM()
{

}

void cuda_CFilter_AM::Init(CFilter* f)
{
	cudaError_t err = cudaSuccess;

	cFilter = f;
	rootFilterInfo1 = &f->rootFilterInfo1;
	rootFilterInfo2 = &f->rootFilterInfo2;
	SrcData = f->SrcData;
	TargetData = f->TargetData;
	SrcLen = f->FilterSrcLen;

	DbgMsg("Cuda_Init [Vector addition of %d CUDA_FILTER_BUFF_SRC_LENGTH]\n", SrcLen);

	UnInit();

	size_t src_data_size = SrcLen * sizeof(ADC_DATA_TYPE);
	err = cudaMalloc((void**)&d_SrcData, src_data_size);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_SrcData failed!\r\n");
		//goto Error;
	}

	size_t filter_core_size1 = rootFilterInfo1->CoreLength * sizeof(CFilter::FILTER_CORE_DATA_TYPE);
	err = cudaMalloc((void**)&d_Filter_Core1, filter_core_size1);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Filter_Core failed!\r\n");
		//goto Error;
	}
	err = cudaMemcpy(d_Filter_Core1, rootFilterInfo1->FilterCore, filter_core_size1, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		DbgMsg("cudaMemcpy d_Filter_Core failed!\r\n");
		//goto Error;
	}
	size_t filter_core_size2 = rootFilterInfo2->CoreLength * sizeof(CFilter::FILTER_CORE_DATA_TYPE);
	err = cudaMalloc((void**)&d_Filter_Core2, filter_core_size2);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Filter_Core failed!\r\n");
		//goto Error;
	}
	err = cudaMemcpy(d_Filter_Core2, rootFilterInfo2->FilterCore, filter_core_size2, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		DbgMsg("cudaMemcpy d_Filter_Core failed!\r\n");
		//goto Error;
	}

	size_t decimation_cache_size1 = (SrcLen >> rootFilterInfo1->decimationFactorBit) * sizeof(FILTTED_DATA_TYPE);
	err = cudaMalloc((void**)&d_Decimation_Cache1, decimation_cache_size1);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Demodulator_Result failed!\r\n");
		//goto Error;
	}
	size_t decimation_cache_size2 = (SrcLen >> (rootFilterInfo1->decimationFactorBit + rootFilterInfo2->decimationFactorBit)) * sizeof(FILTTED_DATA_TYPE);
	err = cudaMalloc((void**)&d_Decimation_Cache2, decimation_cache_size2);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Demodulator_Result failed!\r\n");
		//goto Error;
	}

	size_t filter_result_size1 = (SrcLen >> rootFilterInfo1->decimationFactorBit) * sizeof(FILTTED_DATA_TYPE);
	err = cudaMalloc((void**)&d_Filtted_Result1, filter_result_size1);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Filtted_Result failed!\r\n");
		//goto Error;
	}
	size_t filter_result_size2 = (SrcLen >> (rootFilterInfo1->decimationFactorBit + rootFilterInfo2->decimationFactorBit)) * sizeof(FILTTED_DATA_TYPE);
	err = cudaMalloc((void**)&d_Filtted_Result2, filter_result_size2);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Filtted_Result failed!\r\n");
		//goto Error;
	}

	/*
	size_t demodulator_result_size = ((srcLen >> 2) >> rootFilterInfo->decimationFactorBit) * sizeof(FILTTED_DATA_TYPE);
	err = cudaMalloc((void**)&d_Demodulator_Result, demodulator_result_size);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Demodulator_Result failed!\r\n");
		//goto Error;
	}
	*/
	DbgMsg("Cuda Filter3 Init.\r\n");
}

void cuda_CFilter_AM::UnInit(void)
{
	if (this == NULL)return;
	cudaError_t err = cudaSuccess;

	// Free device global memory
	if (d_SrcData != NULL) {
		err = cudaFree(d_SrcData);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_SrcData failed!\r\n");
			//goto Error;
		}
		d_SrcData = NULL;
	}
	if (d_Filter_Core1 != NULL) {
		err = cudaFree(d_Filter_Core1);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Filter_Core1 failed!\r\n");
			//goto Error;
		}
		d_Filter_Core1 = NULL;
	}
	if (d_Filter_Core2 != NULL) {
		err = cudaFree(d_Filter_Core2);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Filter_Core2 failed!\r\n");
			//goto Error;
		}
		d_Filter_Core2 = NULL;
	}
	if (d_Filtted_Result1 != NULL) {
		err = cudaFree(d_Filtted_Result1);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Filtted_Result1 failed!\r\n");
			//goto Error;
		}
		d_Filtted_Result1 = NULL;
	}
	if (d_Filtted_Result2 != NULL) {
		err = cudaFree(d_Filtted_Result2);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Filtted_Result failed!\r\n");
			//goto Error;
		}
		d_Filtted_Result2 = NULL;
	}

	if (d_Decimation_Cache1 != NULL) {
		err = cudaFree(d_Decimation_Cache1);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Decimation_Cache1 failed!\r\n");
			//goto Error;
		}
		d_Decimation_Cache1 = NULL;
	}
	if (d_Decimation_Cache2 != NULL) {
		err = cudaFree(d_Decimation_Cache2);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Decimation_Cache2 failed!\r\n");
			//goto Error;
		}
		d_Decimation_Cache2 = NULL;
	}
	if (d_Demodulator_Result != NULL) {
		err = cudaFree(d_Demodulator_Result);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Demodulator_Result failed!\r\n");
			//goto Error;
		}
		d_Demodulator_Result = NULL;
	}
	DbgMsg("cuda_CFilter_AM UnInit.\r\n");
}

void cuda_CFilter_AM::Filtting(void)
{
	WaitForSingleObject(cFilter->hCoreMutex, INFINITE);

	if (cFilter->Cuda_Filter_N_New != cFilter->Cuda_Filter_N_Doing) {
		ReleaseMutex(cFilter->hCoreMutex);
		Sleep(100);
		return;
	}

	cudaError_t err = cudaSuccess;
	static unsigned int stage = 0;

	size_t threadsPerBlock = SrcLen >> (2 + rootFilterInfo1->decimationFactorBit);
	//int blocksPerGrid = (numElements + threadsPerBlock - 1) / threadsPerBlock;
	size_t blocksPerGrid = 1;

	h_SrcData = (ADC_DATA_TYPE*)SrcData->Buff + SrcData->ProcessPos;
	//DbgMsg("stage:%d\r\n", stage);
	size_t stage_size = (SrcLen >> 2) * sizeof(ADC_DATA_TYPE);
	err = cudaMemcpy((char*)d_SrcData + stage * stage_size, h_SrcData, stage_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		DbgMsg("3cudaMemcpy d_SrcData failed!\r\n");
	}

	//DbgMsg("CUDA kernel launch with %d blocks of %d threads\n", blocksPerGrid, threadsPerBlock);

		//const ADC_DATA_TYPE* src, int stage, unsigned int srclen,
		//FILTTED_DATA_TYPE* decimation_cache1, const CFilter::FILTER_CORE_DATA_TYPE* core1, unsigned int decimation_factor_bit1, unsigned int corelen1,
		//FILTTED_DATA_TYPE* decimation_cache2, const CFilter::FILTER_CORE_DATA_TYPE* core2, unsigned int decimation_factor_bit2, unsigned int corelen2,
		//CFilter::FILTER_CORE_DATA_TYPE* filtted1_result,
		//CFilter::FILTER_CORE_DATA_TYPE* filtted2_result

	cuda_core_Filter_AM << <threadsPerBlock, blocksPerGrid >> > (
		d_SrcData, stage, SrcLen,
		d_Decimation_Cache1, d_Filter_Core1, rootFilterInfo1->decimationFactorBit, rootFilterInfo1->CoreLength,
		d_Decimation_Cache2, d_Filter_Core2, rootFilterInfo2->decimationFactorBit, rootFilterInfo2->CoreLength,
		d_Filtted_Result1,
		d_Filtted_Result2,
		*cFilter->Scale
		);
	err = cudaGetLastError();
	if (err != cudaSuccess) {
		DbgMsg("3cudaFilter launch failed: %s\r\n", cudaGetErrorString(err));
	}
	// any errors encountered during the launch.
	err = cudaDeviceSynchronize();
	if (err != cudaSuccess) {
		DbgMsg("3cudaDeviceSynchronize returned error code %d after launching cudaFilter!\r\n", err);
	}

	CData* target_Data = (CData*)TargetData;
	h_Filtted_Result2 = (FILTTED_DATA_TYPE*)target_Data->Buff + target_Data->Pos;
	size_t result_size = (SrcLen >> 2) >> (rootFilterInfo1->decimationFactorBit + rootFilterInfo2->decimationFactorBit);
	err = cudaMemcpy(
		h_Filtted_Result2, 
		d_Filtted_Result2 + stage * result_size, 
		result_size * sizeof(FILTTED_DATA_TYPE),
		cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		DbgMsg("3cudaMemcpy h_Filtted_Result failed!\r\n");
	}
	UINT T;
	T = SrcData->ProcessPos;
	SrcData->ProcessPos = (T + (SrcLen >> 2)) & SrcData->Mask;
	T = target_Data->Pos;
	target_Data->Pos = (T + result_size) & target_Data->Mask;

	stage++;
	stage &= 0x3;

	ReleaseMutex(cFilter->hCoreMutex);
}