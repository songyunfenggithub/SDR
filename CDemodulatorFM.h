
#pragma once

#include <stdio.h>

#include "CData.h"
#include "CFilter.h"

namespace DEVICES {
	class CAudio;
}
using namespace DEVICES;

class CDemodulatorFM
{
public:

	CAudio* m_Audio = NULL;

	HANDLE hThread = NULL;
	bool Doing = false;

public:
	CDemodulatorFM();
	~CDemodulatorFM();

	void Init(void);
	void UnInit(void);

	void SaveValue(void);
	void RestoreValue(void);

	void Thread_Func(void);
	void Thread_Func_IQ(void);

	static LPTHREAD_START_ROUTINE Demodulator_Thread(LPVOID lp);
};