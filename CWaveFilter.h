#pragma once
#include "stdafx.h"

#include "public.h"


#define FILTER_CORE_DESC_MAX_LENGTH		8192

//#define FILTER_CORE_BUILD_LENGTH      513
//#define FILTER_CORE_BUILD_LENGTH      1025
//#define FILTER_CORE_BUILD_LENGTH      2049
//#define FILTER_CORE_BUILD_LENGTH      4097
//#define FILTER_CORE_BUILD_LENGTH		8193
#define FILTER_CORE_BUILD_LENGTH		1025
//#define FILTER_CORE_BUILD_LENGTH      2049
//#define FILTER_CORE_BUILD_LENGTH      16385
//#define FILTER_CORE_LENGTH      2049
//#define FILTER_CORE_LENGTH      16385
//#define FILTER_CORE_LENGTH      8193

#define FILTER_WORK_THREADS		4

#define MAX_FILTER_NUMBER		0x100

typedef float FILTERCOREDATATYPE;


#define CUDA_FILTER_BUFF_SRC_LENGTH				0x10000
#define CUDA_FILTER_BUFF_SRC_LENGTH_MASK		0x0FFFF
#define CUDA_FILTER_BUFF_STEP_LENGTH		(CUDA_FILTER_BUFF_SRC_LENGTH >> 2)
#define CUDA_FILTER_BUFF_STEP_LENGTH_MASK	(CUDA_FILTER_BUFF_SRC_LENGTH_MASK >> 2)

#define FILTER_DESC_LENGTH		1024

class CWaveFilter
{
public:
	typedef enum _FilterType {
		FilterLowPass = 0,
		FilterHighPass = 1,
		FilterBandPass = 2,
		FilterBandStop = 3
	} FilterType;

	typedef struct FILTERINFO;
	typedef FILTERINFO* PFILTERINFO;

	struct FILTERINFO
	{
		char* CoreDescStr = NULL;
		FilterType   Type;
		UINT	FreqCenter,BandWidth,SampleRate;
		double  FreqFallWidth;
		UINT	  CoreLength;
		FILTERCOREDATATYPE* FilterCore = NULL;
		bool	Enable = false;
		INT		subFilteindex = 0;
		UINT	subFilterNum = 0;
		UINT	IterateLevel = 0;
		UINT	decimationFactor = 1;
		FILTERINFO* nextFilter = NULL;
	};

	//FILTERINFO FilterInfos[MAX_FILTER_NUMBER];
	FILTERINFO rootFilterInfo;

	int FilterInfoCount = 0;

	FILTERINFO *pCurrentFilterInfo;

	//FILTERCOREDATATYPE*	FilterCore = NULL;
	//int		FilterCoreLength = 0;
	//int     FilterCoreIterateLevel = 0;
	UINT32 filter_ready_poss[FILTER_WORK_THREADS] = { 0, 0, 0, 0 };

	bool isInFiltting = true;	

	HANDLE hFilterMutex;	//定义互斥对象句柄
	HANDLE hCoreMutex;	//定义互斥对象句柄

	HANDLE hCoreAnalyseMutex;	//定义互斥对象句柄

	double* CoreAnalyseDataBuff			= NULL;
	double* CoreAnalyseFilttedDataBuff	= NULL;
	double* CoreAnalyseFFTBuff			= NULL;
	double* CoreAnalyseFFTLogBuff		= NULL;
	int		CoreAnalyseFFTLength		= 0;

	char FilterCoreDesc[FILTER_CORE_DESC_MAX_LENGTH];

	bool cud_Filter_exit = false;

public:
	CWaveFilter();
	~CWaveFilter();

	void core(PFILTERINFO pFilterInfo);
	void corepluse(FILTERCOREDATATYPE* pR, FILTERCOREDATATYPE* pS, UINT32 corelen);
	void lowcore(FILTERCOREDATATYPE fc, FILTERCOREDATATYPE* pBuf, UINT32 corelen);
	void invcore(FILTERCOREDATATYPE* pBuf, UINT32 corelen);


	void GetCMDFilter(char* pCMD);
	void GetCMDFilterDesc(char* pCmd);
	int  ParseCoreDesc(PFILTERINFO rootFilterInfo);

	void build_desc_core(PFILTERINFO rootFilterInfo);
	void build_iterate_core(void);
	void build_last_iterate_core(PFILTERINFO rootFilterInfo);
	
	void ReBuildFilterCore(void);

	void setFilterCoreDesc(PFILTERINFO pFilterInfo, char*CoreDescStr);

	void write_core(PFILTERINFO rootFilterInfo);

	static LPTHREAD_START_ROUTINE filter_thread1(LPVOID lp);
	static LPTHREAD_START_ROUTINE filter_thread2(LPVOID lp);
	static LPTHREAD_START_ROUTINE filter_thread3(LPVOID lp);
	static LPTHREAD_START_ROUTINE filter_thread4(LPVOID lp);

	void filter_func(UINT32 thread_id);
	
	static LPTHREAD_START_ROUTINE cuda_filter_thread(LPVOID lp);
	void cuda_filter_func(void);

	void FilterCoreAnalyse(PFILTERINFO pFilterInfo);
};

extern CWaveFilter clsWaveFilter;