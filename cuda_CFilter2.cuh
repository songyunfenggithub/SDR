#pragma once

class cuda_CFilter2 
{

public:
	FILTER_CORE_DATA_TYPE* d_Filter_Core = NULL;
	ADCDATATYPE* d_SrcData = NULL;
	FILTEDDATATYPE* d_Decimation_Cache = NULL;
	FILTEDDATATYPE* d_Filtted_Result = NULL;
	FILTEDDATATYPE* d_Demodulator_Result = NULL;

	ADCDATATYPE* h_SrcData = NULL;
	FILTEDDATATYPE* h_Filtted_Result = NULL;
	FILTEDDATATYPE* h_Demodulator_Result = NULL;

	CWaveFilter::PFILTERINFO rootFilterInfo = NULL;

public:
	cuda_CFilter2();
	~cuda_CFilter2();

	void Filtting(void);
	void UnInit(void);
	void Init(CWaveFilter::PFILTERINFO pFilterInfo);

};

extern cuda_CFilter2 clscudaFilter2;