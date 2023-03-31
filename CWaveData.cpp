#include "stdafx.h"
#include <stdio.h>
#include "public.h"
#include "CSDR.h"
#include "CWaveData.h"
#include "CWaveFFT.h"
#include "CWaveAnalyze.h"


#define _USE_MATH_DEFINES
#include "math.h"

#define TIMEOUT	1000
CWaveData clsWaveData;

CWaveData::CWaveData()
{
	clsWaveData.AdcSampleRate = ADC_SAMPLE_RATE;
	for (int i = 0; i < DATA_BUFFER_LENGTH; i++) FilttedFlag[i] = 0;

	SetTimer(NULL, 0, TIMEOUT, (TIMERPROC)CWaveData::NumPerSecTimer_Func);
}

CWaveData::~CWaveData()
{

}

void CWaveData::GeneratorWave(void)
{
	return;
	int size = 1024;
	double i;
	double amplitude = (double)(((UINT32)1 << (ADC_DATA_SAMPLE_BIT - 1)) - 1) / (clsWaveData.AdcSampleRate / 2);
	double d;
	double doubleAdcSampleRate = (double)clsWaveData.AdcSampleRate;

	static bool g = false;
	static int fg = 60;
	static int gn = doubleAdcSampleRate / fg;
	static int gnc = 0;

	for (i = 0; i < size; i++, AdcPos++)
	{
		d = 0.0;

		for (double f = 0; f < doubleAdcSampleRate / 2 - 2000; f++) {
			if (f == 50 || f == 150 || f == 512)
				d += amplitude * sin(2 * M_PI * (f+ 1.0) / doubleAdcSampleRate * (double)AdcPos);
		}

		/*
		if (AdcPos % 100000 == 0) {
			gnc = 0;
			g = true;
		}
		if(g)
		//for (double f = 0; f < doubleAdcSampleRate / 2 - 2000; f++) 
		{
			//if (f == fg)
				d += amplitude * sin(2 * M_PI * fg / doubleAdcSampleRate * (double)AdcPos);
			if (gnc++ >= gn) g = false;
		}
		*/

		//            d = (long) 0xff000000;
		AdcBuff[AdcPos & DATA_BUFFER_MASK] = d;
	}
	if (AdcPos >= DATA_BUFFER_LENGTH) AdcPos -= DATA_BUFFER_LENGTH;
	AdcGetNew = true;
}

TIMERPROC CWaveData::NumPerSecTimer_Func(void)
{
//	clsWaveData.NumPerSec = clsWaveData.NumPerSecInProcess / sizeof(ADCDATATYPE) / TIMEOUT * 1000;
//	clsWaveData.NumPerSecInProcess = 0;
	static UINT SaveAdcPos = 0, SaveFFTCount = 0;
	static UINT tick = 0;
	tick++;
	if ((tick % 10) == 0)
	{
		clsWaveFFT.FFTPerSec = (clsWaveFFT.FFTCount - SaveFFTCount) / 10.0;
		SaveFFTCount = clsWaveFFT.FFTCount;
	}
	
	clsWaveData.NumPerSec = (clsWaveData.AdcPos - SaveAdcPos) & DATA_BUFFER_MASK;
	SaveAdcPos = clsWaveData.AdcPos;
	//printf("NumPreSec:%d\r\n", clsWaveData.NumPerSec);
	return 0;
}
