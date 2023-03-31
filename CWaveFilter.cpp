#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>

#include "CDataFromTcpIp.h"
#include "CWaveData.h"
#include "CWaveFilter.h"
#include "CWaveFFT.h"

#include "cuda_Filter.cuh"

using namespace std;

//CWaveFilter::FILTERINFO CWaveFilter::FilterInfos[MAX_FILTER_NUMBER];
//double* CWaveFilter::FilterCore = NULL;
//int		CWaveFilter::FilterCoreLength = 0;
//int		CWaveFilter::FilterInfoCount = 0;
//UINT32	CWaveFilter::filter_ready_poss[4] = { 0, 0, 0, 0 };

CWaveFilter::PFILTERINFO pCurrentFilterInfo = NULL;

CWaveFilter clsWaveFilter;

CWaveFilter::CWaveFilter()
{
	OPENCONSOLE;

	int i;
	for (i = 0; i < FILTER_WORK_THREADS; i++)
	{
		filter_ready_poss[i] = 0;
	}

	/*
	for (i = 0; i < MAX_FILTER_NUMBER; i++)
	{
		FilterInfos[i].Enable		= false;
		FilterInfos[i].FilterCore	= NULL;
	}
	*/

	hFilterMutex = CreateMutex(NULL, false, "hFilterMutex");		//创建互斥对象

	hCoreMutex = CreateMutex(NULL, false, "hCoreMutex");		//创建互斥对象

	hCoreAnalyseMutex = CreateMutex(NULL, false, "hCoreAnalyseMutex");		//创建互斥对象

	//char s[] = "1025, 0, 0; 0, 50, 10; 2, 100, 10";//; 1, 100, 10; 1, 500, 10; 1, 600, 10";
	//strcpy(FilterCoreDesc, s);
	
//	const char h[] = " 65,0,0;0,50000,10000;2,100000,10000;1,150000,10000";
	const char h[] = " 33,0,0;0,50000,100";
	char filterdes[1024];
	char t[100];
	int n, m;
	strcpy(FilterCoreDesc, h);
	m = strlen(h);
	FilterCoreDesc[m] = 0;

	/*
	n = sprintf(t, " %d,%d,%d;", 1, 1000000, 200000);
	sprintf(FilterCoreDesc + m, "%s", t);
	m += n;
	for (i = 2; i < 2; i++) {
		n = sprintf(t, " %d,%d,%d;", 1, i*5000, 40);
		sprintf(FilterCoreDesc + m, "%s", t);
		m += n;
	}
	n = sprintf(t, " %d,%d,%d", 1, i*5000, 200000);
	sprintf(FilterCoreDesc + m, "%s", t);
	*/

	pCurrentFilterInfo = &rootFilterInfo;
	setFilterCoreDesc(&rootFilterInfo, (char*)h);
}

CWaveFilter::~CWaveFilter()
{
	CLOSECONSOLE;
}

void CWaveFilter::setFilterCoreDesc(PFILTERINFO pFilterInfo, char* CoreDescStr)
{
	if (pFilterInfo->CoreDescStr != NULL) delete[] pFilterInfo->CoreDescStr;
	pFilterInfo->CoreDescStr = new char[strlen(CoreDescStr)+1];
	sprintf(pFilterInfo->CoreDescStr, "%s", CoreDescStr);
}

