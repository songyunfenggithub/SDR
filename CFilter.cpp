#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>

#include "public.h"
#include "Debug.h"
#include "CDataFromTcpIp.h"
#include "CData.h"
#include "CFilter.h"
#include "CWinFilter.h"
#include "CWinMain.h"

#include "cuda_CFilter.cuh"
#include "cuda_CFilter2.cuh"
#include "cuda_CFilter3.cuh"

using namespace std;
using namespace WINS; 
using namespace METHOD;

CFilter clsMainFilterI("MainI");
CFilter clsMainFilterQ("MainQ");
CFilter clsAudioFilter("Audio");

CFilter::CFilter(const char* tag)
{
	OPENCONSOLE_SAVED;

	TAG = tag;

	hFilterMutex = CreateMutex(NULL, false, "hFilterMutex");		
	hCoreMutex = CreateMutex(NULL, false, "hCoreMutex");			

	RestoreValue();
}

CFilter::~CFilter()
{
	//CLOSECONSOLE;
}

void CFilter::set_cudaFilter(cuda_CFilter* f, cuda_CFilter2* f2, cuda_CFilter3* f3, UINT filterSrcLen)
{
	FilterSrcLen = filterSrcLen;
	f->SrcLen = filterSrcLen;
	cudaFilter = f;
	if (f2 != NULL)f2->SrcLen = filterSrcLen;
	cudaFilter2 = f2;
	if (f3 != NULL)f3->SrcLen = filterSrcLen;
	cudaFilter3 = f3;
}

void CFilter::setFilterCoreDesc(PFILTER_INFO pFilterInfo, char* CoreDescStr)
{
	if (pFilterInfo->CoreDescStr != NULL) delete[] pFilterInfo->CoreDescStr;
	pFilterInfo->CoreDescStr = new char[strlen(CoreDescStr)+1];
	sprintf(pFilterInfo->CoreDescStr, "%s", CoreDescStr);
}

void CFilter::build_desc_core(PFILTER_INFO rootFilterInfo)
{
	int i;
	PFILTER_INFO pFilterInfo = rootFilterInfo->nextFilter;
	while(pFilterInfo != NULL)
	{
		if(rootFilterInfo->CoreLength < pFilterInfo->CoreLength) rootFilterInfo->CoreLength = pFilterInfo->CoreLength;
		pFilterInfo = pFilterInfo->nextFilter;
	}

	if (rootFilterInfo->FilterCore != NULL) {delete[] rootFilterInfo->FilterCore; rootFilterInfo->FilterCore = NULL;}
	rootFilterInfo->FilterCore = new FILTER_CORE_DATA_TYPE[rootFilterInfo->CoreLength];
	memset(rootFilterInfo->FilterCore, 0, rootFilterInfo->CoreLength * sizeof(FILTER_CORE_DATA_TYPE));

	pFilterInfo = rootFilterInfo->nextFilter;
	while (pFilterInfo != NULL)
	{
		core(pFilterInfo, rootFilterInfo);
		for (i = 0; i < pFilterInfo->CoreLength; i++)
		{
			rootFilterInfo->FilterCore[i] += pFilterInfo->FilterCore[i];
		}
		pFilterInfo = pFilterInfo->nextFilter;
	}
	//write_core(rootFilterInfo);
}

