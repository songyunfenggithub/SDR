#pragma once

class CData;

namespace METHOD {
	class CFilter;

	class cuda_CFilter2
	{

	public:

		CData* SrcData;
		CData* TargetData;

		CFilter::FILTER_CORE_DATA_TYPE* d_Filter_Core = NULL;
		ADC_DATA_TYPE* d_SrcData = NULL;
		FILTTED_DATA_TYPE* d_Decimation_Cache = NULL;
		FILTTED_DATA_TYPE* d_Filtted_Result = NULL;
		FILTTED_DATA_TYPE* d_Demodulator_Result = NULL;

		ADC_DATA_TYPE* h_SrcData = NULL;
		FILTTED_DATA_TYPE* h_Filtted_Result = NULL;
		FILTTED_DATA_TYPE* h_Demodulator_Result = NULL;

		CFilter* cFilter = NULL;
		CFilter::PFILTER_INFO rootFilterInfo = NULL;

		UINT SrcLen;

	public:
		cuda_CFilter2();
		~cuda_CFilter2();

		void Filtting(void);
		void UnInit(void);
		void Init(CFilter* f);

	};
}

extern METHOD::cuda_CFilter2 clscudaMainFilter2;
extern METHOD::cuda_CFilter2 clscudaMainFilter2Q;
extern METHOD::cuda_CFilter2 clscudaAudioFilter2;