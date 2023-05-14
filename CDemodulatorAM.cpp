
#include "stdafx.h"
#include "resource.h"
#include <iostream>

#include "public.h"

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
	me->Thread_Func_IQ();
	return 0;
}

void CDemodulatorAM::Thread_Func(void)
{
	UINT am_pos;
	UINT am_between;
	double Am_Decimation_Factor_offset = 0.0;
	m_Audio->uSampleRate = (double)AdcDataI->SampleRate / (1 << clsMainFilterI.rootFilterInfo1.decimationFactorBit) / 4;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	float Decimation_Factor = (double)AdcDataI->SampleRate / (1 << clsMainFilterI.rootFilterInfo1.decimationFactorBit) / m_Audio->uSampleRate;
	UINT DemodulatorStepLength = SOUNDCARD_STEP_LENGTH * Decimation_Factor;
	Doing = true;
	float* SrcBuff = (float*)AdcDataIFiltted->Buff;
	UINT SrcPosMask = AdcDataIFiltted->Mask;
	UINT DemodulattedPos = (AdcDataIFiltted->Pos - SOUNDCARD_STEP_LENGTH) & AdcDataIFiltted->Mask;
	short* Buff = (short*)m_Audio->outData->Buff;
	while (Doing && Program_In_Process) {
		am_pos = AdcDataIFiltted->Pos;
		am_between = (am_pos - DemodulattedPos) & SrcPosMask;
		if (am_between > DemodulatorStepLength) {
			double dbi = 0;
			for (int i = 0; i < SOUNDCARD_STEP_LENGTH; i++, dbi += Decimation_Factor) {
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
			//Am_Decimation_Factor = (double)AdcDataI->SampleRate / (1 << clsFilter.rootFilterInfo.decimationFactorBit) / m_Audio->SampleRate;
			//Am_Decimation_Factor += Am_Decimation_Factor_offset;
			//DemodulatorStepLength = SOUNDCARD_STEP_LENGTH * Am_Decimation_Factor;
			//break;
		}
		else {
			Sleep(10);
			continue;
		}
	}
	hThread = NULL;
}

void CDemodulatorAM::Thread_Func_Get_Envelope(void)
{
	AudioData->SampleRate = AudioDataFiltted->SampleRate = m_Audio->uSampleRate = clsMainFilterI.rootFilterInfo1.nextFilter->FreqCenter;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	clsAudioFilter.ParseCoreDesc();
	Doing = true;
	float* SrcBuff = (float*)AdcDataIFiltted->Buff;
	UINT DemodulattedPos = (AdcDataIFiltted->Pos - SOUNDCARD_STEP_LENGTH) & AdcDataIFiltted->Mask;
	m_Audio->outData->Pos = 0;
	short* TargetBuff = (short*)m_Audio->outData->Buff;
	CData* out = m_Audio->outDataFiltted;
	while (Doing && Program_In_Process) {
		if (((AdcDataIFiltted->Pos - DemodulattedPos) & AdcDataIFiltted->Mask) > SOUNDCARD_STEP_LENGTH) {
			double dbi = 0;
			for (int i = 0; i < SOUNDCARD_STEP_LENGTH; i++) {
				if (
					SrcBuff[DemodulattedPos] > SrcBuff[(DemodulattedPos - 1) & AdcDataIFiltted->Mask] &&
					SrcBuff[DemodulattedPos] > SrcBuff[(DemodulattedPos + 1) & AdcDataIFiltted->Mask]
					) {
					TargetBuff[m_Audio->outData->Pos++] = SrcBuff[DemodulattedPos] * m_Audio->Am_zoom;
					m_Audio->outData->Pos &= m_Audio->outData->Mask;
				}
				DemodulattedPos++;
				DemodulattedPos &= AdcDataIFiltted->Mask;
			}
		}
		else {
			Sleep(10);
		}
	}
	hThread = NULL;
}


void CDemodulatorAM::Thread_Func_IQ(void)
{
	CData* audioData = m_Audio->outData;
	CData* iData = AdcDataIFiltted;
	CData* qData = AdcDataQFiltted;

	
	UINT df = 0;
	while ((iData->SampleRate >> df) > (1 << 15)) df++;
	UINT Decimation_Factor = 1 << df;

	m_Audio->outData->SampleRate = m_Audio->outDataFiltted->SampleRate = m_Audio->uSampleRate = iData->SampleRate >> df;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	clsAudioFilter.ParseCoreDesc();

	UINT am_pos = iData->Pos;

	//WaitForSingleObject(clsAudioFilter.hCoreMutex, INFINITE);
	//audioData->Pos = audioData->ProcessPos = 0;
	//ReleaseMutex(clsAudioFilter.hCoreMutex);

	float *audioBuff = (float*)m_Audio->outData->Buff;
	float* xi = (float*)iData->Buff;
	float* xq = (float*)AdcDataQFiltted->Buff;

	Doing = true;

	while (Doing && Program_In_Process) {
		if (
			((iData->Pos - am_pos) & iData->Mask) > Decimation_Factor &&
			((qData->Pos - am_pos) & iData->Mask) > Decimation_Factor
			) {
			UINT pos = am_pos + Decimation_Factor;
			//audioBuff[audioData->Pos] = sqrt(xi[pos] * xi[pos] + xq[pos] * xq[pos]);
			audioBuff[audioData->Pos] = xi[pos];
			am_pos += Decimation_Factor ;
			am_pos &= iData->Mask;
			audioData->Pos++;
			audioData->Pos &= audioData->Mask;
		}
		else {
			Sleep(10);
		}
	}
	hThread = NULL;
}