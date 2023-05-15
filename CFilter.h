#pragma once

class CData;

namespace METHOD {

	class cuda_CFilter;
	class cuda_CFilter2;
	class cuda_CFilter3;


#define FILTER_WORK_THREADS		4

#define MAX_FILTER_NUMBER		0x100

#define CUDA_FILTER_ADC_BUFF_SRC_LENGTH			0x20000
#define CUDA_FILTER_AUDIO_BUFF_SRC_LENGTH		0x8000

	class CFilter
	{
	public:
		const char* TAG = NULL;

		typedef float FILTER_CORE_DATA_TYPE;

		typedef enum FILTER_TYPE_STRUCT {
			FilterLowPass = 0,
			FilterHighPass = 1,
			FilterBandPass = 2,
			FilterBandStop = 3
		} FILTER_TYPE;

		typedef struct FILTER_INFO_STRUCT;
		typedef FILTER_INFO_STRUCT FILTER_INFO, *PFILTER_INFO;

		struct FILTER_INFO_STRUCT
		{
			char* CoreDescStr = NULL;
			FILTER_TYPE Type;
			UINT	FreqCenter, BandWidth;
			double  FreqFallWidth;
			UINT	CoreLength;
			FILTER_CORE_DATA_TYPE* FilterCore = NULL;
			bool	Enable = false;
			INT		subFilteindex = 0;
			UINT	subFilterNum = 0;
			UINT	IterateLevel = 0;
			UINT	decimationFactorBit = 0;
			UINT	SampleRate = 0;
			FILTER_INFO* nextFilter = NULL;
		};

		FILTER_INFO rootFilterInfo1;
		FILTER_INFO rootFilterInfo2;

		typedef enum CUDA_FILTER_N_STRUCT {
			cuda_filter_1,
			cuda_filter_2,
			cuda_filter_3
		} CUDA_FILTER_N;
		
		CUDA_FILTER_N Cuda_Filter_N_New = cuda_filter_1;
		CUDA_FILTER_N Cuda_Filter_N_Doing = Cuda_Filter_N_New;

		HANDLE hFilterMutex;
		HANDLE hCoreMutex;

		HANDLE hThread = NULL;
		bool doFiltting = true;
		bool Thread_Exit = true;

		cuda_CFilter* cudaFilter = NULL;
		cuda_CFilter2* cudaFilter2 = NULL;
		cuda_CFilter3* cudaFilter3 = NULL;
		UINT FilterSrcLen = 0;

		CData* SrcData = NULL;
		CData* TargetData = NULL;

		float fscale = 1.0;
		float* Scale = &fscale;

	public:
		CFilter(const char* tag);
		~CFilter();

		void core(FILTER_INFO* pFilterInfo, FILTER_INFO* rootf);
		void corepluse(FILTER_CORE_DATA_TYPE* pR, FILTER_CORE_DATA_TYPE* pS, UINT corelen);
		void lowcore(float fc, FILTER_CORE_DATA_TYPE* pBuf, UINT corelen);
		void invcore(FILTER_CORE_DATA_TYPE* pBuf, UINT corelen);

		bool CheckCoreDesc(char* coreDesc);
		void ParseCoreDesc(void);
		int  ParseOneCore(FILTER_INFO* rootFilterInfo);

		void build_desc_core(FILTER_INFO* rootFilterInfo);
		void build_iterate_core(void);
		void build_last_iterate_core(FILTER_INFO* rootFilterInfo);

		void ReBuildFilterCore(void);

		void setFilterCoreDesc(FILTER_INFO* pFilterInfo, char* CoreDescStr);
		void set_cudaFilter(cuda_CFilter* f, cuda_CFilter2* f2, cuda_CFilter3* f3, UINT filterSrcLen);

		void write_core(FILTER_INFO* rootFilterInfo);

		static LPTHREAD_START_ROUTINE filter_thread1(LPVOID lp);
		static LPTHREAD_START_ROUTINE filter_thread2(LPVOID lp);
		static LPTHREAD_START_ROUTINE filter_thread3(LPVOID lp);
		static LPTHREAD_START_ROUTINE filter_thread4(LPVOID lp);

		void filter_func_short(UINT32 thread_id);
		void filter_func_float(UINT32 thread_id);

		static LPTHREAD_START_ROUTINE cuda_filter_thread(LPVOID lp);
		void cuda_filter_func(void);

		void SaveValue(void);
		void RestoreValue(void);
	};
}

extern METHOD::CFilter clsMainFilterI;
//extern METHOD::CFilter clsMainFilterQ;
extern METHOD::CFilter clsAudioFilter;