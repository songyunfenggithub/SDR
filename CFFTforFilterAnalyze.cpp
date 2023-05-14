
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

#include "CFilter.h"
#include "CFFTforFilterAnalyze.h"
#include "CWinFilter.h"

using namespace std;
using namespace WINS;
using namespace METHOD;

CFFTforFilterAnalyze clsFilterFFT;

////////////////////////////////////////////////////////////////////
//定义一个复数计算，包括乘法，加法，减法
///////////////////////////////////////////////////////////////////
CFFTforFilterAnalyze::CFFTforFilterAnalyze()
{
	OPENCONSOLE_SAVED;
}

CFFTforFilterAnalyze::~CFFTforFilterAnalyze()
{

}

void CFFTforFilterAnalyze::Add_Complex(Complex* src1, Complex* src2, Complex* dst)
{
	dst->imagin = src1->imagin + src2->imagin;
	dst->real = src1->real + src2->real;
}

void CFFTforFilterAnalyze::Sub_Complex(Complex* src1, Complex* src2, Complex* dst)
{
	dst->imagin = src1->imagin - src2->imagin;
	dst->real = src1->real - src2->real;
}

void CFFTforFilterAnalyze::Multy_Complex(Complex* src1, Complex* src2, Complex* dst)
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
void CFFTforFilterAnalyze::getWN(double n, double size_n, Complex* dst)
{
	double x = 2.0 * M_PI * n / size_n;
	dst->imagin = -sin(x);
	dst->real = cos(x);
}
////////////////////////////////////////////////////////////////////
//随机生成一个输入，显示数据部分已经注释掉了
//注释掉的显示部分为数据显示，可以观察结果
///////////////////////////////////////////////////////////////////
void CFFTforFilterAnalyze::setInput1(double* data, int  n)
{
	//DbgMsg("Setinput signal:\n");
	srand((int)time(0));
	for (int i = 0; i < FFT_SIZE; i++) {
		data[i] = rand();
		//DbgMsg("%lf\n",data[i]);
	}

}
////////////////////////////////////////////////////////////////////
//定义DFT函数，其原理为简单的DFT定义，时间复杂度O（n^2）,
//下面函数中有两层循环，每层循环的step为1，size为n，故为O（n*n）,
//注释掉的显示部分为数据显示，可以观察结果
///////////////////////////////////////////////////////////////////
void CFFTforFilterAnalyze::DFT(double* src, Complex* dst, int size)
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
void CFFTforFilterAnalyze::IDFT(Complex* src, Complex* dst, int size)
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
int CFFTforFilterAnalyze::FFT_remap(double* src, int size_n)
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

void CFFTforFilterAnalyze::FFT_for_FilterCore_Analyze(void* buff, void* fw)
{
	CWinFilter* pFilterWin = (CWinFilter*)fw;

	WaitForSingleObject(pFilterWin->hCoreAnalyseMutex, INFINITE);
	CFilter::FILTER_CORE_DATA_TYPE* pbuf = (CFilter::FILTER_CORE_DATA_TYPE*)buff;
	UINT size_n = pFilterWin->CoreAnalyseFFTLength;
	double*  src = new double[size_n];
	Complex* src_com = new Complex[size_n];
	int i;
	for (i = 0; i < size_n; i++) {
		src[i] = (double)*pbuf++;
	}

	FFT_remap(src, size_n);
	clock_t start, end;
	start = clock();
	int k = size_n;
	int z = 0;
	while (k /= 2)
	{
		z++;
	}
	k = z;
	if (size_n != (1 << k))
	{
		DbgMsg("file: %s. line: %d. func: %s\r\n", __FILE__, __LINE__, __FUNCTION__);
		exit(0);
	}
	for (int i = 0; i < size_n; i++)
	{
		src_com[i].real = src[i];
		src_com[i].imagin = 0.0;
	}
	for (int i = 0; i < k; i++)
	{
		z = 0;
		for (int j = 0; j < size_n; j++)
		{
			if ((j / (1 << i)) % 2 == 1)
			{
				Complex wn;
				getWN(z, size_n, &wn);
				Multy_Complex(&src_com[j], &wn, &src_com[j]);
				z += 1 << (k - i - 1);
				Complex temp;
				int neighbour = j - (1 << (i));
				temp.real = src_com[neighbour].real;
				temp.imagin = src_com[neighbour].imagin;
				Add_Complex(&temp, &src_com[j], &src_com[neighbour]);
				Sub_Complex(&temp, &src_com[j], &src_com[j]);
			}
			else z = 0;
		}
	}
	double fftvmax = -1.0 * DBL_MAX;
	double fftvmin = DBL_MAX;
	double fftvlogmax = -1.0 * DBL_MAX;
	double fftvlogmin = DBL_MAX;
	int half_n = size_n / 2;
	
	if (pFilterWin->CoreAnalyseFFTBuff != NULL) { 
		delete[] pFilterWin->CoreAnalyseFFTBuff;   
		pFilterWin->CoreAnalyseFFTBuff = NULL; 
	}
	if (pFilterWin->CoreAnalyseFFTLogBuff != NULL) { 
		delete[] pFilterWin->CoreAnalyseFFTLogBuff; 
		pFilterWin->CoreAnalyseFFTLogBuff = NULL; 
	}
	pFilterWin->CoreAnalyseFFTBuff = new double[half_n + 2];
	pFilterWin->CoreAnalyseFFTLogBuff = new double[half_n + 2];

	double* fftvs = pFilterWin->CoreAnalyseFFTBuff;
	double* fftvslog = pFilterWin->CoreAnalyseFFTLogBuff;

	double d;
	for (i = 0; i < half_n; i++) {
		d = sqrt(src_com[i].real * src_com[i].real + src_com[i].imagin * src_com[i].imagin);
		fftvs[i] = d;
		fftvslog[i] = log10(d);
		if (fftvmax < d) fftvmax = d;
		if (fftvmin > d) fftvmin = d;
	}
	fftvs[i] = fftvmax;
	fftvslog[i] = log10(fftvmax);
	i++;
	fftvs[i] = fftvmin;
	fftvslog[i] = log10(fftvmin);

	delete[] src;
	delete[] src_com;

	end = clock();
	ReleaseMutex(pFilterWin->hCoreAnalyseMutex);
}

void CFFTforFilterAnalyze::setInput(double* data, int  n)
{
	//DbgMsg("Setinput signal:\n");
	//srand((int)time(0));
	for (int i = 0; i < n; i++) {
		data[i] = 4096 + 2048 * (sin(2 * M_PI * i * 50 / n) +
			0.5 * cos(2 * M_PI * i * 20 / n + 2 * M_PI * 0.1));
		//        DbgMsg("%lf\n",data[i]);
				//DbgMsg("%d\n",data[i]);
	}
}