void CFilter::build_last_iterate_core(PFILTER_INFO rootFilterInfo)
{
	if (rootFilterInfo->IterateLevel < 2) return;

	int tFilterLength, tOrignalFilterLength;
	int i;
	FILTER_CORE_DATA_TYPE* tFilterCore = NULL;
	FILTER_CORE_DATA_TYPE* tOrignalFilterCore = new FILTER_CORE_DATA_TYPE[rootFilterInfo->CoreLength];
	memcpy(tOrignalFilterCore, rootFilterInfo->FilterCore, rootFilterInfo->CoreLength * sizeof(FILTER_CORE_DATA_TYPE));
	tOrignalFilterLength = rootFilterInfo->CoreLength;
	for (i = 1; i < rootFilterInfo->IterateLevel; i++)
	{
		tFilterLength = rootFilterInfo->CoreLength;
		if (tFilterCore != NULL)delete[] tFilterCore;
		tFilterCore = new FILTER_CORE_DATA_TYPE[tFilterLength];
		memcpy(tFilterCore, rootFilterInfo->FilterCore, tFilterLength * sizeof(FILTER_CORE_DATA_TYPE));
		rootFilterInfo->CoreLength = tFilterLength + tOrignalFilterLength - 1;
		delete[] rootFilterInfo->FilterCore;
		rootFilterInfo->FilterCore = new FILTER_CORE_DATA_TYPE[rootFilterInfo->CoreLength];
		memset(rootFilterInfo->FilterCore, 0, rootFilterInfo->CoreLength * sizeof(FILTER_CORE_DATA_TYPE));
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

void CFilter::build_iterate_core(void)
{
	/*
	int tFilterLength = 0;
	int i;
	PFILTER_INFO pFilterInfo1 = NULL, pFilterInfo2 = NULL;
	FILTER_CORE_DATA_TYPE* tFilterCore, *CoreMatrix;
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
					FilterCore = new FILTER_CORE_DATA_TYPE[FilterCoreLength];
					memcpy(FilterCore, pFilterInfo1->FilterCore, FilterCoreLength * sizeof(FILTER_CORE_DATA_TYPE));
				}

				int i1, i2;
				FILTER_CORE_DATA_TYPE*tFilterCore = FilterCore;
				int tFilterCoreLength = FilterCoreLength + pFilterInfo2->CoreLength - 1;
				FilterCore = new FILTER_CORE_DATA_TYPE[tFilterCoreLength]{ 0 };
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

void CFilter::invcore(FILTER_CORE_DATA_TYPE* pBuf, UINT corelen)
{
	UINT16 i;
	for (i = 0; i < corelen; i++) pBuf[i] *= -1.0;
	pBuf[(corelen - 1) / 2] += 1.0;
	//*(double*)(pBuf + (pFilterInfo->CoreLength-1)/2*4) += 1000.0;
}

void CFilter::lowcore(float fc, FILTER_CORE_DATA_TYPE* pBuf, UINT corelen)
{
	UINT16 K = 1;
	UINT16 i;
	FILTER_CORE_DATA_TYPE* p;
	FILTER_CORE_DATA_TYPE sum;
	UINT16 M = corelen - 1;
	p = pBuf;
	//(0.42 - 0.5*cos(2*M_PI*i/M) + 0.08*cos(4*M_PI*i/M)) ����������
	//������
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

void CFilter::corepluse(FILTER_CORE_DATA_TYPE* pR, FILTER_CORE_DATA_TYPE* pS, UINT corelen)
{
	//  double *pR = FilterCore;
	//  double *pS = FilterCore_2;
	UINT16 i;
	for (i = 0; i < corelen; i++) *pR++ += *pS++;
}

/*
type: 0��ͨ 1��ͨ 2��ͨ 3���
freq0�� ����Ƶ��
freq_width�� ͨ��/��� Ƶ��
*/
void CFilter::core(FILTER_INFO* pFilterInfo, FILTER_INFO* rootf)
{
	FILTER_CORE_DATA_TYPE fl = pFilterInfo->FreqCenter - pFilterInfo->BandWidth / 2;
	FILTER_CORE_DATA_TYPE fh = pFilterInfo->FreqCenter + pFilterInfo->BandWidth / 2;
	UINT32 fs = rootf->SampleRate;
	UINT32 CoreLength = pFilterInfo->CoreLength;

	pFilterInfo->FreqFallWidth = 4.0 / (CoreLength - 1) * fs;
	if (pFilterInfo->FilterCore != NULL) delete[] pFilterInfo->FilterCore;
	pFilterInfo->FilterCore = new FILTER_CORE_DATA_TYPE[CoreLength];
	FILTER_CORE_DATA_TYPE* FilterCore = pFilterInfo->FilterCore;

	FILTER_CORE_DATA_TYPE* TempBuf = new FILTER_CORE_DATA_TYPE[CoreLength];

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

void CFilter::ReBuildFilterCore(void)
{
	ParseCoreDesc();
	//clsFilter.FilterCoreAnalyse(clsWinMain.m_FilterWin, &rootFilterInfo);
}

void CFilter::write_core(PFILTER_INFO rootFilterInfo)
{
	ofstream outfile;
	string t1;
	string t2;
	char str[1024];
	PFILTER_INFO pFilterInfo;
	outfile.open("core.txt");
	if (!outfile.is_open())	cout << "open file core.txt failure" << endl;

	outfile << "# �˲����ں� " << endl;
	outfile << "#   ����Ƶ��   " << SrcData->SampleRate << endl;

	pFilterInfo = rootFilterInfo->nextFilter;

	int n = 0;
	while (pFilterInfo != NULL)
	{
		outfile << "# �˲����ں� " << n++ << endl;
		outfile << "#   �˲������� " << pFilterInfo->Type << endl;
		outfile << "#   �ں˳���   " << pFilterInfo->CoreLength << endl;
		outfile << "#   ����Ƶ��   " << pFilterInfo->FreqCenter << endl;
		outfile << "#   ͨ/�����  " << pFilterInfo->BandWidth << endl;
		outfile << "#   �½�����   " << pFilterInfo->FreqFallWidth << endl;
		for (int i = 0; i < pFilterInfo->CoreLength; i++)
		{
			sprintf(str, "%d,%.10lf", i, pFilterInfo->FilterCore[i]);
			outfile << str << endl;
		}
		pFilterInfo = pFilterInfo->nextFilter;
	}

	outfile << "# �˲����ں�." << rootFilterInfo->subFilterNum << endl;
	outfile << "#   �˲����ں˳��� " << rootFilterInfo->CoreLength << endl;
	outfile << "#   ����Ƶ��       " << SrcData->SampleRate << endl;

	for (int i = 0; i < rootFilterInfo->CoreLength; i++)
	{
		sprintf(str, "%d,%.10lf", i, rootFilterInfo->FilterCore[i]);
		outfile << str << endl;
	}

	outfile.close();
}

LPTHREAD_START_ROUTINE CFilter::filter_thread1(LPVOID lp)
{
	((CFilter*)lp)->filter_func_short(0);
	return 0;
}

LPTHREAD_START_ROUTINE CFilter::filter_thread2(LPVOID lp)
{
	((CFilter*)lp)->filter_func_short(1);
	return 0;
}

LPTHREAD_START_ROUTINE CFilter::filter_thread3(LPVOID lp)
{
	((CFilter*)lp)->filter_func_short(2);
	return 0;
}

LPTHREAD_START_ROUTINE CFilter::filter_thread4(LPVOID lp)
{
	((CFilter*)lp)->filter_func_short(3);
	return 0;
}

void CFilter::filter_func_short(UINT32 thread_id)
{
	short* pbuf1, * pend1, * pbuf2, * pend2;
	FILTER_CORE_DATA_TYPE* pcore = NULL;
	FILTER_CORE_DATA_TYPE* pc = NULL;
	FILTER_CORE_DATA_TYPE* psave = NULL;
	FILTER_CORE_DATA_TYPE r;
	bool iswork;
	UINT32 work_pos = 0;
	
	char* FilttedFlag = new char(SrcData->Len);
	memset(FilttedFlag, 0, SrcData->Len);

	while (Program_In_Process == true)
	{
		iswork = FALSE;
		WaitForSingleObject(hFilterMutex, INFINITE);
		if (SrcData->ProcessPos != SrcData->Pos)
		{
			work_pos = SrcData->ProcessPos;
			iswork = TRUE;
			SrcData->ProcessPos++;
			if (SrcData->ProcessPos == SrcData->Len)SrcData->ProcessPos = 0;
		}
		while (FilttedFlag[TargetData->Pos])
		{
			FilttedFlag[TargetData->Pos] = 0;
			TargetData->Pos++;
			if (TargetData->Pos == TargetData->Len)TargetData->Pos = 0;
			//TargetData->Pos = TargetData->Pos;
		}
		/*
		if (((SrcData->FilttedPos - SrcData->FilttedForwardPos) & DATA_BUFFER_MASK) > FILTED_FORWORD_PAGE_SIZE) {
			clsGetDataTcpIp.SendFiltedData((char*)(SrcData->FilttedBuff + SrcData->FilttedForwardPos), FILTED_FORWORD_PAGE_SIZE * 8);
			SrcData->FilttedForwardPos = SrcData->FilttedForwardPos + FILTED_FORWORD_PAGE_SIZE;
			if (SrcData->FilttedForwardPos == DATA_BUFFER_LENGTH)SrcData->FilttedForwardPos = 0;
		}
		*/
		ReleaseMutex(hFilterMutex);

		if (iswork)
		{
			UINT32 w_pos = work_pos + 1;
			WaitForSingleObject(hCoreMutex, INFINITE);
			if (w_pos < rootFilterInfo1.CoreLength)
			{
				pbuf1 = (short*)SrcData->Buff + SrcData->Len - (rootFilterInfo1.CoreLength - w_pos) ;
				pend1 = (short*)SrcData->Buff + SrcData->Len;
				pbuf2 = (short*)SrcData->Buff;
				pend2 = (short*)SrcData->Buff + w_pos;
			}
			else
			{
				pbuf1 = (short*)SrcData->Buff + (w_pos - rootFilterInfo1.CoreLength);
				pend1 = (short*)SrcData->Buff + w_pos;
				pbuf2 = pend2 = 0;
			}
			if(psave != rootFilterInfo1.FilterCore){
				psave = rootFilterInfo1.FilterCore;
				if (pcore != NULL) delete[] pcore;
				pcore = new FILTER_CORE_DATA_TYPE[rootFilterInfo1.CoreLength];
				memcpy(pcore, rootFilterInfo1.FilterCore, sizeof(FILTER_CORE_DATA_TYPE) * rootFilterInfo1.CoreLength);
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
			((float*)TargetData->Buff)[work_pos] = r;
			//filter_ready_poss[thread_id] = work_pos;
			FilttedFlag[work_pos] = 1;
		} else Sleep(0);
	}
	delete[] FilttedFlag;
}

LPTHREAD_START_ROUTINE CFilter::cuda_filter_thread(LPVOID lp)
{
	((CFilter*)lp)->cuda_filter_func();
	return 0;
}

void CFilter::cuda_filter_func(void)
{
	doFiltting = true;
	Thread_Exit = false;
	UINT stepLen = FilterSrcLen >> 2;
	SrcData->ProcessPos = ((UINT)(SrcData->Pos / stepLen)) * stepLen;
	SrcData->ProcessPos &= SrcData->Mask;
	while (doFiltting == true && Program_In_Process == true) {
		WaitForSingleObject(hCoreMutex, INFINITE);
		if (Cuda_Filter_N_New != Cuda_Filter_N_Doing) {
			ReleaseMutex(hCoreMutex);
			Sleep(100);
			continue;
		}
		if (((SrcData->Pos - SrcData->ProcessPos) & SrcData->Mask) > stepLen)
		{
			switch (Cuda_Filter_N_Doing) {
			case cuda_filter_1:
				cudaFilter->Filtting();
				break;
			case cuda_filter_2:
				cudaFilter2->Filtting();
				break;
			case cuda_filter_3:
				cudaFilter3->Filtting();
				break;
			}
		}
		else Sleep(10);
		ReleaseMutex(hCoreMutex);
	}
	cudaFilter->UnInit();
	cudaFilter2->UnInit();
	cudaFilter3->UnInit();

	Thread_Exit = true;
	hThread = NULL;
}

bool CFilter::CheckCoreDesc(char* coreDesc)
{
	bool state = true;
	char* str = new char[strlen(coreDesc) + 1];
	sprintf(str, "%s", coreDesc);
	int i, m, n;
	i = 0;
	char* p = str;
	char* Filters[MAX_FILTER_NUMBER] = { 0 };
	char* values[MAX_FILTER_NUMBER][3] = { 0 };
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
		DbgMsg("�˲����������������ֵ %d / %d\r\n", n, MAX_FILTER_NUMBER);
		state = false;
		goto Exit;
	}
	if (n < 2)
	{
		DbgMsg("�˲���������������һ�� %d / (1 - %d)\r\n", n, MAX_FILTER_NUMBER);
		state = false;
		goto Exit;
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
			DbgMsg("�˲����� %d ����ֵ ����Ӧ�� = 3 : %d\r\n", i, m);
			state = false;
			goto Exit;
		}
	}

Exit:

	if (state == false) {
		DbgMsg("coreDesc: %s\r\n", coreDesc);
		for (i = 0; i < n; i++)
		{
			DbgMsg("%d :\r\n", i);
			for (m = 0; m < 3; m++) {
				DbgMsg("\t%d : %s\r\n", m, values[i][m]);
			}
			DbgMsg("\r\n");
		}
	}

	delete[] str;
	return state;
}

void CFilter::ParseCoreDesc(void)
{
	WaitForSingleObject(hCoreMutex, INFINITE);
	
	cudaFilter->UnInit();
	cudaFilter2->UnInit();
	cudaFilter3->UnInit();

	ParseOneCore(&rootFilterInfo1);
	switch (Cuda_Filter_N_New) {
	case cuda_filter_1:
		cudaFilter->Init(this);
		TargetData->Pos = SrcData->ProcessPos;
		break;
	case cuda_filter_2:
		cudaFilter2->Init(this);
		TargetData->Pos = SrcData->ProcessPos >> rootFilterInfo1.decimationFactorBit;
		break;
	case cuda_filter_3:
		ParseOneCore(&rootFilterInfo2);
		cudaFilter3->Init(this);
		TargetData->Pos = SrcData->ProcessPos >> (rootFilterInfo1.decimationFactorBit + rootFilterInfo2.decimationFactorBit);
		break;
	}
	Cuda_Filter_N_Doing = Cuda_Filter_N_New;
	ReleaseMutex(hCoreMutex);
}

int CFilter::ParseOneCore(FILTER_INFO* rootFilterInfo)
{
	TargetData->SampleRate = SrcData->SampleRate / (1 << rootFilterInfo->decimationFactorBit);
	
	if (rootFilterInfo == &rootFilterInfo1) 
		rootFilterInfo1.SampleRate = TargetData->SampleRate;

	if(Cuda_Filter_N_Doing == cuda_filter_3) 
		TargetData->SampleRate = SrcData->SampleRate / (1 << (rootFilterInfo1.decimationFactorBit + rootFilterInfo2.decimationFactorBit));

	if (rootFilterInfo == &rootFilterInfo2)
		rootFilterInfo2.SampleRate = TargetData->SampleRate;

	if (rootFilterInfo->FilterCore != NULL) delete[] rootFilterInfo->FilterCore;
	rootFilterInfo->FilterCore = NULL;
	PFILTER_INFO tt, pp = rootFilterInfo->nextFilter;
	while (pp != NULL) {
		if (pp->CoreDescStr != NULL) delete[] pp->CoreDescStr;
		if (pp->FilterCore != NULL) delete[] pp->FilterCore;
		tt = pp;
		pp = pp->nextFilter;
		delete[] tt;
	}
	rootFilterInfo->nextFilter = NULL;
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
		DbgMsg("�˲����������������ֵ %d / %d\r\n", n, MAX_FILTER_NUMBER);
		return -1;
	}
	if (n < 2)
	{
		DbgMsg("�˲���������������һ�� %d / (1 - %d)\r\n", n, MAX_FILTER_NUMBER);
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
			DbgMsg("�˲����� %d ����ֵ �������� 3 / %d\r\n", i, m);
			return -1;
		}
	}

	for (i = 0; i < n; i++)
	{
		DbgMsg("%d :\r\n", i);
		for (m = 0; m < 3; m++) {
			DbgMsg("\t%d : %s\r\n", m, values[i][m]);
		}
		DbgMsg("\r\n");
	}


	i = 0;
	rootFilterInfo->CoreLength = atoi(values[i][0]);
	rootFilterInfo->IterateLevel = atoi(values[i][1]);
	PFILTER_INFO prevFilterInfo = NULL;
	for (i = 1; i < n; i++)
	{
		PFILTER_INFO pFilterInfo = new FILTER_INFO;
		pFilterInfo->subFilteindex = i - 1;
		pFilterInfo->nextFilter = NULL;
		pFilterInfo->decimationFactorBit = rootFilterInfo->decimationFactorBit;
		{
			if(i == 1) rootFilterInfo->nextFilter = pFilterInfo;
			else prevFilterInfo->nextFilter = pFilterInfo;

			pFilterInfo->FreqCenter = atoi(values[i][1]);
			pFilterInfo->BandWidth = atoi(values[i][2]);
			pFilterInfo->CoreLength = rootFilterInfo->CoreLength;

			switch (atoi(values[i][0])) {
			case 0:
				pFilterInfo->Type = FILTER_TYPE::FilterLowPass;
				break;
			case 1:
				pFilterInfo->Type = FILTER_TYPE::FilterHighPass;
				break;
			case 2:
				pFilterInfo->Type = FILTER_TYPE::FilterBandPass;
				break;
			case 3:
				pFilterInfo->Type = FILTER_TYPE::FilterBandStop;
				break;
			default:
				pFilterInfo->Type = FILTER_TYPE::FilterLowPass;
				break;
			}
		}
		prevFilterInfo = pFilterInfo;
	}
	rootFilterInfo->subFilterNum = i - 1;

	build_desc_core(rootFilterInfo);

	build_last_iterate_core(rootFilterInfo);

	//doFiltting = false;
	//while (cuda_Filter_exit == false) {
	//	Sleep(1000);
	//}
}


void CFilter::SaveValue(void)
{
#define VALUE_LENGTH	100
	char section[VALUE_LENGTH];
	sprintf(section, "CFilter_%s", TAG);
	char value[VALUE_LENGTH];
	
	WritePrivateProfileString(section, "FilterCoreDesc1", rootFilterInfo1.CoreDescStr, IniFilePath);
	WritePrivateProfileString(section, "decimationFactorBit1", std::to_string(rootFilterInfo1.decimationFactorBit).c_str(), IniFilePath);
	
	WritePrivateProfileString(section, "FilterCoreDesc2", rootFilterInfo2.CoreDescStr, IniFilePath);
	WritePrivateProfileString(section, "decimationFactorBit2", std::to_string(rootFilterInfo2.decimationFactorBit).c_str(), IniFilePath);
	
	WritePrivateProfileString(section, "Cuda_Filter_N", std::to_string(Cuda_Filter_N_Doing).c_str(), IniFilePath);

	DbgMsg("SaveValue %s\r\n", section);
}

void CFilter::RestoreValue(void)
{
#define VALUE_LENGTH	1024
	char value[VALUE_LENGTH];
	char section[VALUE_LENGTH];
	sprintf(section, "CFilter_%s", TAG);
	
	GetPrivateProfileString(section, "FilterCoreDesc1", "65, 0, 0; 0, 10000, 100", value, VALUE_LENGTH, IniFilePath);
	rootFilterInfo1.CoreDescStr = new char[strlen(value) + 1];
	strcpy(rootFilterInfo1.CoreDescStr, value);
	GetPrivateProfileString(section, "decimationFactorBit1", "0", value, VALUE_LENGTH, IniFilePath);
	rootFilterInfo1.decimationFactorBit = atoi(value);
	
	GetPrivateProfileString(section, "FilterCoreDesc2", "65, 0, 0; 0, 10000, 100", value, VALUE_LENGTH, IniFilePath);
	rootFilterInfo2.CoreDescStr = new char[strlen(value) + 1];
	strcpy(rootFilterInfo2.CoreDescStr, value);
	GetPrivateProfileString(section, "decimationFactorBit2", "0", value, VALUE_LENGTH, IniFilePath);
	rootFilterInfo2.decimationFactorBit = atoi(value);

	GetPrivateProfileString(section, "Cuda_Filter_N", "0", value, VALUE_LENGTH, IniFilePath);
	Cuda_Filter_N_Doing = Cuda_Filter_N_New = (CUDA_FILTER_N)atoi(value);
	
	DbgMsg("RestoreValue %s\r\n", section);
}