void CWaveFilter::build_desc_core(PFILTERINFO rootFilterInfo)
{
	int i;
	PFILTERINFO pFilterInfo = rootFilterInfo->nextFilter;
	while(pFilterInfo != NULL)
	{
		if(rootFilterInfo->CoreLength < pFilterInfo->CoreLength) rootFilterInfo->CoreLength = pFilterInfo->CoreLength;
		pFilterInfo = pFilterInfo->nextFilter;
	}

	if (rootFilterInfo->FilterCore != NULL) {delete[] rootFilterInfo->FilterCore; rootFilterInfo->FilterCore = NULL;}
	rootFilterInfo->FilterCore = new FILTERCOREDATATYPE[rootFilterInfo->CoreLength];
	memset(rootFilterInfo->FilterCore, 0, rootFilterInfo->CoreLength * sizeof(FILTERCOREDATATYPE));

	pFilterInfo = rootFilterInfo->nextFilter;
	while (pFilterInfo != NULL)
	{
		core(pFilterInfo);
		for (i = 0; i < pFilterInfo->CoreLength; i++)
		{
			rootFilterInfo->FilterCore[i] += pFilterInfo->FilterCore[i];
		}
		pFilterInfo = pFilterInfo->nextFilter;
	}
	write_core(rootFilterInfo);
}

void CWaveFilter::build_last_iterate_core(PFILTERINFO rootFilterInfo)
{
	if (rootFilterInfo->IterateLevel < 2) return;

	int tFilterLength, tOrignalFilterLength;
	int i;
	FILTERCOREDATATYPE* tFilterCore = NULL;
	FILTERCOREDATATYPE* tOrignalFilterCore = new FILTERCOREDATATYPE[rootFilterInfo->CoreLength];
	memcpy(tOrignalFilterCore, rootFilterInfo->FilterCore, rootFilterInfo->CoreLength * sizeof(FILTERCOREDATATYPE));
	tOrignalFilterLength = rootFilterInfo->CoreLength;
	for (i = 1; i < rootFilterInfo->IterateLevel; i++)
	{
		tFilterLength = rootFilterInfo->CoreLength;
		if (tFilterCore != NULL)delete[] tFilterCore;
		tFilterCore = new FILTERCOREDATATYPE[tFilterLength];
		memcpy(tFilterCore, rootFilterInfo->FilterCore, tFilterLength * sizeof(FILTERCOREDATATYPE));
		rootFilterInfo->CoreLength = tFilterLength + tOrignalFilterLength - 1;
		delete[] rootFilterInfo->FilterCore;
		rootFilterInfo->FilterCore = new FILTERCOREDATATYPE[rootFilterInfo->CoreLength];
		memset(rootFilterInfo->FilterCore, 0, rootFilterInfo->CoreLength * sizeof(FILTERCOREDATATYPE));
		int i1, i2;
		for (i2 = 0; i2 < tOrignalFilterLength; i2++)
		{
			for (i1 = 0; i1 < tFilterLength; i1++)
			{
				rootFilterInfo->FilterCore[i2 + i1] += tFilterCore[i1] * tOrignalFilterCore[i2];
			}
		}
	}
	write_core(rootFilterInfo);
}

void CWaveFilter::build_iterate_core(void)
{
	/*
	int tFilterLength = 0;
	int i;
	PFILTERINFO pFilterInfo1 = NULL, pFilterInfo2 = NULL;
	FILTERCOREDATATYPE* tFilterCore, *CoreMatrix;
	if (FilterCore != NULL)
	{
		delete[] FilterCore;
		FilterCore = NULL;
	}
	FilterCoreLength = 0;
	int FilterEnableNum = 0;
	for (i = 0; i < MAX_FILTER_NUMBER; i++)
	{
		if (FilterInfos[i].Enable) {
			FilterEnableNum++;
			core(pFilterInfo2 = &FilterInfos[i]);
			if (pFilterInfo1 && pFilterInfo2) 
			{
				if (FilterCore == NULL)
				{
					FilterCoreLength = pFilterInfo1->CoreLength;
					FilterCore = new FILTERCOREDATATYPE[FilterCoreLength];
					memcpy(FilterCore, pFilterInfo1->FilterCore, FilterCoreLength * sizeof(FILTERCOREDATATYPE));
				}

				int i1, i2;
				FILTERCOREDATATYPE*tFilterCore = FilterCore;
				int tFilterCoreLength = FilterCoreLength + pFilterInfo2->CoreLength - 1;
				FilterCore = new FILTERCOREDATATYPE[tFilterCoreLength]{ 0 };
				for (i1 = 0; i1 < tFilterCoreLength; i1++) FilterCore[i1] = 0.0;
				for (i2 = 0; i2 < pFilterInfo2->CoreLength; i2++) 
				{
					for (i1 = 0; i1 < FilterCoreLength; i1++)
					{
						FilterCore[i2 + i1] += tFilterCore[i1] * pFilterInfo2->FilterCore[i2];
					}
				}
				FilterCoreLength = tFilterCoreLength;
				delete[] tFilterCore;
			}
			pFilterInfo1 = pFilterInfo2;
		}
	}
	if (FilterEnableNum == 1)
	{
		FilterCore = pFilterInfo1->FilterCore;
		FilterCoreLength = pFilterInfo1->CoreLength;
	}
	write_core();
	*/
}

