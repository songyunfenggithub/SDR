
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <limits>

#include "CWaveData.h"

#include "CPMT.h"

CPMT clsPMT;

CPMT::CPMT()
{
	OPENCONSOLE;

	if (PMTBuff != NULL) delete[] PMTBuff;
	PMTBuff = new UINT32[PMT_LENGTH];
	memset(PMTBuff, 0, PMT_LENGTH * sizeof(UINT32));

	if (PMTLogBuff != NULL) delete[] PMTLogBuff;
	PMTLogBuff = new double[PMT_LENGTH];
	memset(PMTLogBuff, 0, PMT_LENGTH * sizeof(double));

	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CPMT::PMTThreadFun, NULL, 0, NULL);
}

CPMT::~CPMT()
{
	if (PMTBuff != NULL) delete[] PMTBuff;
	if (PMTLogBuff != NULL)	delete[] PMTLogBuff;
}

void CPMT::PMTFunc(void)
{
	UINT32 Pos = 0;
	UINT32 top, low;
	UINT32 W1, W2;
	double H1, H2;
	H1 = H2 = 0.0;
	W1 = W2 = 0;
	top = low = 0;
	BOOL isInUP = false;
	PADCDATATYPE pBuff = clsWaveData.AdcBuff;
	u32Max = 0;
	u32Min = 0xFFFFFFFF;
	UINT32 H = 0;
	while (TRUE)
	{
		if (Pos != clsWaveData.FilttedPos) {
			if (pBuff[Pos] > pBuff[(Pos - 1) & DATA_BUFFER_MASK]) {
				if (!isInUP) {
					low = (Pos - 1) & DATA_BUFFER_MASK;
					W2 = (low - top) & DATA_BUFFER_MASK;
					H2 = pBuff[top] - pBuff[low];
					isInUP = true;
					//printf("PMT W1: %u, H1: %f, W2: %u, H2: %f\n", W1, H1, W2, H2);
					H = pBuff[top];
					//PMTBuff[H]++;
					//if (PMTBuff[H] < u32Min) u32Min = PMTBuff[H];
					//if (PMTBuff[H] > u32Max) u32Max = PMTBuff[H];
					//PMTLogBuff[H] = log10(PMTBuff[H]);
				}
			}
			else {
				if (isInUP) {
					top = (Pos - 1) & DATA_BUFFER_MASK;
					W1 = (top - low) & DATA_BUFFER_MASK;
					H1 = pBuff[top] - pBuff[low];
					H = abs(pBuff[top]);
					isInUP = false;
				}
			}
			Pos++;
			if (Pos == DATA_BUFFER_LENGTH) Pos = 0;
		}
		else {
			Sleep(100);
		}
	}
}

LPTHREAD_START_ROUTINE CPMT::PMTThreadFun(LPVOID lp)
{
	clsPMT.PMTFunc();
	return 0;
}