#include "stdafx.h"

#include "stdio.h"

#define _USE_MATH_DEFINES
#include "math.h"

#include "public.h"
#include "CData.h"
#include "CWaveFunc.h"

CWaveFunc clsWaveFunc;

CWaveFunc::CWaveFunc()
{

}

CWaveFunc::~CWaveFunc()
{

}

void CWaveFunc::WaveGenerate(void)
{
	PADCDATATYPE p = clsData.AdcBuff;
	clsData.AdcPos = 0xFFFFF;
	UINT32 i;

	for (i = 0; i < clsData.AdcPos; i++)
	{
		*p++ = Sin(i, 500, (1 << 31)-1);// +Sin(i, 200, 1 << 31);
	}
}

double CWaveFunc::Sin(UINT32 i, double Hz, double amplitude)
{
	double d = amplitude * sin(2 * M_PI / (clsData.AdcSampleRate / Hz) * i);
	//printf("%f\r\n", d);
	return d;
}

double CWaveFunc::Triangle(UINT32 i, double Hz, double amplitude)
{
	double l = clsData.AdcSampleRate / Hz;

	return 0;
}