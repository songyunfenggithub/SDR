
#include "stdafx.h"
#include "resource.h"
#include <iostream>

#include "public.h"

#include "CAudio.h"

#include "CFM.h"

using namespace DEMODULATOR;

CFM::CFM()
{
	RestoreValue();
	Init();
}

CFM::~CFM()
{
	SaveValue();
	UnInit();
}

void CFM::Init(void)
{
	m_Audio = &mAudio;
}

void CFM::UnInit(void)
{

}

void CFM::SaveValue(void)
{
	//WritePrivateProfileString("CDemodulatorFM", "AMFilterCoreDesc", pFilterInfo->CoreDescStr, IniFilePath);
}

void CFM::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	//GetPrivateProfileString("CDemodulatorFM", "AMFilterCoreDesc", "1", AMFilterCoreDesc, FILTER_DESC_LENGTH, IniFilePath);
}

LPTHREAD_START_ROUTINE CFM::Demodulator_Thread(LPVOID lp)
{
	CFM* me = (CFM*)lp;
	me->Thread_Func_Calculation_Frequency();
	return 0;
}

void CFM::Thread_Func(void)
{
	UINT fm_pos;
	UINT fm_between;
	double Am_Decimation_Factor_offset = 0.0;
	m_Audio->uSampleRate = (double)AdcDataI->SampleRate / (1 << clsMainFilterI.rootFilterInfo1.decimationFactorBit) / 4;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	UINT DemodulatorStepLength = SOUNDCARD_STEP_LENGTH;
	Doing = true;
	float* SrcBuff = (float*)AdcDataIFiltted->Buff;
	UINT SrcPosMask = AdcDataIFiltted->Mask;
	UINT DemodulattedPos = (AdcDataIFiltted->Pos - SOUNDCARD_STEP_LENGTH) & AdcDataIFiltted->Mask;
	short* Buff = (short*)m_Audio->outData->Buff;
	while (Doing && Program_In_Process) {
		fm_pos = AdcDataIFiltted->Pos;
		fm_between = (fm_pos - DemodulattedPos) & SrcPosMask;
		if (fm_between > DemodulatorStepLength) {
			double dbi = 0;
			for (int i = 0; i < SOUNDCARD_STEP_LENGTH; i++, dbi += 0) {
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
		}
		else {
			Sleep(10);
			continue;
		}
	}
	Doing = false;
	hThread = NULL;
}

void CFM::Thread_Func_IQ(void)
{
	CData* audioData = m_Audio->outData;
	CData* iData = AdcDataIFiltted;
	//CData* qData = AdcDataQFiltted;

	float dt = 1.0 / iData->SampleRate;
	UINT df = 0;
	while ((iData->SampleRate >> df) > (1 << 15)) df++;
	UINT Decimation_Factor = 1 << df;

	m_Audio->outData->SampleRate = m_Audio->outDataFiltted->SampleRate = m_Audio->uSampleRate = iData->SampleRate >> df;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	clsAudioFilter.ParseCoreDesc();

	UINT fm_pos = iData->Pos;

	float* audioBuff = (float*)m_Audio->outData->Buff;
	float* xi = (float*)iData->Buff;
	//float* xq = (float*)AdcDataQFiltted->Buff;

	Doing = true;

	while (Doing && Program_In_Process) {
		if (((iData->Pos - fm_pos) & iData->Mask) > Decimation_Factor) {
			UINT pos = fm_pos;

			//UINT pos_1 = (pos - 1) & iData->Mask;
			//if (xi[pos] == 0 && xq[pos] == 0) 
			//	audioBuff[audioData->Pos] = audioBuff[(audioData->Pos - 1) & AudioData->Mask];
			//else 
			//	audioBuff[audioData->Pos] = (xi[pos_1] * xq[pos] - xi[pos] * xq[pos_1]) / (xi[pos] * xi[pos] + xq[pos] * xq[pos]);

			//if (xi[pos] == 0)
			//	audioBuff[audioData->Pos] = audioBuff[(audioData->Pos - 1) & AudioData->Mask];
			//else
			//	audioBuff[audioData->Pos] = atan(xq[pos] / xi[pos]) *1000;


			//audioBuff[audioData->Pos] = sqrt(xi[pos] * xi[pos] + xq[pos] * xq[pos]);
 
			audioBuff[audioData->Pos] = xi[pos];

			fm_pos = (fm_pos + Decimation_Factor) & iData->Mask;
			audioData->Pos = ++audioData->Pos & audioData->Mask;
		}
		else {
			Sleep(10);
		}
	}
	hThread = NULL;
}

void CFM::Thread_Func_Calculation_Frequency(void)
{
	CData* oData = m_Audio->outData;
	CData* iData = AdcDataIFiltted;

	float dt = 1.0 / iData->SampleRate;
	UINT df = 0;
	while ((iData->SampleRate >> df) > (1 << 15)) df++;
	UINT Decimation_Factor = 1 << df;

	m_Audio->outData->SampleRate = m_Audio->outDataFiltted->SampleRate = m_Audio->uSampleRate = iData->SampleRate >> df;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	clsAudioFilter.ParseCoreDesc();

	UINT fm_pos = iData->Pos;

	float* audioBuff = (float*)m_Audio->outData->Buff;
	float* xi = (float*)iData->Buff;

	Doing = true;
	while (Doing && Program_In_Process) {
		if (((iData->Pos - fm_pos) & iData->Mask) > Decimation_Factor) {
			UINT pos = fm_pos;
			audioBuff[oData->Pos] = xi[pos];
			fm_pos = (fm_pos + Decimation_Factor) & iData->Mask;
			oData->Pos = ++oData->Pos & oData->Mask;
		}
		else {
			Sleep(10);
		}
	}
	hThread = NULL;
}