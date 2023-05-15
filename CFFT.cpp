
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <fstream>
#include <string>

#include <limits>

#define _USE_MATH_DEFINES
#include <math.h>

#include "public.h"
#include "Debug.h"
#include "CData.h"
#include "CFFT.h"
#include "CWinFFT.h"

using namespace std;
using namespace WINS;
using namespace METHOD;

CFFT::CFFT(const char *flag, FFT_INFO* fftInfo)
{
	OPENCONSOLE_SAVED;
	
	hMutexBuff = CreateMutex(NULL, false, "CFFThMutexBuff");
	hMutexDraw = CreateMutex(NULL, false, "CFFThMutexDraw");
	
	Flag = flag;
	FFTInfo = fftInfo;

	RestoreValue();
}

CFFT::~CFFT()
{
	FFTDoing = false;
	while (Thread_Exit == false);
	UnInit();
}

void CFFT::Init(UINT fftsize, UINT fftstep, UINT averagedeep)
{
	WaitForSingleObject(hMutexBuff, INFINITE);
	WaitForSingleObject(hMutexDraw, INFINITE);

	FFTInfo->AverageDeep = averagedeep;
	FFTInfo->FFTSize = fftsize;
	FFTInfo->HalfFFTSize = fftsize / 2;
	FFTInfo->FFTStep = fftstep;

	if (FFT_Process_CallBackInit != NULL)FFT_Process_CallBackInit(this);

	cuda_FFT_Init(Data);

	if (FFT_src != NULL) delete[] FFT_src;
	FFT_src = new double[FFTInfo->FFTSize];

	if (FFT_src_com != NULL) delete[] FFT_src_com;
	FFT_src_com = new Complex[FFTInfo->FFTSize];

	FFTInfo->AverageDeepNum = 0;
	int memsize = (FFTInfo->HalfFFTSize + 2) * (FFTInfo->AverageDeep + 4);
	if (FFTBuff) delete[] FFTBuff;
	FFTBuff = new double[memsize];
	memset(FFTBuff, 0, memsize * sizeof(double));
	FFTOutBuff = &FFTBuff[(FFTInfo->HalfFFTSize + 2) * FFTInfo->AverageDeep];
	FFTOutLogBuff = &FFTBuff[(FFTInfo->HalfFFTSize + 2) * (FFTInfo->AverageDeep + 1)];
	FFTBrieflyBuff = &FFTBuff[(FFTInfo->HalfFFTSize + 2) * (FFTInfo->AverageDeep + 2)];
	FFTBrieflyLogBuff = &FFTBuff[(FFTInfo->HalfFFTSize + 2) * (FFTInfo->AverageDeep + 3)];

	ReleaseMutex(hMutexBuff);
	ReleaseMutex(hMutexDraw);
}

void CFFT::UnInit(void)
{

}

void CFFT::Add_Complex(Complex* src1, Complex* src2, Complex* dst)
{
	dst->imagin = src1->imagin + src2->imagin;
	dst->real = src1->real + src2->real;
}

void CFFT::Sub_Complex(Complex* src1, Complex* src2, Complex* dst)
{
	dst->imagin = src1->imagin - src2->imagin;
	dst->real = src1->real - src2->real;
}

void CFFT::Multy_Complex(Complex* src1, Complex* src2, Complex* dst)
{
	double r1 = 0.0, r2 = 0.0;
	double i1 = 0.0, i2 = 0.0;
	r1 = src1->real;
	r2 = src2->real;
	i1 = src1->imagin;
	i2 = src2->imagin;
	dst->imagin = r1 * i2 + r2 * i1;
	dst->real = r1 * r2 - i1 * i2;
}
////////////////////////////////////////////////////////////////////
//在FFT中有一个WN的n次方项，在迭代中会不断用到，具体见算法说明
///////////////////////////////////////////////////////////////////
void CFFT::getWN(double n, double size_n, Complex* dst)
{
	double x = 2.0 * M_PI * n / size_n;
	dst->imagin = -sin(x);
	dst->real = cos(x);
}