void CWaveFilter::invcore(FILTERCOREDATATYPE* pBuf, UINT32 corelen)
{
	UINT16 i;
	for (i = 0; i < corelen; i++) pBuf[i] *= -1.0;
	pBuf[(corelen - 1) / 2] += 1.0;
	//*(double*)(pBuf + (pFilterInfo->CoreLength-1)/2*4) += 1000.0;
}

void CWaveFilter::lowcore(FILTERCOREDATATYPE fc, FILTERCOREDATATYPE* pBuf, UINT32 corelen)
{
	UINT16 K = 1;
	UINT16 i;
	FILTERCOREDATATYPE* p;
	FILTERCOREDATATYPE sum;
	UINT16 M = corelen - 1;
	p = pBuf;
	//(0.42 - 0.5*cos(2*M_PI*i/M) + 0.08*cos(4*M_PI*i/M)) 布莱克曼窗
	//汉明窗
	for (i = 0; i < corelen; i++, p++)
	{
		if (i == M / 2)
		{
			*p = K * 2 * M_PI * fc;
		}
		else
		{
			*p = K * sin(2 * M_PI * fc * (i - M / 2)) / (i - M / 2);
		}
		*p *= (0.54 - 0.46 * cos(2 * M_PI * i / M));
	}
	for (p = pBuf, sum = 0.0, i = 0; i < corelen; i++)sum += *p++;
	for (p = pBuf, i = 0; i < corelen; i++)*p++ /= sum;
}

void CWaveFilter::corepluse(FILTERCOREDATATYPE* pR, FILTERCOREDATATYPE* pS, UINT32 corelen)
{
	//  double *pR = FilterCore;
	//  double *pS = FilterCore_2;
	UINT16 i;
	for (i = 0; i < corelen; i++) *pR++ += *pS++;
}

/*
type: 0低通 1高通 2带通 3阻带
freq0： 中心频率
freq_width： 通带/阻带 频宽
*/
void CWaveFilter::core(PFILTERINFO pFilterInfo)
{
	FILTERCOREDATATYPE fl = pFilterInfo->FreqCenter - pFilterInfo->BandWidth / 2;
	FILTERCOREDATATYPE fh = pFilterInfo->FreqCenter + pFilterInfo->BandWidth / 2;
	UINT32 fs = pFilterInfo->SampleRate / pFilterInfo->decimationFactor;
	UINT32 CoreLength = pFilterInfo->CoreLength;

	pFilterInfo->FreqFallWidth = 4.0 / (CoreLength - 1) * fs;
	if (pFilterInfo->FilterCore != NULL) delete[] pFilterInfo->FilterCore;
	pFilterInfo->FilterCore = new FILTERCOREDATATYPE[CoreLength];
	FILTERCOREDATATYPE* FilterCore = pFilterInfo->FilterCore;

	FilterInfoCount++;

	FILTERCOREDATATYPE* TempBuf = new FILTERCOREDATATYPE[CoreLength];

	switch (pFilterInfo->Type)
	{
	case FilterLowPass:
		lowcore(fl / fs, FilterCore, CoreLength);
		break;
	case FilterHighPass:
		lowcore(fh / fs, FilterCore, CoreLength);
		invcore(FilterCore, CoreLength);
		break;
	case FilterBandPass:
		lowcore(fl / fs, FilterCore, CoreLength);
		lowcore(fh / fs, TempBuf, CoreLength);
		invcore(TempBuf, CoreLength);
		corepluse(FilterCore, TempBuf, CoreLength);
		invcore(FilterCore, CoreLength);
		break;
	case FilterBandStop:
		lowcore(fl / fs, FilterCore, CoreLength);
		lowcore(fh / fs, TempBuf, CoreLength);
		invcore(TempBuf, CoreLength),
		corepluse(FilterCore, TempBuf, CoreLength);
		break;
	}

	delete[] TempBuf;
}

