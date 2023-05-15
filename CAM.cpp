
#include "stdafx.h"
#include "resource.h"
#include <iostream>

#define _USE_MATH_DEFINES
#include <math.h>

#include "public.h"

#include "CAudio.h"
#include "CFilter.h"

#include "CAM.h"

using namespace DEMODULATOR;

CAM::CAM()
{
	RestoreValue();
	Init();
}

CAM::~CAM()
{
	SaveValue();
	UnInit();
}

void CAM::Init(void)
{
	m_Audio = &mAudio;
}

void CAM::UnInit(void)
{

}

void CAM::SaveValue(void)
{
	//WritePrivateProfileString("CDemodulatorAM", "AMFilterCoreDesc", pFilterInfo->CoreDescStr, IniFilePath);
}

void CAM::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	//GetPrivateProfileString("CDemodulatorAM", "AMFilterCoreDesc", "1", AMFilterCoreDesc, FILTER_DESC_LENGTH, IniFilePath);
}

LPTHREAD_START_ROUTINE CAM::AM_Demodulator_Thread(LPVOID lp)
{
	CAM* me = (CAM*)lp;
	//me->AM_Demodulator_Thread_Func();
	switch (me->AmMode) {
	case AM_DEMODULATOR_MODE::Am_Move_Frequency_Mode:
		me->Thread_Func_Move_Frequency();
		break;
	case AM_DEMODULATOR_MODE::Am_IQ_Mode:
		me->Thread_Func_IQ();
		break;
	case AM_DEMODULATOR_MODE::Am_Get_Envelope_Mode:
		me->Thread_Func_Get_Envelope();
		break;
	}
	return 0;
}

void CAM::Thread_Func(void)
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

void CAM::Thread_Func_Get_Envelope(void)
{
	AudioData->SampleRate = AudioDataFiltted->SampleRate = m_Audio->uSampleRate =
		clsMainFilterI.Cuda_Filter_N_Doing == CFilter::CUDA_FILTER_N::cuda_filter_3 ?
		clsMainFilterI.rootFilterInfo2.nextFilter->FreqCenter : clsMainFilterI.rootFilterInfo1.nextFilter->FreqCenter;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	clsAudioFilter.ParseCoreDesc();

	Doing = true;
	float* SrcBuff = (float*)AdcDataIFiltted->Buff;
	UINT DemodulattedPos = (AdcDataIFiltted->Pos - SOUNDCARD_STEP_LENGTH) & AdcDataIFiltted->Mask;

	m_Audio->outData->Pos = 0;
	float* TargetBuff = (float*)m_Audio->outData->Buff;
	while (Doing && Program_In_Process) {
		if (((AdcDataIFiltted->Pos - DemodulattedPos) & AdcDataIFiltted->Mask) > SOUNDCARD_STEP_LENGTH) {
			for (int i = 0; i < SOUNDCARD_STEP_LENGTH; i++) {
				if (
					SrcBuff[DemodulattedPos] > SrcBuff[(DemodulattedPos - 1) & AdcDataIFiltted->Mask] &&
					SrcBuff[DemodulattedPos] > SrcBuff[(DemodulattedPos + 1) & AdcDataIFiltted->Mask]
					) {
					TargetBuff[m_Audio->outData->Pos] = SrcBuff[DemodulattedPos] * m_Audio->Am_zoom;
					m_Audio->outData->Pos = (m_Audio->outData->Pos + 1) & m_Audio->outData->Mask;
				}
				DemodulattedPos = (DemodulattedPos + 1) & AdcDataIFiltted->Mask;
			}
		}
		else {
			Sleep(10);
		}
	}
	hThread = NULL;
}



