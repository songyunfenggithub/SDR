#pragma once

#include <stdio.h>

#include "CWaveData.h"
#include "CWaveFilter.h"

#define DEMODULATOR_AM_DECIMATION_FACTOR_BIT	0x3
#define DEMODULATOR_AM_DECIMATION_FACTOR		(0x1 << DEMODULATOR_AM_DECIMATION_FACTOR_BIT)

#define DEMODULATOR_AM_BUFF_LENGTH			0x10000
#define DEMODULATOR_AM_BUFF_LENGTH_MASK		(DEMODULATOR_AM_BUFF_LENGTH - 1)

#define DEMODULATOR_AM_FILTTED_BUFF_LENGTH			0x10000
#define DEMODULATOR_AM_FILTTED_BUFF_LENGTH_MASK		(DEMODULATOR_AM_BUFF_LENGTH - 1)

#define DEMODULATOR_AM_BUFF_STEP_LENGTH						0x2000
#define DEMODULATOR_AM_FILTER_SAMPLERATE_OFFSET_BIT			0x3


class CDemodulatorAM
{
public:
	CHAR AMFilterCoreDesc[FILTER_DESC_LENGTH];
	
	FILTEDDATATYPE	FilttedBuff[DEMODULATOR_AM_FILTTED_BUFF_LENGTH];
	UINT FilttedPos = 0;

	FILTEDDATATYPE	AMBuff[DEMODULATOR_AM_BUFF_LENGTH];
	UINT AMPos = 0;


	CWaveFilter::PFILTERINFO pFilterInfo;


public:
	CDemodulatorAM();
	~CDemodulatorAM();

	void build_AM_Filter_Core(void);

	void SaveValue(void);
	void RestoreValue(void);

};

extern CDemodulatorAM clsDemodulatorAm;