void CWaveFilter::ReBuildFilterCore(void)
{
	clsWaveFilter.ParseCoreDesc(&rootFilterInfo);
	clsWaveFilter.FilterCoreAnalyse(&rootFilterInfo);
}

void CWaveFilter::write_core(PFILTERINFO rootFilterInfo)
{
	ofstream outfile;
	string t1;
	string t2;
	char str[1024];
	PFILTERINFO pFilterInfo;
	outfile.open("core.txt");
	if (!outfile.is_open())	cout << "open file core.txt failure" << endl;

	outfile << "# 滤波器内核 " << endl;
	outfile << "#   采样频率   " << clsWaveData.AdcSampleRate << endl;

	pFilterInfo = rootFilterInfo->nextFilter;

	int n = 0;
	while (pFilterInfo != NULL)
	{
		outfile << "# 滤波器内核 " << n++ << endl;
		outfile << "#   滤波器类型 " << pFilterInfo->Type << endl;
		outfile << "#   内核长度   " << pFilterInfo->CoreLength << endl;
		outfile << "#   中心频率   " << pFilterInfo->FreqCenter << endl;
		outfile << "#   通/阻带宽  " << pFilterInfo->BandWidth << endl;
		outfile << "#   下降带宽   " << pFilterInfo->FreqFallWidth << endl;
		for (int i = 0; i < pFilterInfo->CoreLength; i++)
		{
			sprintf(str, "%d,%.10lf", i, pFilterInfo->FilterCore[i]);
			outfile << str << endl;
		}
		pFilterInfo = pFilterInfo->nextFilter;
	}

	outfile << "# 滤波器内核." << rootFilterInfo->subFilterNum << endl;
	outfile << "#   滤波器内核长度 " << rootFilterInfo->CoreLength << endl;
	outfile << "#   采样频率       " << clsWaveData.AdcSampleRate << endl;

	for (int i = 0; i < rootFilterInfo->CoreLength; i++)
	{
		sprintf(str, "%d,%.10lf", i, rootFilterInfo->FilterCore[i]);
		outfile << str << endl;
	}

	outfile.close();
}

LPTHREAD_START_ROUTINE CWaveFilter::filter_thread1(LPVOID lp)
{
	clsWaveFilter.filter_func(0);
	return 0;
}

LPTHREAD_START_ROUTINE CWaveFilter::filter_thread2(LPVOID lp)
{
	clsWaveFilter.filter_func(1);
	return 0;
}

LPTHREAD_START_ROUTINE CWaveFilter::filter_thread3(LPVOID lp)
{
	clsWaveFilter.filter_func(2);
	return 0;
}

LPTHREAD_START_ROUTINE CWaveFilter::filter_thread4(LPVOID lp)
{
	clsWaveFilter.filter_func(3);
	return 0;
}