////////////////////////////////////////////////////////////////////
//定义DFT函数，其原理为简单的DFT定义，时间复杂度O（n^2）,
//下面函数中有两层循环，每层循环的step为1，size为n，故为O（n*n）,
//注释掉的显示部分为数据显示，可以观察结果
///////////////////////////////////////////////////////////////////
void CFFT::DFT(double* src, Complex* dst, int size)
{
	clock_t start, end;
	start = clock();
	double XX = M_PI * 2 / size;
	for (int m = 0; m < size; m++) {
		double real = 0.0;
		double imagin = 0.0;
		double X = XX * m;
		for (int n = 0; n < size; n++) {
			/*
						double x=M_PI*2*m*n;
						real+=src[n]*cos(x/size);
						imagin+=src[n]*(-sin(x/size));
			*/
			//double x=M_PI*2*m*n;
			real += src[n] * 1.0;//cos(n*X);
			imagin += src[n] * 1.0;//(-sin(x*X));

		}
		dst[m].imagin = imagin;
		dst[m].real = real;
		/* if(imagin>=0.0)
			 DbgMsg("%lf+%lfj\n",real,imagin);
		 else
			 DbgMsg("%lf%lfj\n",real,imagin);*/
	}
	end = clock();
	DbgMsg("DFT use time :%lfs for Datasize of:%d\n", (double)(end - start) / CLOCKS_PER_SEC, size);

}
////////////////////////////////////////////////////////////////////
//定义IDFT函数，其原理为简单的IDFT定义，时间复杂度O（n^2）,
//下面函数中有两层循环，每层循环的step为1，size为n，故为O（n*n）,
///////////////////////////////////////////////////////////////////
void CFFT::IDFT(Complex* src, Complex* dst, int size)
{
	clock_t start, end;
	start = clock();
	for (int m = 0; m < size; m++) {
		double real = 0.0;
		double imagin = 0.0;
		for (int n = 0; n < size; n++) {
			double x = M_PI * 2 * m * n / size;
			real += src[n].real * cos(x) - src[n].imagin * sin(x);
			imagin += src[n].real * sin(x) + src[n].imagin * cos(x);

		}
		real /= size;
		imagin /= size;
		if (dst != NULL) {
			dst[m].real = real;
			dst[m].imagin = imagin;
		}
		/*
		if(imagin>=0.0)
			DbgMsg("%lf+%lfj\n",real,imagin);
		else
			DbgMsg("%lf%lfj\n",real,imagin);
		*/
	}
	end = clock();
	DbgMsg("IDFT use time :%lfs for Datasize of:%d\n", (double)(end - start) / CLOCKS_PER_SEC, size);


}
////////////////////////////////////////////////////////////////////
//定义FFT的初始化数据，因为FFT的数据经过重新映射，递归结构
///////////////////////////////////////////////////////////////////
int CFFT::FFT_remap(double* src, int size_n)
{

	if (size_n == 1)
		return 0;
	double* temp = (double*)malloc(sizeof(double) * size_n);
	//double* temp =  new double(size_n);
	for (int i = 0; i < size_n; i++)
		if (i % 2 == 0)
			temp[i / 2] = src[i];
		else
			temp[(size_n + i) / 2] = src[i];
	for (int i = 0; i < size_n; i++)
		src[i] = temp[i];
	free(temp);
	//delete[] temp;
	FFT_remap(src, size_n / 2);
	FFT_remap(src + size_n / 2, size_n / 2);
	return 1;

}


