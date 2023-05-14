
#include "stdafx.h"

#include <stdio.h>
#include <random>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "Debug.h"
#include "CData.h"
#include "CFilter.h"
#include "CDemodulatorAM.h"

#include "cuda_CFilter2.cuh"

using namespace METHOD;

cuda_CFilter2 clscudaMainFilter2;
cuda_CFilter2 clscudaMainFilter2Q;
cuda_CFilter2 clscudaAudioFilter2;

__global__ void
cuda_core_Filter2(
	const ADC_DATA_TYPE* src, unsigned int srclen,
	int stage,
	FILTTED_DATA_TYPE* decimation_cache, unsigned int decimation_factor_bit,
	const CFilter::FILTER_CORE_DATA_TYPE* core, unsigned int corelen,
	CFilter::FILTER_CORE_DATA_TYPE* filtted_result,
	float scale
)
{
	//	int i = blockDim.x * blockIdx.x + threadIdx.x;
	unsigned int cache_length_mask = (srclen >> decimation_factor_bit) - 1;
	unsigned int cache_stage_length = (srclen>> 2) >> decimation_factor_bit;
	unsigned int cache_stage_step = stage * cache_stage_length;
	unsigned int src_stage_step = stage * (srclen >> 2);
	unsigned int index = blockIdx.x;
	decimation_cache[cache_stage_step + index] = src[src_stage_step + (index << decimation_factor_bit)];
	__syncthreads();
	unsigned int i_stage_step = cache_stage_step + index - corelen;
	float d = 0;
	for (unsigned int i = 0; i < corelen; i++) {
		d += decimation_cache[(i_stage_step + i) & cache_length_mask] * core[i];
	}
	filtted_result[cache_stage_step + index] = d * scale;
}

__global__ void
cuda_core_Filter2_Demodulator(CFilter::FILTER_CORE_DATA_TYPE* filtted_result, FILTTED_DATA_TYPE* demodulator_result,
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
	DbgMsg("gpu num %d\n", count);
	cudaGetDeviceProperties(&prop, 0);
	DbgMsg("max thread num: %d\n", prop.maxThreadsPerBlock);
	DbgMsg("max grid dimensions: %d, %d, %d)\n",
		prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
}

void cuda_CFilter2::Init(CFilter* f)
{
	UnInit();

	cuda_getThreadNum();

	cudaError_t err = cudaSuccess;

	cFilter = f;
	rootFilterInfo = &f->rootFilterInfo1;
	SrcData = f->SrcData;
	TargetData = f->TargetData;
	SrcLen = f->FilterSrcLen;

	DbgMsg("Cuda_Init [Vector addition of %d CUDA_FILTER_BUFF_SRC_LENGTH]\n", SrcLen);

	if (d_SrcData != NULL) {
		err = cudaFree(d_SrcData);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_SrcData failed!\r\n");
			//goto Error;
		}
	}
	size_t src_data_size = SrcLen * sizeof(ADC_DATA_TYPE);
	err = cudaMalloc((void**)&d_SrcData, src_data_size);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_SrcData failed!\r\n");
		//goto Error;
	}

	if (d_Filter_Core != NULL) {
		err = cudaFree(d_Filter_Core);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Filter_Core failed!\r\n");
			//goto Error;
		}
	}
	size_t filter_core_size = rootFilterInfo->CoreLength * sizeof(CFilter::FILTER_CORE_DATA_TYPE);
	err = cudaMalloc((void**)&d_Filter_Core, filter_core_size);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Filter_Core failed!\r\n");
		//goto Error;
	}
	err = cudaMemcpy(d_Filter_Core, rootFilterInfo->FilterCore, filter_core_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		DbgMsg("cudaMemcpy d_Filter_Core failed!\r\n");
		//goto Error;
	}

	if (d_Decimation_Cache != NULL) {
		err = cudaFree(d_Decimation_Cache);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Decimation_Cache failed!\r\n");
			//goto Error;
		}
	}

	size_t decimation_cache_size = (SrcLen >> rootFilterInfo->decimationFactorBit) * sizeof(FILTTED_DATA_TYPE);
	err = cudaMalloc((void**)&d_Decimation_Cache, decimation_cache_size);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Demodulator_Result failed!\r\n");
		//goto Error;
	}

	if (d_Filtted_Result != NULL) {
		err = cudaFree(d_Filtted_Result);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Filtted_Result failed!\r\n");
			//goto Error;
		}
	}
	size_t filter_result_size = (SrcLen >> rootFilterInfo->decimationFactorBit) * sizeof(FILTTED_DATA_TYPE);
	err = cudaMalloc((void**)&d_Filtted_Result, filter_result_size);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Filtted_Result failed!\r\n");
		//goto Error;
	}

	if (d_Demodulator_Result != NULL) {
		err = cudaFree(d_Demodulator_Result);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Demodulator_Result failed!\r\n");
			//goto Error;
		}
	}
	size_t demodulator_result_size = ((SrcLen >> 2) >> rootFilterInfo->decimationFactorBit) * sizeof(FILTTED_DATA_TYPE);
	err = cudaMalloc((void**)&d_Demodulator_Result, demodulator_result_size);
	if (err != cudaSuccess) {
		DbgMsg("cudaMalloc d_Demodulator_Result failed!\r\n");
		//goto Error;
	}

	DbgMsg("Cuda Filter2 Init Done.\r\n");
}