void CWaveFilter::filter_func(UINT32 thread_id)
{
	ADCDATATYPE* pbuf1, * pend1, * pbuf2, * pend2;
	FILTERCOREDATATYPE* pcore = NULL;
	FILTERCOREDATATYPE* pc = NULL;
	FILTERCOREDATATYPE* psave = NULL;
	FILTERCOREDATATYPE r;
	bool iswork;
	UINT32 work_pos = 0;

	while (Program_In_Process == true)
	{
		iswork = FALSE;
		WaitForSingleObject(hFilterMutex, INFINITE);
		if (clsWaveData.FilttingPos != clsWaveData.AdcPos)
		{
			work_pos = clsWaveData.FilttingPos;
			iswork = TRUE;
			clsWaveData.FilttingPos++;
			if (clsWaveData.FilttingPos == DATA_BUFFER_LENGTH)clsWaveData.FilttingPos = 0;
		}
		while (clsWaveData.FilttedFlag[clsWaveData.FilttedPos])
		{
			clsWaveData.FilttedFlag[clsWaveData.FilttedPos] = 0;
			clsWaveData.FilttedPos++;
			if (clsWaveData.FilttedPos == DATA_BUFFER_LENGTH)clsWaveData.FilttedPos = 0;
		}
		/*
		if (((clsWaveData.FilttedPos - clsWaveData.FilttedForwardPos) & DATA_BUFFER_MASK) > FILTED_FORWORD_PAGE_SIZE) {
			clsGetDataTcpIp.SendFiltedData((char*)(clsWaveData.FilttedBuff + clsWaveData.FilttedForwardPos), FILTED_FORWORD_PAGE_SIZE * 8);
			clsWaveData.FilttedForwardPos = clsWaveData.FilttedForwardPos + FILTED_FORWORD_PAGE_SIZE;
			if (clsWaveData.FilttedForwardPos == DATA_BUFFER_LENGTH)clsWaveData.FilttedForwardPos = 0;
		}
		*/
		ReleaseMutex(hFilterMutex);

		if (iswork)
		{
			UINT32 w_pos = work_pos + 1;
			WaitForSingleObject(hCoreMutex, INFINITE);
			if (w_pos < pCurrentFilterInfo->CoreLength)
			{
				pbuf1 = clsWaveData.AdcBuff + DATA_BUFFER_LENGTH - (pCurrentFilterInfo->CoreLength - w_pos) ;
				pend1 = clsWaveData.AdcBuff + DATA_BUFFER_LENGTH;
				pbuf2 = clsWaveData.AdcBuff;
				pend2 = clsWaveData.AdcBuff + w_pos;
			}
			else
			{
				pbuf1 = clsWaveData.AdcBuff + (w_pos - pCurrentFilterInfo->CoreLength);
				pend1 = clsWaveData.AdcBuff + w_pos;
				pbuf2 = pend2 = 0;
			}
			if(psave != pCurrentFilterInfo->FilterCore){
				psave = pCurrentFilterInfo->FilterCore;
				if (pcore != NULL) delete[] pcore;
				pcore = new FILTERCOREDATATYPE[pCurrentFilterInfo->CoreLength];
				memcpy(pcore, pCurrentFilterInfo->FilterCore, sizeof(FILTERCOREDATATYPE) * pCurrentFilterInfo->CoreLength);
			}
			pc = pcore;
			ReleaseMutex(hCoreMutex);
			r = 0.0;
			while (pbuf1 != pend1)
			{
				r += *pc++ * *pbuf1++;
			}
			while (pbuf2 != pend2)
			{
				r += *pc++ * *pbuf2++;
			}
			clsWaveData.FilttedBuff[work_pos] = r;
			//filter_ready_poss[thread_id] = work_pos;
			clsWaveData.FilttedFlag[work_pos] = 1;
		} else Sleep(0);
	}
}

LPTHREAD_START_ROUTINE CWaveFilter::cuda_filter_thread(LPVOID lp)
{

	clsWaveFilter.cuda_filter_func();

	return 0;
}

void CWaveFilter::cuda_filter_func(void)
{
	while (isInFiltting == true && Program_In_Process == true)
	{
		if (((clsWaveData.AdcPos - clsWaveData.FilttedPos) & DATA_BUFFER_MASK) > CUDA_FILTER_BUFF_STEP_LENGTH)
		{

			cuda_Filtting();
		}
		else Sleep(0);
	}
	cud_Filter_exit = true;
}

