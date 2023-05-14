#pragma once

class CData;

namespace METHOD {

	class CFilter;

	class cuda_CFilter_AM
	{

	public:

		CData* SrcData = NULL;
		CData* TargetData = NULL;

		CFilter* cFilter = NULL;
		CFilter::PFILTER_INFO rootFilterInfo1 = NULL;
		CFilter::PFILTER_INFO rootFilterInfo2 = NULL;

		UINT SrcLen;

		CFilter::FILTER_CORE_DATA_TYPE* d_Filter_Core1 = NULL;
		CFilter::FILTER_CORE_DATA_TYPE* d_Filter_Core2 = NULL;

		ADC_DATA_TYPE* d_SrcData = NULL;
		ADC_DATA_TYPE* h_SrcData = NULL;

		FILTTED_DATA_TYPE* d_Decimation_Cache1 = NULL;
		FILTTED_DATA_TYPE* d_Decimation_Cache2 = NULL;

		FILTTED_DATA_TYPE* d_Filtted_Result1 = NULL;
		FILTTED_DATA_TYPE* h_Filtted_Result1 = NULL;

		FILTTED_DATA_TYPE* d_Filtted_Result2 = NULL;
		FILTTED_DATA_TYPE* h_Filtted_Result2 = NULL;

		FILTTED_DATA_TYPE* d_Demodulator_Result = NULL;
		FILTTED_DATA_TYPE* h_Demodulator_Result = NULL;

	public:
		cuda_CFilter_AM();
		~cuda_CFilter_AM();

		void Filtting(void);
		void UnInit(void);
		void Init(CFilter* f);

	};
}

extern METHOD::cuda_CFilter_AM clscudaMainFilter_AM;