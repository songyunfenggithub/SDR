
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

//#include "CXPathXML.h"
#include "CDataFromTcpIp.h"
#include "CData.h"
#include "CWaveFilter.h"
#include "CWinSpectrum.h"
#include "CWaveFFT.h"
#include "CWinFFT.h"
#include "CWinOneFFT.h"
#include "CFilterWin.h"

#include "cuda_CFFT.cuh"

using namespace std;

CWaveFFT clsWaveFFT;

////////////////////////////////////////////////////////////////////
//定义一个复数计算，包括乘法，加法，减法
///////////////////////////////////////////////////////////////////
CWaveFFT::CWaveFFT()
{
	OPENCONSOLE;
	
	hMutexBuff = CreateMutex(NULL, false, "CWaveFFThMutexBuff");

}

CWaveFFT::~CWaveFFT()
{
	FFTDoing = false;
	while (!FFTThreadExit);
	/*
	WaitForSingleObject(hMutexBuff, INFINITE);
	if (FFT_src) delete[] FFT_src;
	if (FFT_src_com) delete[] FFT_src_com;
	if (FFTOrignalBuff) delete[] FFTOrignalBuff;
	if (FFTOrignalLogBuff) delete[] FFTOrignalLogBuff;
	if (FFTFilttedBuff) delete[] FFTFilttedBuff;
	if (FFTFilttedLogBuff) delete[] FFTFilttedLogBuff;
	ReleaseMutex(hMutexBuff);
	*/
}

void CWaveFFT::InitAllBuff(UINT fftsize, UINT fftstep)
{
	WaitForSingleObject(hMutexBuff, INFINITE);
	WaitForSingleObject(clsWinSpect.hMutexBuff, INFINITE);

	FFTSize = fftsize;
	HalfFFTSize = fftsize / 2;
	FFTStep = fftstep;

	//cuda_FFT_Init();
	if (orignal_FFT != NULL) delete orignal_FFT;
	orignal_FFT = new cuda_CFFT(); 
	orignal_FFT->cuda_FFT_Init(FFTSize, FFTStep, ADC_DATA_SAMPLE_BIT);

	if (filtted_FFT != NULL) delete filtted_FFT;
	filtted_FFT = new cuda_CFFT();
	filtted_FFT->cuda_FFT_Init(
		FFTSize >> clsWaveFilter.rootFilterInfo.decimationFactorBit,
		FFTStep >> clsWaveFilter.rootFilterInfo.decimationFactorBit,
		ADC_DATA_SAMPLE_BIT
	);

	clsWaveFFT.InitBuff();
	clsWinSpect.Init();
	clsWinSpect.InitBuff();

	ReleaseMutex(hMutexBuff);
	ReleaseMutex(clsWinSpect.hMutexBuff);
}

void CWaveFFT::SaveSettings(void)
{
	//clsXmlSet.SetElementVaue("FFT", "FFTSize", &FFTSize, CXPathXML::NodeInt32Value);
	//clsXmlSet.SetElementVaue("FFT", "FFTSize",&FFTStep, CXPathXML::NodeInt32Value);
}

void CWaveFFT::LoadSettings(void)
{
	//clsXmlSet.GetElementVaue("FFT", "FFTSize", &FFTSize, CXPathXML::NodeInt32Value);
	//clsXmlSet.GetElementVaue("FFT", "FFTSize", &FFTStep, CXPathXML::NodeInt32Value);
}

void CWaveFFT::Add_Complex(Complex* src1, Complex* src2, Complex* dst)
{
	dst->imagin = src1->imagin + src2->imagin;
	dst->real = src1->real + src2->real;
}

void CWaveFFT::Sub_Complex(Complex* src1, Complex* src2, Complex* dst)
{
	dst->imagin = src1->imagin - src2->imagin;
	dst->real = src1->real - src2->real;
}

