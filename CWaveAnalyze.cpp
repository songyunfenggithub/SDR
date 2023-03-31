#include "stdafx.h"

#include "CWaveAnalyze.h"
#include "CWaveData.h"

#include "CWaveFFT.h"
#include "CWinOneFFT.h"

#include "CDataFromSDR.h"

#include "CWaveFilter.h"
#include "cuda_Filter.cuh"
#include "cuda_AM_Filter.cuh"
#include "CDemodulatorAM.h"

#pragma comment(lib,"SDRPlay_API.3.09/API/x64/sdrplay_api.lib")

CWaveAnalyze clsWaveAnalyze;

CWaveAnalyze::CWaveAnalyze()
{

}

CWaveAnalyze::~CWaveAnalyze()
{

}

void CWaveAnalyze::set_SDR_SampleRate(UINT Rate)
{
	int index = clsSDR.get_ValueIndex("fsHz double");
	*(double*)SDR_params[index].pValue = Rate;
	sdrplay_api_ErrT err;
	if ((err = sdrplay_api_Update(clsGetDataSDR.chosenDevice->dev, clsGetDataSDR.chosenDevice->tuner,
		(sdrplay_api_ReasonForUpdateT)SDR_params[index].paramUpdateReason, sdrplay_api_Update_Ext1_None)) !=
		sdrplay_api_Success)
	{
		printf("sdrplay_api_Update %d failed %s\n",
			SDR_params[index].paramUpdateReason,
			sdrplay_api_GetErrorString(err));
	}
	else
	{
		if (SDR_params[index].p_set_params_func != NULL)SDR_params[index].p_set_params_func(index);
	}
}

void CWaveAnalyze::set_SDR_decimationFactorEnable(UCHAR Enable)
{
	int index = clsSDR.get_ValueIndex("enable decimationFactor unsigned char");
	*(UCHAR*)SDR_params[index].pValue = Enable;
	sdrplay_api_ErrT err;
	if ((err = sdrplay_api_Update(clsGetDataSDR.chosenDevice->dev, clsGetDataSDR.chosenDevice->tuner,
		(sdrplay_api_ReasonForUpdateT)SDR_params[index].paramUpdateReason, sdrplay_api_Update_Ext1_None)) !=
		sdrplay_api_Success)
	{
		printf("sdrplay_api_Update %d failed %s\n",
			SDR_params[index].paramUpdateReason,
			sdrplay_api_GetErrorString(err));
	}
	else
	{
		if (SDR_params[index].p_set_params_func != NULL)SDR_params[index].p_set_params_func(index);
	}
}

void CWaveAnalyze::set_SDR_decimationFactor(sdrplay_api_decimationFactorT decimationFactor)
{
	int index = clsSDR.get_ValueIndex("decimationFactor unsigned char");
	*(UCHAR*)SDR_params[index].pValue = (UCHAR)decimationFactor;
	sdrplay_api_ErrT err;
	if ((err = sdrplay_api_Update(clsGetDataSDR.chosenDevice->dev, clsGetDataSDR.chosenDevice->tuner,
		(sdrplay_api_ReasonForUpdateT)SDR_params[index].paramUpdateReason, sdrplay_api_Update_Ext1_None)) !=
		sdrplay_api_Success)
	{
		printf("sdrplay_api_Update %d failed %s\n",
			SDR_params[index].paramUpdateReason,
			sdrplay_api_GetErrorString(err));
	}
	else
	{
		if (SDR_params[index].p_set_params_func != NULL)SDR_params[index].p_set_params_func(index);
	}
}