enum FilterCMDSettingValueType{
	FilterCMDSettingType			= 0,
	FilterCMDSettingFreqCenter		= 1,
	FilterCMDSettingBandWidth		= 2,
	FilterCMDSettingCoreLength		= 3,
	FilterCMDSettingFreqFallWidth	= 4,
	FilterCMDSettingEnable			= 5
};

void CWaveFilter::GetCMDFilter(char* pCmd)
{
	/*
	PFILTERINFO pInfo = &FilterInfos[pCmd[1]];
	switch (pCmd[2])
	{
	case FilterCMDSettingType:
		pInfo->Type = (FilterType)pCmd[8];
		cout << pInfo->Type << endl;
		break;
	case FilterCMDSettingFreqCenter:
		pInfo->FreqCenter = *((UINT32*)&pCmd[8]);
		cout << pInfo->FreqCenter << endl;
		break;
	case FilterCMDSettingBandWidth:
		pInfo->BandWidth = *((UINT32*)&pCmd[8]);
		cout << pInfo->BandWidth << endl;
		break;
	case FilterCMDSettingCoreLength:
		pInfo->CoreLength = *((UINT32*)&pCmd[8]);
		cout << pInfo->CoreLength << endl;
		break;
	case FilterCMDSettingFreqFallWidth:
		pInfo->FreqFallWidth = *((double*)&pCmd[8]);
		cout << pInfo->FreqFallWidth << endl;
		break;
	case FilterCMDSettingEnable:
		pInfo->Enable = pCmd[8] == 0 ? false : true;
		cout << pInfo->Enable << endl;
		break;
	default:
		break;
	}
	*/

}

void CWaveFilter::GetCMDFilterDesc(char* pCmd)
{
	static int n = 0;
	if (pCmd[1] == 0) n = 0;
	n += sprintf(FilterCoreDesc + n, "%d,%d,%d;", *((INT32*)&pCmd[4]), *((INT32*)&pCmd[8]), *((INT32*)&pCmd[12]));
	FilterCoreDesc[n] = '\0';
}

