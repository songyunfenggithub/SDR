
#pragma once


class cuda_CFilter {

public:
	FILTER_CORE_DATA_TYPE* d_Filter_Core = NULL;
	ADCDATATYPE* d_SrcData = NULL;
	FILTEDDATATYPE* d_Result = NULL;

	//FILTER_CORE_DATA_TYPE* h_Filter_Core = NULL;
	ADCDATATYPE* h_SrcData = NULL;
	FILTEDDATATYPE* h_Result = NULL;

	size_t thread_size;
	size_t result_size;
	size_t src_step_size;

	CWaveFilter::PFILTERINFO rootFilterInfo = NULL;

	CWaveFilter::PFILTERINFO FilterInfo;

public:
	
	void Filtting(void);
	void UnInit(void);
	void Init(CWaveFilter::PFILTERINFO pFilterInfo);

	void cuda_CFilter::getThreadNum(void);
};

extern cuda_CFilter clscudaFilter;