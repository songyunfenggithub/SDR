
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
#include "CData.h"
#include "CFFT.h"
#include "CFFTWin.h"
#include "cuda_FFT.cuh"

using namespace std;

CFFT::CFFT()
{
	OPENCONSOLE;
	hMutexBuff = CreateMutex(NULL, false, "CFFThMutexBuff");
}

CFFT::~CFFT()
{
	FFTDoing = false;
	while (bFFT_Thread_Exitted == false);
	UnInit();
}

void CFFT::Init(CFFTWin* fftwin)
{
	WaitForSingleObject(hMutexBuff, INFINITE);

	this->fftWin = fftwin;

	FFTPos = 0;
	FFTSize = fftWin->FFTSize;
	FFTStep = fftWin->FFTStep;
	HalfFFTSize = FFTSize / 2;
	FFTCount = 0;
	FFTPerSec = 0.0;

	DataBuff = fftWin->DataBuff;
	DataBuffPos = fftWin->DataBuffPos;
	data_buff_data_bits = fftWin->data_buff_data_bits;
	data_buff_length_mask = fftWin->data_buff_length_mask;

	cuda_FFT_Init(FFTSize, FFTStep, data_buff_data_bits);

	average_Deep = fftWin->average_Deep;
	average_Deep_mask = average_Deep - 1;
	average_Deep_num = 0;

	if (FFT_src)
		delete[] FFT_src;
	FFT_src = new double[FFTSize];

	if (FFT_src_com)
		delete[] FFT_src_com;
	FFT_src_com = new Complex[FFTSize];

	int memsize = (HalfFFTSize + 2) * (average_Deep + 4);
	if (FFTBuff)
		delete[] FFTBuff;
	FFTBuff = new double[memsize];
	memset(FFTBuff, 0, memsize * sizeof(double));
	FFTOutBuff = &FFTBuff[(HalfFFTSize + 2) * average_Deep];
	FFTOutLogBuff = &FFTBuff[(HalfFFTSize + 2) * (average_Deep + 1)];
	FFTBrieflyBuff = &FFTBuff[(HalfFFTSize + 2) * (average_Deep + 2)];
	FFTBrieflyLogBuff = &FFTBuff[(HalfFFTSize + 2) * (average_Deep + 3)];

	ReleaseMutex(hMutexBuff);
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
			 printf("%lf+%lfj\n",real,imagin);
		 else
			 printf("%lf%lfj\n",real,imagin);*/
	}
	end = clock();
	printf("DFT use time :%lfs for Datasize of:%d\n", (double)(end - start) / CLOCKS_PER_SEC, size);

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
			printf("%lf+%lfj\n",real,imagin);
		else
			printf("%lf%lfj\n",real,imagin);
		*/
	}
	end = clock();
	printf("IDFT use time :%lfs for Datasize of:%d\n", (double)(end - start) / CLOCKS_PER_SEC, size);


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
	memset(FFT_src, 0, FFTSize * sizeof(double));
	switch (type)
	{
	case BUFF_DATA_TYPE::char_type:
	{
		char* buff = (char*)Buff;
		for (i = 0; i < FFTStep; i++, pos++) {
			FFT_src[i] = buff[pos & mask];
		}
	}
	break;
	case BUFF_DATA_TYPE::short_type:
	{
		short* buff = (short*)Buff;
		for (i = 0; i < FFTStep; i++, pos++) {
			FFT_src[i] = buff[pos & mask];
		}
	}
	break;
	}


	FFT_remap(FFT_src, FFTSize);
	// for(int i=0;i<size_n;i++)
	 //    printf("%lf\n",src[i]);

	int k = FFTSize;
	int z = 0;
	while (k /= 2) {
		z++;
	}
	k = z;
	if (FFTSize != (1 << k))
	{
		printf("file: %s. line: %d. func: %s\r\n", __FILE__, __LINE__, __FUNCTION__);
		exit(0);
	}
	//Complex * src_com=(Complex*)malloc(sizeof(Complex)*size_n);
	//if(src_com==NULL)exit(0);
	for (int i = 0; i < FFTSize; i++) {
		FFT_src_com[i].real = FFT_src[i];
		FFT_src_com[i].imagin = 0.0;
	}
	for (int i = 0; i < k; i++) {
		z = 0;
		for (int j = 0; j < FFTSize; j++) {
			if ((j / (1 << i)) % 2 == 1) {
				Complex wn;
				getWN(z, FFTSize, &wn);
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
void CFFT::FFT(void* Buff, BUFF_DATA_TYPE type, uint32_t pos, UINT mask)
{
	int i;

	clock_t start, end;
	start = clock();

	cuda_FFT(Buff, type, pos, mask);

	//NormalFFT(Buff, type, pos, mask);

	double maxfftv = -1 * DBL_MAX;
	double minfftv = DBL_MAX;// numeric_limits<double>::max();
	double maxfftvlog = -1 * DBL_MAX;
	double minfftvlog = DBL_MAX;// numeric_limits<double>::max();
	double d;
	double* addbuf;
	double* subbuf;
	double scale = FFTSize / FFTStep;

	WaitForSingleObject(hMutexBuff, INFINITE);

	addbuf = &FFTBuff[average_Deep_num * (HalfFFTSize + 2)];
	subbuf = &FFTBuff[((average_Deep_num + 1) & average_Deep_mask) * (HalfFFTSize + 2)];
	average_Deep_num++;
	average_Deep_num &= average_Deep_mask;
	for (i = 0; i < HalfFFTSize; i++)
	{
		//d = scale * sqrt(FFT_src_com[i].real * FFT_src_com[i].real + FFT_src_com[i].imagin * FFT_src_com[i].imagin);
		d = scale * sqrt(cuda_FFT_CompoData[i].x * cuda_FFT_CompoData[i].x + cuda_FFT_CompoData[i].y * cuda_FFT_CompoData[i].y);
		addbuf[i] = d / average_Deep;
		FFTOutBuff[i] += addbuf[i];
		FFTOutBuff[i] -= subbuf[i];

		FFTOutLogBuff[i] = log10(FFTOutBuff[i] / FFTMaxValue);

		if (i && maxfftv < d)maxfftv = d;
		if (i && minfftv > d)minfftv = d;
	}
	addbuf[i] = maxfftv / average_Deep;
	FFTOutBuff[i] += addbuf[i];
	FFTOutBuff[i] -= subbuf[i];
	FFTOutLogBuff[i] = log10(FFTOutBuff[i] / FFTMaxValue);
	i++;
	addbuf[i] = minfftv / average_Deep;
	FFTOutBuff[i] += addbuf[i];
	FFTOutBuff[i] -= subbuf[i];
	FFTOutLogBuff[i] = log10(FFTOutBuff[i] / FFTMaxValue);
	FFTCount++;

	fftWin->BrieflyBuff();
	fftWin->PaintSpectrum(this);

	ReleaseMutex(hMutexBuff);

	end = clock();
	static int TimeDelay = 0;
	if (TimeDelay++ == 100)
	{
		TimeDelay = 0;
		//printf("CFFT use time :%lfs for Datasize of:%d\n", (double)(end - start) / CLOCKS_PER_SEC, FFTSize);
	}
}

LPTHREAD_START_ROUTINE CFFT::FFT_Thread(LPVOID lp)
{
	OPENCONSOLE;
	CFFTWin* fw = (CFFTWin*)lp;
	fw->fft->Init(fw);
	fw->fft->FFT_func();
	//CLOSECONSOLE;
	return 0;
}

void CFFT::FFT_func(void)
{
	UINT fft_pos;
	UINT fft_between;

	FFTDoing = true;
	bFFT_Thread_Exitted = false;
	while (FFTDoing && Program_In_Process)
	{
		if (FFTNext == false) {	Sleep(0); continue;	}
		FFTNext = false;

		while (FFTDoing) {
			fft_pos = *DataBuffPos;
			fft_between = (fft_pos - FFTPos) & data_buff_length_mask;
			if (fft_between > FFTStep) {
				FFT(DataBuff, fftWin->buff_type, fft_pos - FFTStep, data_buff_length_mask);
				FFTPos = fft_pos;
				break;
			}
			else {
				Sleep(0);
				continue;
			}
		}
	}

	cuda_FFT_UnInit();
	bFFT_Thread_Exitted = true;
	hFFT_Thread = NULL;
}

double CFFT::GetFFTMaxValue(void)
{
	double i;
	double sumRe = 0.0;
	double sumIm = 0.0;
	UINT64 max = ((UINT64)1 << (ADC_DATA_SAMPLE_BIT -1)) - 1;
	for (i = 0; i < FFTSize; i++) {
		sumRe += (double)max * cos(2 * M_PI * i / FFTSize) * sin(2 * M_PI * i / FFTSize);
		sumIm += (double)max * sin(2 * M_PI * i / FFTSize) * sin(2 * M_PI * i / FFTSize);
	}
	double d = sqrt(sumRe * sumRe + sumIm * sumIm);
	printf("maxvalue:%d, %lf\n", max, d);
	return d/16;

}
