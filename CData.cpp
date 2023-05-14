#include "stdafx.h"
#include <stdio.h>
#include "public.h"
#include "CSDR.h"
#include "CData.h"
#include "CAnalyze.h"


#define _USE_MATH_DEFINES
#include "math.h"

//using namespace WINS; 
//using namespace DEVICES;

#define TIMEOUT	1000

CData* AdcDataI = NULL;
CData* AdcDataIFiltted = NULL;
CData* AdcDataQ = NULL;
CData* AdcDataQFiltted = NULL;

CData* AudioData = NULL;
CData* AudioDataFiltted = NULL;

char AdcBuffMarks[DATA_BUFFER_LENGTH];

CData::CData()
{

}

CData::~CData()
{
	UnInit();
}

void CData::Init(UINT len, BUFF_DATA_TYPE dataType, INT dataBits)
{
	UnInit();

	DataType = dataType;
	Pos = 0;
	SavedPos = 0;
	ProcessPos = 0;
	CharPos = 0;
	Len = len;
	Mask = len - 1;
	GetNew = false;
	SampleRate = 0;
	DataBits = dataBits;

	switch (dataType) {
	case short_type:
		SizeOfType = sizeof(short);		
		Buff = new short[len];
		MoveBit = 1;
		break;
	case float_type:
		SizeOfType = sizeof(float);
		Buff = new float[len];
		MoveBit = 2;
		break;
	}
}

void CData::UnInit(void)
{
	if (Buff != NULL)
	{
		delete[] Buff;
		Buff = NULL;
	}
}

void CData::GeneratorWave(void)
{
	return;
	int size = 1024;
	double i;
	double amplitude = (double)(((UINT32)1 << (DataBits - 1)) - 1) / (SampleRate / 2);
	double d;
	double doubleAdcSampleRate = (double)SampleRate;

	static bool g = false;
	static int fg = 60;
	static int gn = doubleAdcSampleRate / fg;
	static int gnc = 0; 

	for (i = 0; i < size; i++)
	{
		d = 0.0;

		for (double f = 0; f < doubleAdcSampleRate / 2 - 2000; f++) {
			if (f == 50 || f == 150 || f == 512)
				d += amplitude * sin(2 * M_PI * (f+ 1.0) / doubleAdcSampleRate * (double)Pos);
		}

		/*
		if (Pos % 100000 == 0) {
			gnc = 0;
			g = true;
		}
		if(g)
		//for (double f = 0; f < doubleAdcSampleRate / 2 - 2000; f++) 
		{
			//if (f == fg)
				d += amplitude * sin(2 * M_PI * fg / doubleAdcSampleRate * (double)Pos);
			if (gnc++ >= gn) g = false;
		}
		*/
		//            d = (long) 0xff000000;
		switch (DataType) {
		case short_type:
			((short*)Buff)[Pos++] = d;
			break;
		case float_type:
			((float*)Buff)[Pos++] = d;
			break;
		}
		Pos &= Mask;
	}
	GetNew = true;
}