void CWaveAnalyze::set_SDR_bwType_Bw_MHzT(sdrplay_api_Bw_MHzT bwType_Bw_MHzT)
{
	int index = clsSDR.get_ValueIndex("bwType sdrplay_api_Bw_MHzT");
	*(int*)SDR_params[index].pValue = (int)bwType_Bw_MHzT;
	sdrplay_api_ErrT err;
	if ((err = sdrplay_api_Update(clsGetDataSDR.chosenDevice->dev, clsGetDataSDR.chosenDevice->tuner,
		(sdrplay_api_ReasonForUpdateT)SDR_params[index].paramUpdateReason, sdrplay_api_Update_Ext1_None)) !=
		sdrplay_api_Success)
	{
		printf("sdrplay_api_Update %d failed %s\n",
			SDR_params[index].paramUpdateReason,
			sdrplay_api_GetErrorString(err));
	}
	else
	{
		if (SDR_params[index].p_set_params_func != NULL)SDR_params[index].p_set_params_func(index);
	}
}

void CWaveAnalyze::set_SDR_rfHz(double rfHz)
{
	int index = clsSDR.get_ValueIndex("rfHz double");
	*(double*)SDR_params[index].pValue = (double)rfHz;
	sdrplay_api_ErrT err;
	if ((err = sdrplay_api_Update(clsGetDataSDR.chosenDevice->dev, clsGetDataSDR.chosenDevice->tuner,
		(sdrplay_api_ReasonForUpdateT)SDR_params[index].paramUpdateReason, sdrplay_api_Update_Ext1_None)) !=
		sdrplay_api_Success)
	{
		printf("sdrplay_api_Update %d failed %s\n",
			SDR_params[index].paramUpdateReason,
			sdrplay_api_GetErrorString(err));
	}
	else
	{
		if (SDR_params[index].p_set_params_func != NULL)SDR_params[index].p_set_params_func(index);
	}
}

void CWaveAnalyze::set_FFT_FFTSize(UINT FFTSize, UINT FFTStep)
{
	clsWaveFFT.InitAllBuff(FFTSize, FFTStep);
}

#define TIMEOUT	1000
void CWaveAnalyze::Init_Params(void)
{
	INT64 rf = 11000000;
	this->set_FFT_FFTSize(0x100000, 0x10000);
	this->set_SDR_bwType_Bw_MHzT(sdrplay_api_Bw_MHzT::sdrplay_api_BW_0_200);
	this->set_SDR_decimationFactorEnable(1);
	this->set_SDR_decimationFactor(sdrplay_api_decimationFactorT::decimationFactor4);
	this->set_SDR_SampleRate(2000000);
	this->set_SDR_rfHz(rf);
	clsWinOneFFT.rfButtonRefresh(clsWinOneFFT.rfButton, rf);
	clsWinOneFFT.rfButtonRefresh(clsWinOneFFT.rfStepButton, rfHz_Step);

	Cuda_Filter_Init();
	
	//clsWaveFilter.ParseCoreDesc(&clsWaveFilter.rootFilterInfo, clsWaveData.AdcSampleRate);

	clsDemodulatorAm.build_AM_Filter_Core();
//	clsWaveFilter.ParseCoreDesc(clsDemodulatorAm.pFilterInfo, clsWaveData.AdcSampleRate);
	clsDemodulatorAm.pFilterInfo->decimationFactor = DEMODULATOR_AM_DECIMATION_FACTOR;
	clsWaveFilter.ParseCoreDesc(clsDemodulatorAm.pFilterInfo);


	SetTimer(NULL, 0, TIMEOUT, (TIMERPROC)CWaveAnalyze::PerSecTimer_Func);

}

TIMERPROC CWaveAnalyze::PerSecTimer_Func(void)
{
	static UINT tick = 0;

	if ((tick % 10) == 0)
	{

		clsWaveAnalyze.set_SDR_rfHz(clsGetDataSDR.chParams->tunerParams.rfFreq.rfHz + clsWaveAnalyze.rfHz_Step);
		clsWinOneFFT.rfButtonRefresh(clsWinOneFFT.rfButton, clsGetDataSDR.chParams->tunerParams.rfFreq.rfHz);

	}
	
	tick++;

	return 0;
}