void CWaveFilter::FilterCoreAnalyse(PFILTERINFO pFilterInfo)
{
	
	FILTERCOREDATATYPE* FilterCore = pFilterInfo->FilterCore;
	if (FilterCore)
	{
		int CoreLength = pFilterInfo->CoreLength;

		/*
		// 生成信号----------------------------------------
		CoreAnalyseFFTLength = 1;
		while (CoreAnalyseFFTLength < clsWaveData.AdcSampleRate) CoreAnalyseFFTLength = CoreAnalyseFFTLength << 1;
		//CoreAnalyseFFTLength <<= 1;
		int n = CoreAnalyseFFTLength + CoreLength;
		CoreAnalyseDataBuff = new double[n];
		long i, j;
		double amplitude = (double)((long)1 << (ADC_DATA_SAMPLE_BIT - 1) - 1) / (clsWaveData.AdcSampleRate );
		double d;
		for (i = 0; i < n; i++)
		{
			d = (double)0.0;
			for (j = 1; j <= CoreAnalyseFFTLength / 2; j += 1)
			{
				//if (j == 50)// || j == 501 || j == 521 || j == 511)
				d += (double)amplitude * sin((double)2 * (double)M_PI / ((double)clsWaveData.AdcSampleRate / (double)j) * (double)i);
			}
			//double f = 50;// *clsWaveData.AdcSampleRate / CoreAnalyseFFTLength;
			//d += (double)amplitude * sin((double)2 * (double)M_PI / ((double)clsWaveData.AdcSampleRate / (double)f) * (double)i);
			CoreAnalyseDataBuff[i] = d;
		}
		
		// 滤波信号----------------------------------------
		if (CoreAnalyseFilttedDataBuff != NULL) { delete[] CoreAnalyseFilttedDataBuff; CoreAnalyseFilttedDataBuff = NULL; }
		CoreAnalyseFilttedDataBuff = new double[CoreAnalyseFFTLength];
		for (i = 0; i < CoreAnalyseFFTLength; i++)
		{
			CoreAnalyseFilttedDataBuff[i] = 0.0;
			for (j = 0; j < CoreLength; j++) 
			{
				CoreAnalyseFilttedDataBuff[i] += CoreAnalyseDataBuff[i + j] * FilterCore[j];
			}
			//CoreAnalyseFilttedDataBuff[i] = CoreAnalyseDataBuff[i];
		}
		*/


		// FFT信号----------------------------------------
		CoreAnalyseFFTLength = 1;
		while (CoreAnalyseFFTLength < CoreLength) CoreAnalyseFFTLength = CoreAnalyseFFTLength << 1;
		FILTERCOREDATATYPE* CoreFFTBuff = new FILTERCOREDATATYPE[CoreAnalyseFFTLength];
		memset(CoreFFTBuff, 0, sizeof(FILTERCOREDATATYPE) * CoreAnalyseFFTLength);
		memcpy(CoreFFTBuff, FilterCore, sizeof(FILTERCOREDATATYPE) * CoreLength);

		WaitForSingleObject(hCoreAnalyseMutex, INFINITE);

		if (CoreAnalyseFFTBuff != NULL) { delete[] CoreAnalyseFFTBuff;    CoreAnalyseFFTBuff = NULL; }
		if (CoreAnalyseFFTLogBuff != NULL) { delete[] CoreAnalyseFFTLogBuff; CoreAnalyseFFTLogBuff = NULL; }
		clsWaveFFT.only_FFT(
			//FilterCore,
			//CoreAnalyseFilttedDataBuff,
			CoreFFTBuff,
			CoreAnalyseFFTLength,// = CoreLength - 1,
			&CoreAnalyseFFTBuff,
			&CoreAnalyseFFTLogBuff
		);
		ReleaseMutex(hCoreAnalyseMutex);
	}
}

