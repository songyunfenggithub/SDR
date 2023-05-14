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

	HANDLE hThread = NULL;
	bool Doing = false;

public:
	CDemodulatorAM();
	~CDemodulatorAM();

	void Init(void);
	void UnInit(void);

	void SaveValue(void);
	void RestoreValue(void);
	
	void Thread_Func(void);
	void Thread_Func_Get_Envelope(void);
	void Thread_Func_IQ(void);
	static LPTHREAD_START_ROUTINE AM_Demodulator_Thread(LPVOID lp);
};