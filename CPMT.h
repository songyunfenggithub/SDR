#pragma once

#include <Windows.h>    

#define PMT_LENGTH			0x40000

class CPMT
{
public:

	HANDLE hThread;

	UINT32 u32Max = 0;
	UINT32 u32Min = 0;
	UINT32* PMTBuff = NULL;
	double* PMTLogBuff = NULL;

public:
	CPMT(void);
	~CPMT(void);

	void PMTFunc(void);

	static LPTHREAD_START_ROUTINE PMTThreadFun(LPVOID lp);


};

extern CPMT clsPMT;