void CAM::Thread_Func_Move_Frequency(void)
{
	CData* iData = AdcDataIFiltted;
	//CData* qData = AdcDataQFiltted;
	CData* oData = m_Audio->outData;

	UINT bit = 0;
	while ((iData->SampleRate >> bit) > (1 << 15)) bit++;
	UINT Decimation_Factor = 1 << bit;

	float SampleRate = AudioData->SampleRate = AudioDataFiltted->SampleRate = m_Audio->uSampleRate = iData->SampleRate >> bit;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	clsAudioFilter.ParseCoreDesc();

	float* xi = (float*)iData->Buff;
	//float* xq = (float*)qData->Buff;
	float* oBuff = (float*)oData->Buff; 

	iData->ProcessPos = iData->Pos;

	float f0 = clsMainFilterI.rootFilterInfo2.nextFilter->FreqCenter;
	UINT fPos = 0;

	float Ffactor = 2 * M_PI * f0 / iData->SampleRate;
	Doing = true;
	UINT ip = iData->Pos;
	//UINT qp = qData->Pos;
	iData->ProcessPos = ip;

	while (Doing && Program_In_Process) {
		if (iData->Pos != iData->ProcessPos) {
			UINT pos = iData->ProcessPos;
			float v = xi[pos];
			oBuff[oData->Pos] = cos(Ffactor * (fPos += Decimation_Factor)) * v;
			iData->ProcessPos = (iData->ProcessPos + Decimation_Factor) & iData->Mask;
			oData->Pos = (oData->Pos + 1) & oData->Mask;
		}
		else {
			Sleep(10);
		}
	}
	hThread = NULL;
}



void CAM::Thread_Func_Move_Frequency_f0(void)
{
	CData* iData = AdcDataIFiltted;
	//CData* qData = AdcDataQFiltted;
	CData* oData = m_Audio->outData;

	UINT bit = 0;
	while ((iData->SampleRate >> bit) > (1 << 15)) bit++;
	UINT Decimation_Factor = 1 << bit;

	float SampleRate = AudioData->SampleRate = AudioDataFiltted->SampleRate = m_Audio->uSampleRate = iData->SampleRate >> bit;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	clsAudioFilter.ParseCoreDesc();

	float* xi = (float*)iData->Buff;
	//float* xq = (float*)qData->Buff;
	float* oBuff = (float*)oData->Buff;

	iData->ProcessPos = iData->Pos;

	UINT fPos = 0;
	float f0 = 13000;// Move_Frequency_f0;
	float Ffactor = 2 * M_PI * f0 / iData->SampleRate;

	Doing = true;
	UINT ip = iData->Pos;
	//UINT qp = qData->Pos;
	iData->ProcessPos = ip;

	while (Doing && Program_In_Process) {
		if (iData->Pos != iData->ProcessPos) {
			UINT pos = iData->ProcessPos;
			float v = xi[pos];
			oBuff[oData->Pos] = cos(Ffactor * (fPos += Decimation_Factor)) * v;
			iData->ProcessPos = (iData->ProcessPos + Decimation_Factor) & iData->Mask;
			oData->Pos = (oData->Pos + 1) & oData->Mask;
		}
		else {
			Sleep(10);
		}
	}
	hThread = NULL;
}

void CAM::Thread_Func_IQ(void)
{
	CData* audioData = m_Audio->outData;
	CData* iData = AdcDataIFiltted;
	//CData* qData = AdcDataQFiltted;

	UINT bit = 0;
	while ((iData->SampleRate >> bit) > (1 << 15)) bit++;
	UINT Decimation_Factor = 1 << bit;

	m_Audio->outData->SampleRate = m_Audio->outDataFiltted->SampleRate = m_Audio->uSampleRate = iData->SampleRate >> bit;
	m_Audio->SampleRate = &m_Audio->uSampleRate;
	clsAudioFilter.ParseCoreDesc();

	iData->ProcessPos = iData->Pos;

	//WaitForSingleObject(clsAudioFilter.hCoreMutex, INFINITE);
	//audioData->Pos = audioData->ProcessPos = 0;
	//ReleaseMutex(clsAudioFilter.hCoreMutex);

	float *audioBuff = (float*)m_Audio->outData->Buff;
	float* xi = (float*)iData->Buff;
	//float* xq = (float*)AdcDataQFiltted->Buff;

	Doing = true;
	UINT ip = iData->Pos;
	//UINT qp = qData->Pos;
	iData->ProcessPos = ip;

	while (Doing && Program_In_Process) {
		if (iData->Pos != iData->ProcessPos) {
			UINT pos = iData->ProcessPos;
			//audioBuff[audioData->Pos] = sqrt(xi[pos] * xi[pos] + xq[pos] * xq[pos]);
			audioBuff[audioData->Pos] = xi[pos];
			iData->ProcessPos = (iData->ProcessPos + Decimation_Factor) & iData->Mask;
			audioData->Pos = (audioData->Pos + 1) & audioData->Mask;
		}
		else {
			Sleep(10);
		}
	}
	hThread = NULL;
}