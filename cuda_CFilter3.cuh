#pragma once

class CData;

namespace METHOD {

	class CFilter;

	class cuda_CFilter3
	{

	public:

		CData* SrcData = NULL;
		CData* TargetData1 = NULL;
		CData* TargetData2 = NULL;

		CFilter* cFilter = NULL;
		CFilter::PFILTER_INFO rootFilterInfo1 = NULL;
		CFilter::PFILTER_INFO rootFilterInfo2 = NULL;

		UINT SrcLen;

		FILTER_CORE_DATA_TYPE* d_Filter_Core1 = NULL;
		FILTER_CORE_DATA_TYPE* d_Filter_Core2 = NULL;

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
		cuda_CFilter3();
		~cuda_CFilter3();

		void Filtting(void);
		void UnInit(void);
		void Init(CFilter* f);

	};
}

extern METHOD::cuda_CFilter3 clscudaMainFilter3;