void CWaveFFT::Multy_Complex(Complex* src1, Complex* src2, Complex* dst)
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
void CWaveFFT::getWN(double n, double size_n, Complex* dst)
{
	double x = 2.0 * M_PI * n / size_n;
	dst->imagin = -sin(x);
	dst->real = cos(x);
}
////////////////////////////////////////////////////////////////////
//随机生成一个输入，显示数据部分已经注释掉了
//注释掉的显示部分为数据显示，可以观察结果
///////////////////////////////////////////////////////////////////
void CWaveFFT::setInput1(double* data, int  n)
{
	//printf("Setinput signal:\n");
	srand((int)time(0));
	for (int i = 0; i < FFT_SIZE; i++) {
		data[i] = rand() % FFT_VALUE_MAX;
		//printf("%lf\n",data[i]);
	}

}
////////////////////////////////////////////////////////////////////
//定义DFT函数，其原理为简单的DFT定义，时间复杂度O（n^2）,
//下面函数中有两层循环，每层循环的step为1，size为n，故为O（n*n）,
//注释掉的显示部分为数据显示，可以观察结果
///////////////////////////////////////////////////////////////////
void CWaveFFT::DFT(double* src, Complex* dst, int size)
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
void CWaveFFT::IDFT(Complex* src, Complex* dst, int size)
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
int CWaveFFT::FFT_remap(double* src, int size_n)
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