void CFFT::NormalFFT(void* Buff, BUFF_DATA_TYPE type, uint32_t pos, UINT mask)
{
	int i;
	memset(FFT_src, 0, FFTInfo->FFTSize * sizeof(double));
	switch (type)
	{
	case BUFF_DATA_TYPE::char_type:
	{
		char* buff = (char*)Buff;
		for (i = 0; i < FFTInfo->FFTStep; i++, pos++) {
			FFT_src[i] = buff[pos & mask];
		}
	}
	break;
	case BUFF_DATA_TYPE::short_type:
	{
		short* buff = (short*)Buff;
		for (i = 0; i < FFTInfo->FFTStep; i++, pos++) {
			FFT_src[i] = buff[pos & mask];
		}
	}
	break;
	}


	FFT_remap(FFT_src, FFTInfo->FFTSize);
	// for(int i=0;i<size_n;i++)
	 //    DbgMsg("%lf\n",src[i]);

	int k = FFTInfo->FFTSize;
	int z = 0;
	while (k /= 2) {
		z++;
	}
	k = z;
	if (FFTInfo->FFTSize != (1 << k))
	{
		DbgMsg("file: %s. line: %d. func: %s\r\n", __FILE__, __LINE__, __FUNCTION__);
		EXIT(0);
	}
	//Complex * src_com=(Complex*)malloc(sizeof(Complex)*size_n);
	//if(src_com==NULL)EXIT(0);
	for (int i = 0; i < FFTInfo->FFTSize; i++) {
		FFT_src_com[i].real = FFT_src[i];
		FFT_src_com[i].imagin = 0.0;
	}
	for (int i = 0; i < k; i++) {
		z = 0;
		for (int j = 0; j < FFTInfo->FFTSize; j++) {
			if ((j / (1 << i)) % 2 == 1) {
				Complex wn;
				getWN(z, FFTInfo->FFTSize, &wn);
				Multy_Complex(&FFT_src_com[j], &wn, &FFT_src_com[j]);
				z += 1 << (k - i - 1);
				Complex temp;
				int neighbour = j - (1 << (i));
				temp.real = FFT_src_com[neighbour].real;
				temp.imagin = FFT_src_com[neighbour].imagin;
				Add_Complex(&temp, &FFT_src_com[j], &FFT_src_com[neighbour]);
				Sub_Complex(&temp, &FFT_src_com[j], &FFT_src_com[j]);
			}
			else
				z = 0;
		}
	}
}

////////////////////////////////////////////////////////////////////
//定义FFT，具体见算法说明，注释掉的显示部分为数据显示，可以观察结果
///////////////////////////////////////////////////////////////////
void CFFT::FFT(UINT pos)
{
	int i;

	clock_t start, end;
	start = clock();
	WaitForSingleObject(hMutexBuff, INFINITE);

	cuda_FFT(pos);

	//NormalFFT(Buff, type, pos, mask);

	double maxfftv = -1 * DBL_MAX;
	double minfftv = DBL_MAX;// numeric_limits<double>::max();
	double maxfftvlog = -1 * DBL_MAX;
	double minfftvlog = DBL_MAX;// numeric_limits<double>::max();
	double d;
	double* addbuf;
	double* subbuf;
	double scale = FFTInfo->FFTSize / FFTInfo->FFTStep;

	WaitForSingleObject(hMutexDraw, INFINITE);

	addbuf = &FFTBuff[FFTInfo->AverageDeepNum * (FFTInfo->HalfFFTSize + 2)];
	FFTInfo->AverageDeepNum++;
	if (FFTInfo->AverageDeepNum == FFTInfo->AverageDeep) FFTInfo->AverageDeepNum = 0;
	subbuf = &FFTBuff[FFTInfo->AverageDeepNum * (FFTInfo->HalfFFTSize + 2)];
	for (i = 0; i < FFTInfo->HalfFFTSize; i++)
	{
		//d = scale * sqrt(FFT_src_com[i].real * FFT_src_com[i].real + FFT_src_com[i].imagin * FFT_src_com[i].imagin);
		d = scale * sqrt(
			cuda_FFT_CompoData[i].x * cuda_FFT_CompoData[i].x + 
			cuda_FFT_CompoData[i].y * cuda_FFT_CompoData[i].y
		);
		addbuf[i] = d / FFTInfo->AverageDeep;
		FFTOutBuff[i] += addbuf[i];
		FFTOutBuff[i] -= subbuf[i];

		FFTOutLogBuff[i] = log10(FFTOutBuff[i] / FFTMaxValue);

		if (i && maxfftv < d)maxfftv = d;
		if (i && minfftv > d)minfftv = d;
	}
	addbuf[i] = maxfftv / FFTInfo->AverageDeep;
	FFTOutBuff[i] += addbuf[i];
	FFTOutBuff[i] -= subbuf[i];
	FFTOutLogBuff[i] = log10(FFTOutBuff[i] / FFTMaxValue);
	i++;
	addbuf[i] = minfftv / FFTInfo->AverageDeep;
	FFTOutBuff[i] += addbuf[i];
	FFTOutBuff[i] -= subbuf[i];
	FFTOutLogBuff[i] = log10(FFTOutBuff[i] / FFTMaxValue);
	
	if (FFT_Process_CallBack != NULL)FFT_Process_CallBack(this);

	ReleaseMutex(hMutexDraw);

	FFTInfo->FFTCount++;
	FFTInfo->FFTNew = true;

	if(hWnd != NULL) PostMessage(hWnd, WM_FFT, 0, (LPARAM)this);

	ReleaseMutex(hMutexBuff);

	end = clock();
	static int TimeDelay = 0;
	if (TimeDelay++ == 100)
	{
		TimeDelay = 0;
		//DbgMsg("CFFT use time :%lfs for Datasize of:%d\n", (double)(end - start) / CLOCKS_PER_SEC, FFTSize);
	}
}