void cuda_CFilter2::UnInit(void)
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
	if (d_Filter_Core != NULL) {
		err = cudaFree(d_Filter_Core);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Filter_Core failed!\r\n");
			//goto Error;
		}
		d_Filter_Core = NULL;
	}
	if (d_Filtted_Result != NULL) {
		err = cudaFree(d_Filtted_Result);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Filtted_Result failed!\r\n");
			//goto Error;
		}
		d_Filtted_Result = NULL;
	}

	if (d_Decimation_Cache != NULL) {
		err = cudaFree(d_Decimation_Cache);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Decimation_Cache failed!\r\n");
			//goto Error;
		}
		d_Decimation_Cache = NULL;
	}
	if (d_Demodulator_Result != NULL) {
		err = cudaFree(d_Demodulator_Result);
		if (err != cudaSuccess) {
			DbgMsg("cudaFree d_Demodulator_Result failed!\r\n");
			//goto Error;
		}
		d_Demodulator_Result = NULL;
	}
	// Free host memory
	//free(h_Filter_Core);

	DbgMsg("cuda_CFilter2 UnInit Closed.\r\n");
}

void cuda_CFilter2::Filtting(void)
{
	cudaError_t err = cudaSuccess;
	static unsigned int stage = 0;

	size_t threadsPerBlock = (SrcLen >> 2) >> rootFilterInfo->decimationFactorBit;
	//int blocksPerGrid = (numElements + threadsPerBlock - 1) / threadsPerBlock;
	size_t blocksPerGrid = 1;

	CData* src_Data = (CData*)SrcData;
	h_SrcData = (ADC_DATA_TYPE*)src_Data->Buff + src_Data->ProcessPos;
	//DbgMsg("stage:%d\r\n", stage);
	static size_t stage_size = (SrcLen >> 2) * sizeof(ADC_DATA_TYPE);
	err = cudaMemcpy((char*)d_SrcData + stage * stage_size, h_SrcData, stage_size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		DbgMsg("cudaMemcpy d_SrcData failed!\r\n");
	}

	//DbgMsg("CUDA kernel launch with %d blocks of %d threads\n", blocksPerGrid, threadsPerBlock);

		//const ADC_DATA_TYPE* src, unsigned int srclen,
		//int stage,
		//FILTTED_DATA_TYPE* decimation_cache, unsigned int decimation_factor_bit,
		//const CFilter::FILTER_CORE_DATA_TYPE* core, unsigned int corelen,
		//CFilter::FILTER_CORE_DATA_TYPE* filtted_result,
		//float scale

	cuda_core_Filter2 <<< threadsPerBlock, blocksPerGrid >>> (
		d_SrcData, SrcLen,
		stage,
		d_Decimation_Cache, rootFilterInfo->decimationFactorBit,
		d_Filter_Core, rootFilterInfo->CoreLength,
		d_Filtted_Result,
		*cFilter->scale
		);

	err = cudaGetLastError();
	if (err != cudaSuccess) {
		DbgMsg("cudaFilter launch failed: %s\r\n", cudaGetErrorString(err));
	}
	// any errors encountered during the launch.
	err = cudaDeviceSynchronize();
	if (err != cudaSuccess) {
		DbgMsg("cudaDeviceSynchronize returned error code %d after launching cudaFilter!\r\n", err);
	}

	CData* target_Data = (CData*)TargetData;
	h_Filtted_Result = (FILTTED_DATA_TYPE*)target_Data->Buff + target_Data->Pos;
	err = cudaMemcpy(h_Filtted_Result, 
		d_Filtted_Result + stage * ((SrcLen >> 2) >> rootFilterInfo->decimationFactorBit),
		((SrcLen >> 2) >> rootFilterInfo->decimationFactorBit) * sizeof(FILTTED_DATA_TYPE),
		cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		DbgMsg("cudaMemcpy h_Filtted_Result failed!\r\n");
	}
	UINT T;
	T = src_Data->ProcessPos;
	src_Data->ProcessPos = (T + (SrcLen >> 2)) & src_Data->Mask;
	T = target_Data->Pos;
	target_Data->Pos = (T + ((SrcLen >> 2) >> rootFilterInfo->decimationFactorBit)) & target_Data->Mask;

	/*
	h_Demodulator_Result = (FILTTED_DATA_TYPE*)clsDemodulatorAm.AMBuff + clsDemodulatorAm.AMPos;
	err = cudaMemcpy(h_Demodulator_Result, d_Demodulator_Result, cuda_demodulator_result_size, cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		DbgMsg("cudaMemcpy h_Demodulator_Result failed!\r\n");
	}
	clsDemodulatorAm.AMPos += cuda_demodulator_result_size / sizeof(FILTTED_DATA_TYPE);
	clsDemodulatorAm.AMPos &= DEMODULATOR_BUFF_LENGTH_MASK;
	*/

	stage++;
	stage &= 0x3;
}