void CWaveFFT::NormalFFT(WHICHSIGNAL WhichSignal, UINT pos)
{
	int i;
	memset(FFT_src, 0, FFTSize * sizeof(double));
	if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
		for (i = 0; i < FFTStep; i++, pos++) {
			FFT_src[i] = (double)clsData.AdcBuff[pos & DATA_BUFFER_MASK];
		}
	}
	else {
		for (i = 0; i < FFTStep; i++, pos++) {
			FFT_src[i] = (double)clsData.FilttedBuff[pos & DATA_BUFFER_MASK];
		}
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
void CWaveFFT::FFT(WHICHSIGNAL WhichSignal, UINT pos)
{
	int i;

	WaitForSingleObject(hMutexBuff, INFINITE);

	clock_t start, end;
	start = clock();

	if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL)
	{
		this->orignal_FFT->cuda_FFT(clsData.AdcBuff, BUFF_DATA_TYPE::short_type, pos, DATA_BUFFER_MASK);
	}
	else {
		this->filtted_FFT->cuda_FFT(clsData.FilttedBuff, BUFF_DATA_TYPE::float_type, pos, DATA_BUFFER_MASK);
	}
	//cuda_FFT(WhichSignal, pos);
	//NormalFFT(WhichSignal, pos);

	end = clock();

	double maxfftv = 0.0;
	double minfftv = DBL_MAX;// numeric_limits<double>::max();
	double maxfftvlog = 0.0;
	double minfftvlog = DBL_MAX;// numeric_limits<double>::max();
	double d, dlog;
	double* buf;
	double* buflog;
	double* subbuf;
	double* sublogbuf;
	double scale = FFTSize / FFTStep;

	int HalfFFTSize = WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL ? this->HalfFFTSize : (this->HalfFFTSize >> clsWaveFilter.rootFilterInfo.decimationFactorBit);
	double FFTMaxValue = WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL ? orignal_FFT->FFTMaxValue : filtted_FFT->FFTMaxValue;

	if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL)
	{
		buf			= &FFTOrignalBuff[FFTOrignalBuffNum * (HalfFFTSize + 2)];
		buflog		= &FFTOrignalLogBuff[FFTOrignalBuffNum * (HalfFFTSize + 2)];
		FFTOrignalBuffNum++;
		if (FFTOrignalBuffNum == FFTDeep) FFTOrignalBuffNum = 0;
		subbuf		= &FFTOrignalBuff[FFTOrignalBuffNum * (HalfFFTSize + 2)];
		sublogbuf	= &FFTOrignalLogBuff[FFTOrignalBuffNum * (HalfFFTSize + 2)];
		for (i = 0; i < HalfFFTSize; i++)
		{
			//d = scale * sqrt(FFT_src_com[i].real * FFT_src_com[i].real + FFT_src_com[i].imagin * FFT_src_com[i].imagin);
			d = scale * sqrt(orignal_FFT->cuda_FFT_CompoData[i].x * orignal_FFT->cuda_FFT_CompoData[i].x + orignal_FFT->cuda_FFT_CompoData[i].y * orignal_FFT->cuda_FFT_CompoData[i].y);
			buf[i] = d / (FFTDeep - 1);
			dlog = log10(d / FFTMaxValue);
//			dlog = log10(d);
			buflog[i] = dlog / (FFTDeep - 1);
			FFTOBuff[i] += buf[i];
			FFTOLogBuff[i] += buflog[i];
			FFTOBuff[i] -= subbuf[i];
			FFTOLogBuff[i] -= sublogbuf[i];

			if (i && maxfftv < d)maxfftv = d;
			if (i && minfftv > d)minfftv = d;
			if (i && maxfftvlog < dlog)maxfftvlog = dlog;
			if (i && minfftvlog > dlog)minfftvlog = dlog;
		}
		buf[i] = maxfftv / (FFTDeep - 1);
		FFTOBuff[i] += buf[i];
		buflog[i] = maxfftvlog / (FFTDeep - 1);
		FFTOLogBuff[i] += buflog[i];
		FFTOBuff[i] -= subbuf[i];
		FFTOLogBuff[i] -= sublogbuf[i];
		i++;
		buf[i] = minfftv / (FFTDeep - 1);
		FFTOBuff[i] += buf[i];
		buflog[i] = minfftvlog / (FFTDeep - 1);
		FFTOLogBuff[i] += buflog[i];
		FFTOBuff[i] -= subbuf[i];
		FFTOLogBuff[i] -= sublogbuf[i];
		FFTOrignalCount++;
	}
	else {
		buf = &FFTFilttedBuff[FFTFilttedBuffNum * (HalfFFTSize + 2)];
		buflog = &FFTFilttedLogBuff[FFTFilttedBuffNum * (HalfFFTSize + 2)];
		FFTFilttedBuffNum++;
		if (FFTFilttedBuffNum == FFTDeep) FFTFilttedBuffNum = 0;
		subbuf = &FFTFilttedBuff[FFTFilttedBuffNum * (HalfFFTSize + 2)];
		sublogbuf = &FFTFilttedLogBuff[FFTFilttedBuffNum * (HalfFFTSize + 2)];
		for (i = 0; i < HalfFFTSize; i++)
		{
			//d = scale * sqrt(FFT_src_com[i].real * FFT_src_com[i].real + FFT_src_com[i].imagin * FFT_src_com[i].imagin);
			d = scale * sqrt(filtted_FFT->cuda_FFT_CompoData[i].x * filtted_FFT->cuda_FFT_CompoData[i].x + filtted_FFT->cuda_FFT_CompoData[i].y * filtted_FFT->cuda_FFT_CompoData[i].y);
			buf[i] = d / (FFTDeep - 1);
			dlog = log10(d / FFTMaxValue);
//			dlog = log10(d);
			buflog[i] = dlog / (FFTDeep - 1);

			FFTFBuff[i] += buf[i];
			FFTFLogBuff[i] += buflog[i];
			FFTFBuff[i] -= subbuf[i];
			FFTFLogBuff[i] -= sublogbuf[i];
			if (i && maxfftv < d)maxfftv = d;
			if (i && minfftv > d)minfftv = d;
			if (i && maxfftvlog < dlog)maxfftvlog = dlog;
			if (i && minfftvlog > dlog)minfftvlog = dlog;
		}
		buf[i] = maxfftv / (FFTDeep - 1);
		FFTFBuff[i] += buf[i];
		buflog[i] = maxfftvlog / (FFTDeep - 1);
		FFTFLogBuff[i] += buflog[i];
		FFTFBuff[i] -= subbuf[i];
		FFTFLogBuff[i] -= sublogbuf[i];
		i++;
		buf[i] = minfftv / (FFTDeep - 1);
		FFTFBuff[i] += buf[i];
		buflog[i] = minfftvlog / (FFTDeep - 1);
		FFTFLogBuff[i] += buflog[i];
		FFTFBuff[i] -= subbuf[i];
		FFTFLogBuff[i] -= sublogbuf[i];
		FFTFilttedCount++;
	}

	ReleaseMutex(hMutexBuff);

