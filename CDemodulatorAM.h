#pragma once

#include <stdio.h>

#include "CData.h"
#include "CFilter.h"

#define DEMODULATOR_AM_DECIMATION_FACTOR_BIT		0x3
#define DEMODULATOR_AM_DECIMATION_FACTOR			(0x1 << DEMODULATOR_AM_DECIMATION_FACTOR_BIT)

#define DEMODULATOR_AM_BUFF_LENGTH					0x10000
#define DEMODULATOR_AM_BUFF_LENGTH_MASK				(DEMODULATOR_AM_BUFF_LENGTH - 1)

#define DEMODULATOR_AM_FILTTED_BUFF_LENGTH			0x10000
#define DEMODULATOR_AM_FILTTED_BUFF_LENGTH_MASK		(DEMODULATOR_AM_BUFF_LENGTH - 1)

#define DEMODULATOR_AM_BUFF_STEP_LENGTH				0x2000
#define DEMODULATOR_AM_FILTER_SAMPLERATE_OFFSET_BIT	0x3
namespace DEVICES {
	class CAudio;
}
using namespace DEVICES;

class CDemodulatorAM
{
public:

	CAudio* m_Audio = NULL;

	HANDLE h_AM_Demodulator_Thread = NULL;
	bool AM_Demodulator_Doing = false;

	HANDLE h_AM_Demodulator_Thread_Audio_Out = NULL;
	bool AM_Demodulator_Audio_Out_Doing = false;

	double Am_Decimation_Factor = 1.0;

public:
	CDemodulatorAM();
	~CDemodulatorAM();

	void Init(void);
	void UnInit(void);

	void SaveValue(void);
	void RestoreValue(void);
	
	void AM_Demodulator_Thread_Func(void);
	void AM_Demodulator_Thread_Func_Get_Envelope(void);
	static LPTHREAD_START_ROUTINE AM_Demodulator_Thread(LPVOID lp);

	void AM_Demodulator_Thread_Func_Audio_Out(void);
	static LPTHREAD_START_ROUTINE AM_Demodulator_Thread_Audio_Out(LPVOID lp);

};