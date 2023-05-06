
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
	m_Audio->SampleRate = (double)AdcData->SampleRate / (1 << clsMainFilter.rootFilterInfo.decimationFactorBit) / 4;
	Am_Decimation_Factor = (double)AdcData->SampleRate / (1 << clsMainFilter.rootFilterInfo.decimationFactorBit) / m_Audio->SampleRate;
	UINT DemodulatorStepLength = SOUNDCARD_STEP_LENGTH * Am_Decimation_Factor;
	AM_Demodulator_Doing = true;
	float* SrcBuff = (float*)AdcDataFiltted->Buff;
	UINT SrcPosMask = AdcDataFiltted->Mask;
	UINT DemodulattedPos = (AdcDataFiltted->Pos - SOUNDCARD_STEP_LENGTH) & AdcDataFiltted->Mask;
	short* Buff = (short*)m_Audio->outData->Buff;
	while (AM_Demodulator_Doing && Program_In_Process) {
		am_pos = AdcDataFiltted->Pos;
		am_between = (am_pos - DemodulattedPos) & SrcPosMask;
		if (am_between > DemodulatorStepLength) {
			double dbi = 0;
			for (int i = 0; i < SOUNDCARD_STEP_LENGTH; i++, dbi += Am_Decimation_Factor) {
				Buff[m_Audio->outData->Pos + i] = SrcBuff[(UINT)(DemodulattedPos + dbi) & SrcPosMask] * m_Audio->Am_zoom;
			}
			DemodulattedPos += SOUNDCARD_STEP_LENGTH;
			DemodulattedPos &= SrcPosMask;
			if (m_Audio->boutOpened == true) {
				INT x = m_Audio->WriteToOut(m_Audio->outData->Pos);
				if (x >= 0) Am_Decimation_Factor_offset += x > (SOUNDCARD_WAVEHDR_DEEP / 2) ? -0.1 : 0.1;
			}
			else { Am_Decimation_Factor_offset = 0.0; }
			m_Audio->outData->Pos += SOUNDCARD_STEP_LENGTH;
			m_Audio->outData->Pos &= SOUNDCARD_BUFF_LENGTH_MASK;
			//Am_Decimation_Factor = (double)AdcData->SampleRate / (1 << clsFilter.rootFilterInfo.decimationFactorBit) / m_Audio->SampleRate;
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
	AudioData->SampleRate = AudioDataFiltted->SampleRate = m_Audio->SampleRate = clsMainFilter.rootFilterInfo.nextFilter->FreqCenter;
	clsAudioFilter.ParseCoreDesc(&clsAudioFilter.rootFilterInfo);
	AM_Demodulator_Doing = true;
	float* SrcBuff = (float*)AdcDataFiltted->Buff;
	UINT DemodulattedPos = (AdcDataFiltted->Pos - SOUNDCARD_STEP_LENGTH) & AdcDataFiltted->Mask;
	m_Audio->outData->Pos = 0;
	short* TargetBuff = (short*)m_Audio->outData->Buff;
	CData* out = m_Audio->outDataFiltted;
	while (AM_Demodulator_Doing && Program_In_Process) {
		if (((AdcDataFiltted->Pos - DemodulattedPos) & AdcDataFiltted->Mask) > SOUNDCARD_STEP_LENGTH) {
			double dbi = 0;
			for (int i = 0; i < SOUNDCARD_STEP_LENGTH; i++) {
				if (
					SrcBuff[DemodulattedPos] > SrcBuff[(DemodulattedPos - 1) & AdcDataFiltted->Mask] &&
					SrcBuff[DemodulattedPos] > SrcBuff[(DemodulattedPos + 1) & AdcDataFiltted->Mask]
					) {
					TargetBuff[m_Audio->outData->Pos++] = SrcBuff[DemodulattedPos] * m_Audio->Am_zoom;
					m_Audio->outData->Pos &= m_Audio->outData->Mask;
				}
				DemodulattedPos++;
				DemodulattedPos &= AdcDataFiltted->Mask;
			}
		}
		else {
			Sleep(10);
		}
	}
	h_AM_Demodulator_Thread = NULL;
}

LPTHREAD_START_ROUTINE CDemodulatorAM::AM_Demodulator_Thread_Audio_Out(LPVOID lp)
{
	CDemodulatorAM* me = (CDemodulatorAM*)lp;
	me->AM_Demodulator_Thread_Func_Audio_Out();
	return 0;
}

void CDemodulatorAM::AM_Demodulator_Thread_Func_Audio_Out(void)
{
	AM_Demodulator_Audio_Out_Doing = true;
	short outBuff[SOUNDCARD_STEP_LENGTH];
	CData* out = m_Audio->outDataFiltted;
	UINT pos = 0;
	while (AM_Demodulator_Audio_Out_Doing && m_Audio->boutOpened && Program_In_Process) {
		if (((out->Pos - pos) & out->Mask) > SOUNDCARD_STEP_LENGTH) {
			float* srcBuff = (float*)out->Buff;
			for (int i = 0; i < SOUNDCARD_STEP_LENGTH; i++) {
				outBuff[i] = (short)srcBuff[pos++];
				pos &= out->Mask;
			}
			m_Audio->WriteToOut(outBuff);
		}
	}
	h_AM_Demodulator_Thread_Audio_Out = NULL;
}