//	if (clsWinOneFFT.OrignalFFTBuffReady == false && WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
	if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
		WaitForSingleObject(clsWinSpect.hMutexBuff, INFINITE);
		memcpy(clsWinSpect.OrignalFFTBuff, FFTOBuff, sizeof(double) * (HalfFFTSize + 2));
		memcpy(clsWinSpect.OrignalFFTBuffLog, FFTOLogBuff, sizeof(double) * (HalfFFTSize + 2));
		ReleaseMutex(clsWinSpect.hMutexBuff);
		if (clsWinOneSpectrum.hWnd != NULL) {
			clsWinOneFFT.BrieflyBuff(WhichSignal);
			while (clsWinOneSpectrum.inPaintSpectrumThread == true);
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CWinOneSpectrum::PaintSpectrum, NULL, 0, NULL);
		}
	}
	else  {
		WaitForSingleObject(clsWinSpect.hMutexBuff, INFINITE);
		memcpy(clsWinSpect.FilttedFFTBuff, FFTFBuff, sizeof(double) * (HalfFFTSize + 2));
		memcpy(clsWinSpect.FilttedFFTBuffLog, FFTFLogBuff, sizeof(double) * (HalfFFTSize + 2));
		ReleaseMutex(clsWinSpect.hMutexBuff);
		if (clsWinOneSpectrum.hWnd != NULL) {
			clsWinOneFFT.BrieflyBuff(WhichSignal);
			while (clsWinOneSpectrum.inPaintSpectrumThread == true);
			clsWinOneSpectrum.inPaintSpectrumThread = true;
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CWinOneSpectrum::PaintSpectrum, NULL, 0, NULL);
		}
	}

//	if (clsWinOneFFT.OrignalFFTBuffReady && clsWinOneFFT.FilttedFFTBuffReady && 
	if(clsWinOneFFT.hWnd != NULL) {
		InvalidateRect(clsWinOneFFT.hWnd, NULL, true);
		UpdateWindow(clsWinOneFFT.hWnd);
	}

	FFTCount++;
	//printf("FFTCount: %d, %d\r\n", FFTCount, sizeof(double));

	//end = clock();

	static int TimeDelay = 0;
	if (TimeDelay++ == 100)
	{
		TimeDelay = 0;
		printf("FFT use time :%lfs for Datasize of:%d\n", (double)(end - start) / CLOCKS_PER_SEC, FFTSize);
	}
}

