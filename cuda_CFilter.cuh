
#pragma once

class CData;

namespace METHOD {
	class CFilter;

	class cuda_CFilter {

	public:
		CData* SrcData;
		CData* TargetData;

		char* d_SrcData = NULL;
		CFilter::FILTER_CORE_DATA_TYPE* d_Filter_Core = NULL;
		FILTTED_DATA_TYPE* d_Result = NULL;

		char* h_SrcData = NULL;
		FILTTED_DATA_TYPE* h_Result = NULL;

		size_t thread_size;
		size_t result_size;
		size_t src_step_size;

		CFilter* cFilter = NULL;
		CFilter::PFILTER_INFO rootFilterInfo = NULL;

		UINT SrcLen = 0;

	public:

		cuda_CFilter();
		~cuda_CFilter();

		void Filtting(void);
		void UnInit(void);
		void Init(CFilter* f);

		void getThreadNum(void);
	};
}
extern METHOD::cuda_CFilter clscudaMainFilter;
extern METHOD::cuda_CFilter clscudaMainFilterQ;
extern METHOD::cuda_CFilter clscudaAudioFilter;