LPTHREAD_START_ROUTINE CFFT::FFT_Thread(LPVOID lp)
{
	OPENCONSOLE_SAVED;

	CFFT* fft = (CFFT*)lp;
	fft->FFT_Thread_func();

	//CLOSECONSOLE;
	return 0;
}

void CFFT::FFT_Thread_func(void)
{
	UINT fft_pos;
	UINT fft_between;
	FFTDoing = true;
	Thread_Exit = false;

	while (FFTDoing && Program_In_Process) {

		if (FFTNext == false) {	Sleep(10); continue; }
		fft_pos = Data->Pos;
		fft_between = (fft_pos - FFTInfo->FFTPos) & Data->Mask;
		if (fft_between > FFTInfo->FFTStep) {
			FFT(fft_pos - FFTInfo->FFTStep);
			FFTInfo->FFTPos = fft_pos;
			FFTNext = false;
		}
		else {
			Sleep(10);
		}

	}

	cuda_FFT_UnInit();

	SaveValue();

	Thread_Exit = true;
	hThread = NULL;
}

double CFFT::GetFFTMaxValue(void)
{
	double i;
	double sumRe = 0.0;
	double sumIm = 0.0;
	UINT64 max = ((UINT64)1 << (ADC_DATA_SAMPLE_BIT -1)) - 1;
	for (i = 0; i < FFTInfo->FFTSize; i++) {
		sumRe += (double)max * cos(2 * M_PI * i / FFTInfo->FFTSize) * sin(2 * M_PI * i / FFTInfo->FFTSize);
		sumIm += (double)max * sin(2 * M_PI * i / FFTInfo->FFTSize) * sin(2 * M_PI * i / FFTInfo->FFTSize);
	}
	double d = sqrt(sumRe * sumRe + sumIm * sumIm);
	DbgMsg("maxvalue:%d, %lf\n", max, d);
	return d/16;
}


void CFFT::SaveValue(void)
{
	const UINT VALUE_LENGTH = 100;
	char section[VALUE_LENGTH];
	sprintf(section, "CFFT_%s", Flag);
	WritePrivateProfileString(section, "FFTSize", std::to_string(FFTInfo->FFTSize).c_str(), IniFilePath);
	WritePrivateProfileString(section, "FFTStep", std::to_string(FFTInfo->FFTStep).c_str(), IniFilePath);
	WritePrivateProfileString(section, "AverageDeep", std::to_string(FFTInfo->AverageDeep).c_str(), IniFilePath);
}

void CFFT::RestoreValue(void)
{
	const UINT VALUE_LENGTH = 100;
	char section[VALUE_LENGTH];
	sprintf(section, "CFFT_%s", Flag);
	char value[VALUE_LENGTH];
	GetPrivateProfileString(section, "FFTSize", "1024", value, VALUE_LENGTH, IniFilePath);
	FFTInfo->FFTSize = atoi(value);
	FFTInfo->HalfFFTSize = FFTInfo->FFTSize / 2;
	GetPrivateProfileString(section, "FFTStep", "1024", value, VALUE_LENGTH, IniFilePath);
	FFTInfo->FFTStep = atoi(value);
	GetPrivateProfileString(section, "AverageDeep", "16", value, VALUE_LENGTH, IniFilePath);
	FFTInfo->AverageDeep = atoi(value);
}