////////////////////////////////////////////////////////////////////
//定义FFT，具体见算法说明，注释掉的显示部分为数据显示，可以观察结果
///////////////////////////////////////////////////////////////////
void CWaveFFT::FFT_orignal(WHICHSIGNAL WhichSignal, uint32_t pos)
{
	static double src[FFT_MAX_SIZE];
	static Complex src_com[FFT_MAX_SIZE];
	int i;
	if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
		for (i = 0; i < FFTSize; i++, pos++) {
			src[i] = (double)clsData.AdcBuff[pos & DATA_BUFFER_MASK];
		}
	}
	else {
		for (i = 0; i < FFTSize; i++, pos++) {
			src[i] = (double)clsData.FilttedBuff[pos & DATA_BUFFER_MASK];
		}
	}

	clock_t start, end;
	start = clock();

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

	for (i = 0; i < FFTSize; i++)
	{
		src_com[i].real = 0;
		src_com[i].imagin = 0;
		int n;
		for (n = 0; n < FFTSize; n++)
		{
			src_com[i].real += src[i] * cos(2 * M_PI * i * n / FFTSize);
			src_com[i].imagin += src[i] * sin(2 * M_PI * i * n / FFTSize);
		}
	}

	int halfi = FFTSize / 2;
	double maxfftv = 0.0;
	double minfftv = DBL_MAX;// numeric_limits<double>::max();
	double maxfftvlog = 0.0;
	double minfftvlog = DBL_MAX;// numeric_limits<double>::max();
	double d, dlog;
	if (WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL)
	{
		for (i = 0; i < halfi; i++)
		{
			d = sqrt(src_com[i].real * src_com[i].real + src_com[i].imagin * src_com[i].imagin);
			FFTOrignalBuff[i] = d;
			FFTOrignalLogBuff[i] = dlog = log10(d / FFTMaxValue);
			if (maxfftv < d)maxfftv = d;
			if (minfftv > d)minfftv = d;
			if (maxfftvlog < dlog)maxfftvlog = dlog;
			if (minfftvlog > dlog)minfftvlog = dlog;
		}
		FFTOrignalBuff[i] = maxfftv;
		FFTOrignalLogBuff[i] = maxfftvlog;
		i++;
		FFTOrignalBuff[i] = minfftv;
		FFTOrignalLogBuff[i] = minfftvlog;
		FFTOrignalCount++;
	}
	else {
		for (i = 0; i < halfi; i++)
		{
			d = sqrt(src_com[i].real * src_com[i].real + src_com[i].imagin * src_com[i].imagin);
			FFTFilttedBuff[i] = d;
			FFTFilttedLogBuff[i] = dlog = log10(d / FFTMaxValue);
			if (maxfftv < d)maxfftv = d;
			if (minfftv > d)minfftv = d;
			if (maxfftvlog < dlog)maxfftvlog = dlog;
			if (minfftvlog > dlog)minfftvlog = dlog;
		}
		FFTFilttedBuff[i] = maxfftv;
		FFTFilttedLogBuff[i] = maxfftvlog;
		i++;
		FFTFilttedBuff[i] = minfftv;
		FFTFilttedLogBuff[i] = minfftvlog;
		FFTFilttedCount++;
	}

	if (WhichSignal == WHICHSIGNAL::SIGNAL_FILTTED)
		clsGetDataTcpIp.SendFFTData((char*)FFTFilttedLogBuff, (halfi + 2) * sizeof(double));

	if (clsWinFFT.FFTNeedReDraw) {
		if (clsWinFFT.OrignalFFTBuffReady == false && WhichSignal == WHICHSIGNAL::SIGNAL_ORIGNAL) {
			memcpy(clsWinFFT.OrignalFFTBuff, FFTOrignalBuff, sizeof(double) * (FFTSize / 2 + 2));
			memcpy(clsWinFFT.OrignalFFTBuffLog, FFTOrignalLogBuff, sizeof(double) * (FFTSize / 2 + 2));
			clsWinFFT.OrignalFFTBuffReady = true;
		}
		if (clsWinFFT.FilttedFFTBuffReady == false && WhichSignal == WHICHSIGNAL::SIGNAL_FILTTED) {
			memcpy(clsWinFFT.FilttedFFTBuff, FFTFilttedBuff, sizeof(double) * (FFTSize / 2 + 2));
			memcpy(clsWinFFT.FilttedFFTBuffLog, FFTFilttedLogBuff, sizeof(double) * (FFTSize / 2 + 2));
			clsWinFFT.FilttedFFTBuffReady = true;
		}
		if (clsWinFFT.OrignalFFTBuffReady && clsWinFFT.FilttedFFTBuffReady) 
		{
			InvalidateRect(clsWinFFT.hWnd, NULL, true);
		}
	}

	FFTCount++;
	//printf("FFTCount: %d, %d\r\n", FFTCount, sizeof(double));

	//end = clock();

	static int TimeDelay = 0;
	if (TimeDelay++ == 100)
	{
		TimeDelay = 0;
		printf("FFT use time :%lfs for Datasize of:%d\n", (double)(end - start) / CLOCKS_PER_SEC, FFTSize);
	}
}
////////////////////////////////////////////////////////////////////