int CWaveFilter::ParseCoreDesc(PFILTERINFO rootFilterInfo)
{
	if (rootFilterInfo->FilterCore != NULL) delete[] rootFilterInfo->FilterCore;
	rootFilterInfo->FilterCore = NULL;
	PFILTERINFO tt, pp = rootFilterInfo->nextFilter;
	while (pp != NULL) {
		if (pp->CoreDescStr != NULL) delete[] pp->CoreDescStr;
		if (pp->FilterCore != NULL) delete[] pp->FilterCore;
		tt = pp;
		pp = pp->nextFilter;
		delete[] tt;
	}
	rootFilterInfo->nextFilter = NULL;
	rootFilterInfo->SampleRate = clsWaveData.AdcSampleRate;
	char* str = new char[strlen(rootFilterInfo->CoreDescStr)+1];
	sprintf(str, "%s", rootFilterInfo->CoreDescStr);
	int i, m, n;
	i = 0;
	char* p = str;
	char* Filters[MAX_FILTER_NUMBER];
	char* values[MAX_FILTER_NUMBER][3];
	n = 0;
	Filters[n++] = str;
	while (*p != '\0')
	{
		if (*p == ';')
		{
			Filters[n++] = p + 1;
			*p = '\0';
		}
		p++;
	}
	if (n > MAX_FILTER_NUMBER)
	{
		printf("滤波器核数量大于最大值 %d / %d\r\n", n, MAX_FILTER_NUMBER);
		return -1;
	}
	if (n < 2)
	{
		printf("滤波器核数量至少有一个 %d / (1 - %d)\r\n", n, MAX_FILTER_NUMBER);
		return -1;
	}
	for (i = 0; i < n; i++)
	{
		m = 0;
		values[i][m++] = p = Filters[i];
		while (*p != '\0')
		{
			if (*p == ',')
			{
				values[i][m++] = p + 1;
				*p = '\0';
			}
			p++;
		}
		if (m != 3)
		{
			printf("滤波器核 %d 设置值 数量大于 3 / %d\r\n", i, m);
			return -1;
		}
	}

	for (i = 0; i < n; i++)
	{
		printf("%d :\r\n", i);
		for (m = 0; m < 3; m++) {
			printf("\t%d : %s\r\n", m, values[i][m]);
		}
		printf("\r\n");
	}

	//for(i = 0; i < MAX_FILTER_NUMBER; i++)FilterInfos[i].Enable = false;

	i = 0;
	rootFilterInfo->CoreLength = atoi(values[i][0]);
	rootFilterInfo->IterateLevel = atoi(values[i][1]);
	PFILTERINFO prevFilterInfo = NULL;
	for (i = 1; i < n; i++)
	{
		PFILTERINFO pFilterInfo = new FILTERINFO;
		pFilterInfo->subFilteindex = i - 1;
		pFilterInfo->nextFilter = NULL;
		pFilterInfo->SampleRate = clsWaveData.AdcSampleRate;
		pFilterInfo->decimationFactor = rootFilterInfo->decimationFactor;

		//if (i == 1)
		{
			if(i == 1) rootFilterInfo->nextFilter = pFilterInfo;
			else prevFilterInfo->nextFilter = pFilterInfo;

			pFilterInfo->FreqCenter = atoi(values[i][1]);
			pFilterInfo->BandWidth = atoi(values[i][2]);
			pFilterInfo->CoreLength = rootFilterInfo->CoreLength;

			switch (atoi(values[i][0])) {
			case 0:
				pFilterInfo->Type = FilterType::FilterLowPass;
				break;
			case 1:
				pFilterInfo->Type = FilterType::FilterHighPass;
				break;
			case 2:
				pFilterInfo->Type = FilterType::FilterBandPass;
				break;
			case 3:
				pFilterInfo->Type = FilterType::FilterBandStop;
				break;
			default:
				pFilterInfo->Type = FilterType::FilterLowPass;
				break;
			}
			/*
			if (atoi(values[i][0]) == 0) {
				pFilterInfo->Type = FilterType::FilterLowPass;
			}
			else if (atoi(values[i][0]) == 2) {
				pFilterInfo->Type = FilterType::FilterHighPass;
			//	break;
			}else if (atoi(values[i][0]) == 1) {
				pFilterInfo->Type = FilterType::FilterBandStop;
			//	break;
			}
			*/
		}
		/*
		else {
			prevFilterInfo->nextFilter = pFilterInfo;
			pFilterInfo->Type = FilterType::FilterBandPass;
			int f0 = atoi(values[i - 1][1]);
			int w0 = atoi(values[i - 1][2]);
			int f1 = atoi(values[i][1]);
			int w1 = atoi(values[i][2]);
			pFilterInfo->BandWidth = (f1 - w1 / 2) - (f0 + w0 / 2);
			pFilterInfo->FreqCenter = (f0 + w0 / 2) + pFilterInfo->BandWidth / 2;
			pFilterInfo->CoreLength = rootFilterInfo->CoreLength;
			if (atoi(values[i][0]) == 2) {
				pFilterInfo->Type = FilterType::FilterHighPass;
				pFilterInfo->FreqCenter = atoi(values[i][1]);
				pFilterInfo->BandWidth = atoi(values[i][2]);
				pFilterInfo->CoreLength = rootFilterInfo->CoreLength;
				break;
			}
		}
		*/
		prevFilterInfo = pFilterInfo;
	}
	rootFilterInfo->subFilterNum = i - 1;

	WaitForSingleObject(hCoreMutex, INFINITE);

	build_desc_core(rootFilterInfo);

	build_last_iterate_core(rootFilterInfo);

	Cuda_ReInitFilterCore(rootFilterInfo);

	ReleaseMutex(hCoreMutex);

	//build_iterate_core();

}