
#include "stdafx.h"
#include "resource.h"
#include <iostream>

#include "CAudio.h"

#include "CDemodulatorAM.h"

//CDemodulatorAM clsDemodulatorAm;

CDemodulatorAM::CDemodulatorAM()
{
	RestoreValue();
	Init();
}

CDemodulatorAM::~CDemodulatorAM()
{
	SaveValue();
	UnInit();
}

void CDemodulatorAM::Init(void)
{
	m_Audio = &mAudio;
}

void CDemodulatorAM::UnInit(void)
{

}

void CDemodulatorAM::SaveValue(void)
{
	//WritePrivateProfileString("CDemodulatorAM", "AMFilterCoreDesc", pFilterInfo->CoreDescStr, IniFilePath);
}

void CDemodulatorAM::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	//GetPrivateProfileString("CDemodulatorAM", "AMFilterCoreDesc", "1", AMFilterCoreDesc, FILTER_DESC_LENGTH, IniFilePath);
}

LPTHREAD_START_ROUTINE CDemodulatorAM::AM_Demodulator_Thread(LPVOID lp)
{
	CDemodulatorAM* me = (CDemodulatorAM*)lp;
	//me->AM_Demodulator_Thread_Func();
	me->AM_Demodulator_Thread_Func_Get_Envelope();
	return 0;
}

void CDemodulatorAM::AM_Demodulator_Thread_Func(void)
{
	UINT am_pos;
	UINT am_between;
	double Am_Decimation_Factor_offset = 0.0;
	m_Audio->SampleRate = (double)clsData.AdcSampleRate / (1 << clsWaveFilter.rootFilterInfo.decimationFactorBit) / 4;
	Am_Decimation_Factor = (double)clsData.AdcSampleRate / (1 << clsWaveFilter.rootFilterInfo.decimationFactorBit) / m_Audio->SampleRate;
	DemodulatorStepLength = SOUNDCARD_STEP_LENGTH * Am_Decimation_Factor;
	AM_Demodulator_Doing = true;
	SrcBuff = clsData.FilttedBuff;
	SrcPos = &clsData.FilttedBuffPos;
	SrcPosMask = DATA_BUFFER_MASK;
	DemodulattedPos = *SrcPos;
	while (AM_Demodulator_Doing && Program_In_Process) {
		am_pos = *SrcPos;
		am_between = (am_pos - DemodulattedPos) & SrcPosMask;
		if (am_between > DemodulatorStepLength) {
			double dbi = 0;
			for (int i = 0; i < SOUNDCARD_STEP_LENGTH; i++, dbi += Am_Decimation_Factor) {
				m_Audio->outBuff[m_Audio->outBuffPos + i] = SrcBuff[(UINT)(DemodulattedPos + dbi) & SrcPosMask] * m_Audio->Am_zoom;
			}
			DemodulattedPos += SOUNDCARD_STEP_LENGTH;
			DemodulattedPos &= SrcPosMask;
			if (m_Audio->boutOpened == true) {
				INT x = m_Audio->WriteToOut(m_Audio->outBuffPos);
				if (x >= 0) Am_Decimation_Factor_offset += x > (SOUNDCARD_WAVEHDR_DEEP / 2) ? -0.1 : 0.1;
			}
			else { Am_Decimation_Factor_offset = 0.0; }
			m_Audio->outBuffPos += SOUNDCARD_STEP_LENGTH;
			m_Audio->outBuffPos &= SOUNDCARD_BUFF_LENGTH_MASK;
			//Am_Decimation_Factor = (double)clsData.AdcSampleRate / (1 << clsWaveFilter.rootFilterInfo.decimationFactorBit) / m_Audio->SampleRate;
			//Am_Decimation_Factor += Am_Decimation_Factor_offset;
			//DemodulatorStepLength = SOUNDCARD_STEP_LENGTH * Am_Decimation_Factor;
			//break;
		}
		else {
			Sleep(10);
			continue;
		}
	}
	h_AM_Demodulator_Thread = NULL;
}

void CDemodulatorAM::AM_Demodulator_Thread_Func_Get_Envelope(void)
{
	m_Audio->SampleRate = clsWaveFilter.rootFilterInfo.nextFilter->FreqCenter;
	AM_Demodulator_Doing = true;
	SrcBuff = clsData.FilttedBuff;
	SrcPos = &clsData.FilttedBuffPos;
	DemodulattedPos = *SrcPos;
	m_Audio->outBuffPos = 0;
	while (AM_Demodulator_Doing && Program_In_Process) {
		if (((*SrcPos - DemodulattedPos) & DATA_BUFFER_MASK) > SOUNDCARD_STEP_LENGTH) {
			double dbi = 0;
			for (int i = 0; i < SOUNDCARD_STEP_LENGTH; i++) {
				if (
					SrcBuff[DemodulattedPos] > SrcBuff[(DemodulattedPos - 1) & DATA_BUFFER_MASK] &&
					SrcBuff[DemodulattedPos] > SrcBuff[(DemodulattedPos + 1) & DATA_BUFFER_MASK]
					) {
					m_Audio->outBuff[m_Audio->outBuffPos++] = SrcBuff[DemodulattedPos] * m_Audio->Am_zoom;
					m_Audio->outBuffPos &= SOUNDCARD_BUFF_LENGTH_MASK;
				
					if ((m_Audio->boutOpened == true) && ((m_Audio->outBuffPos % SOUNDCARD_STEP_LENGTH) == 0)) {
						m_Audio->WriteToOut((m_Audio->outBuffPos - SOUNDCARD_STEP_LENGTH) % SOUNDCARD_BUFF_LENGTH_MASK);
					}
				}
				DemodulattedPos++;
				DemodulattedPos &= DATA_BUFFER_MASK;
			}
		}
		else {
			Sleep(10);
			continue;
		}
	}
	h_AM_Demodulator_Thread = NULL;
}