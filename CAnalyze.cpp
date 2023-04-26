#include "stdafx.h"

#include "CAnalyze.h"
#include "CData.h"

#include "CWaveFFT.h"
#include "CWinOneFFT.h"

#include "CDataFromSDR.h"

#include "CWaveFilter.h"
#include "cuda_CFilter.cuh"
#include "cuda_CFilter2.cuh"
#include "CDemodulatorAM.h"

#pragma comment(lib,"SDRPlay_API.3.09/API/x64/sdrplay_api.lib")

CAnalyze clsAnalyze;

CAnalyze::CAnalyze()
{

}

CAnalyze::~CAnalyze()
{

}

void CAnalyze::set_SDR_Params_Update(int index)
{
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
		//if (SDR_params[index].p_set_params_func != NULL)SDR_params[index].p_set_params_func(index);
	}
}

void CAnalyze::set_SDR_SampleRate(UINT Rate)
{
	int index = clsSDR.get_ValueIndex("fsHz double");
	*(double*)SDR_params[index].pValue = Rate;
	set_SDR_Params_Update(index);
	if (SDR_params[index].p_set_params_func != NULL)SDR_params[index].p_set_params_func(index);
}

void CAnalyze::set_SDR_AgcControlenable(sdrplay_api_AgcControlT AgcControl)
{
	int index = clsSDR.get_ValueIndex("enable sdrplay_api_AgcControlT");
	*(int*)SDR_params[index].pValue = (int)AgcControl;
	set_SDR_Params_Update(index);
}

void CAnalyze::set_SDR_AgcControl_setPoint_dBfs(int AgcDB)
{
	int index = clsSDR.get_ValueIndex("setPoint_dBfs int");
	*(int*)SDR_params[index].pValue = (int)AgcDB;
	set_SDR_Params_Update(index);
}

void CAnalyze::set_SDR_decimationFactorEnable(UCHAR Enable)
{
	int index = clsSDR.get_ValueIndex("enable decimationFactor unsigned char");
	*(UCHAR*)SDR_params[index].pValue = Enable;
	set_SDR_Params_Update(index);
}

void CAnalyze::set_SDR_decimationFactor(sdrplay_api_decimationFactorT decimationFactor)
{
	int index = clsSDR.get_ValueIndex("decimationFactor unsigned char");
	*(UCHAR*)SDR_params[index].pValue = (UCHAR)decimationFactor;
	set_SDR_Params_Update(index);
}

void CAnalyze::set_SDR_bwType_Bw_MHzT(sdrplay_api_Bw_MHzT bwType_Bw_MHzT)
{
	int index = clsSDR.get_ValueIndex("bwType sdrplay_api_Bw_MHzT");
	*(int*)SDR_params[index].pValue = (int)bwType_Bw_MHzT;
	set_SDR_Params_Update(index);
}

void CAnalyze::set_SDR_rfHz(double rfHz)
{
	int index = clsSDR.get_ValueIndex("rfHz double");
	*(double*)SDR_params[index].pValue = (double)rfHz;
	set_SDR_Params_Update(index);
}

void CAnalyze::set_FFT_FFTSize(UINT fftsize, UINT fftstep)
{
	clsWaveFFT.InitAllBuff(fftsize, fftstep);
}

#define TIMEOUT	1000
void CAnalyze::Init_Params(void)
{
	INT64 rf = 11749000;

	this->set_SDR_bwType_Bw_MHzT(sdrplay_api_Bw_MHzT::sdrplay_api_BW_0_200);
	this->set_SDR_decimationFactorEnable(1);
	this->set_SDR_decimationFactor(sdrplay_api_decimationFactorT::decimationFactor4);
	this->set_SDR_AgcControlenable(sdrplay_api_AgcControlT::sdrplay_api_AGC_50HZ);
	this->set_SDR_AgcControl_setPoint_dBfs(-30);
	this->set_SDR_rfHz(rf);
	
	clsWinOneFFT.rfButton->RefreshMouseNumButton(rf);
	clsWinOneFFT.rfStepButton->RefreshMouseNumButton(rfHz_Step);

	clsWaveFFT.FFTSize = 0x100000;
	clsWaveFFT.FFTStep = 0x40000;

	this->set_SDR_SampleRate(2000000);
	clsWaveFilter.ParseCoreDesc(&clsWaveFilter.rootFilterInfo);

	//clsDemodulatorAm.build_AM_Filter_Core();
	//clsWaveFilter.ParseCoreDesc(clsDemodulatorAm.pFilterInfo, clsData.AdcSampleRate);
	//clsDemodulatorAm.pFilterInfo->decimationFactorBit = DEMODULATOR_AM_DECIMATION_FACTOR_BIT;
	//clsWaveFilter.ParseCoreDesc(clsDemodulatorAm.pFilterInfo);

	SetTimer(NULL, 0, TIMEOUT, (TIMERPROC)CAnalyze::PerSecTimer_Func);
}

TIMERPROC CAnalyze::PerSecTimer_Func(void)
{
	static UINT tick = 0;
	if ((tick % 10) == 0)
	{
		if (clsWinOneFFT.rfStepButton->Button->value > 0) {
			clsWinOneFFT.rfButton->RefreshMouseNumButton(
				clsGetDataSDR.chParams->tunerParams.rfFreq.rfHz + clsWinOneFFT.rfStepButton->Button->value);
			clsAnalyze.set_SDR_rfHz(clsWinOneFFT.rfButton->Button->value);
		}
	}
	tick++;
	return 0;
}