void CWaveFFT::FFT_for_FilterCore_Analyze(FILTER_CORE_DATA_TYPE* pbuf, CFilterWin* pFilterWin)
{
	WaitForSingleObject(pFilterWin->hCoreAnalyseMutex, INFINITE);

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
		printf("file: %s. line: %d. func: %s\r\n", __FILE__, __LINE__, __FUNCTION__);
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

void CWaveFFT::setInput(double* data, int  n)
{
	//printf("Setinput signal:\n");
	//srand((int)time(0));
	for (int i = 0; i < n; i++) {
		data[i] = 4096 + 2048 * (sin(2 * M_PI * i * 50 / n) +
			0.5 * cos(2 * M_PI * i * 20 / n + 2 * M_PI * 0.1));
		//        printf("%lf\n",data[i]);
				//printf("%d\n",data[i]);
	}
}

LPTHREAD_START_ROUTINE CWaveFFT::FFT_Thread(LPVOID lp)
{
	OPENCONSOLE;
	clsWaveFFT.FFT_func();
	//CLOSECONSOLE;
	return 0;
}

void CWaveFFT::FFT_func(void)
{

	UINT orignal_work_pos;
	UINT orignal_between;
	UINT filtted_work_pos;
	UINT filtted_between;

	while (FFTDoing && Program_In_Process)
	{
		clsWinOneFFT.FFTNeedReDraw = false;
		while (FFTDoing && clsWinOneFFT.FFTNeedReDraw == false) {
			Sleep(50);
		}

		UINT fftstep_filtted = FFTStep >> clsWaveFilter.rootFilterInfo.decimationFactorBit;
		orignal_work_pos = clsData.AdcPos;
		orignal_between = (orignal_work_pos - FFTPos) & DATA_BUFFER_MASK;
		filtted_work_pos = clsData.FilttedBuffPos;
		filtted_between = (filtted_work_pos - FFTFilttedPos) & DATA_BUFFER_MASK;
		if (orignal_between > FFTStep || filtted_between > fftstep_filtted) {
			if (orignal_between > FFTStep)
			{
				FFT(WHICHSIGNAL::SIGNAL_ORIGNAL, orignal_work_pos - FFTStep);
				FFTPos = orignal_work_pos;
			}
			if (filtted_between > fftstep_filtted)
			{
				FFT(WHICHSIGNAL::SIGNAL_FILTTED, filtted_work_pos - fftstep_filtted);
				FFTFilttedPos = filtted_work_pos;
			}
		}
		else {
			Sleep(0);
			continue;
		}
	}
	orignal_FFT->cuda_FFT_UnInit();
	filtted_FFT->cuda_FFT_UnInit();
	FFTThreadExit = true;
}

void CWaveFFT::InitBuff(void) 
{

	if (FFT_src) delete[] FFT_src;
	FFT_src = new double[FFTSize];

	if (FFT_src_com) delete[] FFT_src_com;
	FFT_src_com = new Complex[FFTSize];

	int memsize = (HalfFFTSize + 2) * (FFT_DEEP + 1);
	if (FFTOrignalBuff) delete[] FFTOrignalBuff;
	FFTOrignalBuff = new double[memsize];
	memset(FFTOrignalBuff, 0, memsize * sizeof(double));
	FFTOBuff = &FFTOrignalBuff[(HalfFFTSize + 2) * FFT_DEEP];
	
	if (FFTOrignalLogBuff) delete[] FFTOrignalLogBuff;
	FFTOrignalLogBuff = new double[memsize];
	memset(FFTOrignalLogBuff, 0, memsize * sizeof(double));
	FFTOLogBuff = &FFTOrignalLogBuff[(HalfFFTSize + 2) * FFT_DEEP];

	if (FFTFilttedBuff) delete[] FFTFilttedBuff;
	FFTFilttedBuff = new double[memsize];
	memset(FFTFilttedBuff, 0, memsize * sizeof(double));
	FFTFBuff = &FFTFilttedBuff[(HalfFFTSize + 2) * FFT_DEEP];

	if (FFTFilttedLogBuff) delete[] FFTFilttedLogBuff;
	FFTFilttedLogBuff = new double[memsize];
	memset(FFTFilttedLogBuff, 0, memsize * sizeof(double));
	FFTFLogBuff = &FFTFilttedLogBuff[(HalfFFTSize + 2) * FFT_DEEP];
}

double CWaveFFT::GetFFTMaxValue(void)
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
