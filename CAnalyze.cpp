#include "stdafx.h"

#include "CAnalyze.h"
#include "CData.h"

#include "CDataFromSDR.h"
#include "CFFT.h"
#include "CDemodulatorAM.h"

#include "CWinSpectrum.h"
#include "CWinOneFFT.h"

#include "CFilter.h"
#include "cuda_CFilter.cuh"
#include "cuda_CFilter2.cuh"
#include "CDemodulatorAM.h"

#pragma comment(lib,"SDRPlay_API.3.09/API/x64/sdrplay_api.lib")

using namespace WINS;
using namespace DEVICES;

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
	//if (SDR_params[index].p_set_params_func != NULL)SDR_params[index].p_set_params_func(index);
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
	//clsWaveFFT.InitAllBuff(fftsize, fftstep);
}

#define TIMEOUT	1000
void CAnalyze::Init_Params(void)
{
	UINT sampleRate = 8000000;
	AdcData = new CData();
	AdcData->Init(DATA_BUFFER_LENGTH, short_type, ADC_DATA_SAMPLE_BIT);
	AdcData->SampleRate = sampleRate;
	AdcDataFiltted = new CData();
	AdcDataFiltted->Init(DATA_BUFFER_LENGTH, float_type, ADC_DATA_SAMPLE_BIT);

	INT64 rf = 11000000;
	this->set_SDR_bwType_Bw_MHzT(sdrplay_api_Bw_MHzT::sdrplay_api_BW_0_200);
	this->set_SDR_decimationFactorEnable(1);
	this->set_SDR_decimationFactor(sdrplay_api_decimationFactorT::decimationFactor4);
	this->set_SDR_AgcControlenable(sdrplay_api_AgcControlT::sdrplay_api_AGC_50HZ);
	this->set_SDR_AgcControl_setPoint_dBfs(-20);
	this->set_SDR_rfHz(rf);

	clsWinOneFFT.rfButton->RefreshMouseNumButton(rf);
	clsWinOneFFT.rfStepButton->RefreshMouseNumButton(rfHz_Step);

	this->set_SDR_SampleRate(sampleRate);
	AdcData->SampleRate = clsGetDataSDR.chParams->ctrlParams.decimation.enable != 0 ?
		clsGetDataSDR.deviceParams->devParams->fsFreq.fsHz / clsGetDataSDR.chParams->ctrlParams.decimation.decimationFactor :
		clsGetDataSDR.deviceParams->devParams->fsFreq.fsHz;
	clsMainFilter.rootFilterInfo.SampleRate = AdcData->SampleRate / (1 << clsMainFilter.rootFilterInfo.decimationFactorBit);

	clsMainFilter.SrcData = AdcData;
	clsMainFilter.TargetData = AdcDataFiltted;
	clsMainFilter.set_cudaFilter(&clscudaMainFilter, &clscudaMainFilter2, CUDA_FILTER_ADC_BUFF_SRC_LENGTH);
	clsMainFilter.ParseCoreDesc(&clsMainFilter.rootFilterInfo);

	FFTInfo_Signal.FFTSize = FFT_SIZE;
	FFTInfo_Signal.HalfFFTSize = FFT_SIZE / 2;
	FFTInfo_Signal.FFTStep = FFT_STEP;
	FFTInfo_Signal.AverageDeep = FFT_DEEP;

	FFTInfo_Filtted.FFTSize = FFT_SIZE >> clsMainFilter.rootFilterInfo.decimationFactorBit;
	FFTInfo_Filtted.HalfFFTSize = FFTInfo_Filtted.FFTSize / 2;
	FFTInfo_Filtted.FFTStep = FFT_STEP >> clsMainFilter.rootFilterInfo.decimationFactorBit;
	FFTInfo_Filtted.AverageDeep = FFT_DEEP;

	clsWinSpect.FFTOrignal = new CFFT();
	clsWinSpect.FFTOrignal->Data = AdcData;

	clsWinSpect.FFTOrignal->FFTInfo = &FFTInfo_Signal;

	clsWinSpect.FFTFiltted = new CFFT();
	clsWinSpect.FFTFiltted->Data = AdcDataFiltted;
	clsWinSpect.FFTFiltted->FFTInfo = &FFTInfo_Filtted;

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

		FFTInfo_Signal.FFTPerSec = (float)(FFTInfo_Signal.FFTCount - FFTInfo_Signal.SavedFFTCount) / 10.0;
		FFTInfo_Signal.SavedFFTCount = FFTInfo_Signal.FFTCount;

		FFTInfo_Filtted.FFTPerSec = (float)(FFTInfo_Filtted.FFTCount - FFTInfo_Filtted.SavedFFTCount) / 10.0;
		FFTInfo_Filtted.SavedFFTCount = FFTInfo_Filtted.FFTCount;

		FFTInfo_Audio.FFTPerSec = (float)(FFTInfo_Audio.FFTCount - FFTInfo_Audio.SavedFFTCount) / 10.0;
		FFTInfo_Audio.SavedFFTCount = FFTInfo_Audio.FFTCount;

		FFTInfo_AudioFiltted.FFTPerSec = (FFTInfo_AudioFiltted.FFTCount - FFTInfo_AudioFiltted.SavedFFTCount) / 10.0;
		FFTInfo_AudioFiltted.SavedFFTCount = FFTInfo_AudioFiltted.FFTCount;
	}
	if (AdcData != NULL) {
		AdcData->NumPerSec = (AdcData->Pos - AdcData->SavedPos) & AdcData->Mask;
		AdcData->SavedPos = AdcData->Pos;
	}

	if (AdcDataFiltted != NULL) {
		AdcDataFiltted->NumPerSec = (AdcDataFiltted->Pos - AdcDataFiltted->SavedPos) & AdcDataFiltted->Mask;
		AdcDataFiltted->SavedPos = AdcDataFiltted->Pos;
	}
	if (AudioData != NULL) {
		AudioData->NumPerSec = (AudioData->Pos - AudioData->SavedPos) & AudioData->Mask;
		AudioData->SavedPos = AudioData->Pos;
	}

	if (AudioDataFiltted != NULL) {
		AudioDataFiltted->NumPerSec = (AudioDataFiltted->Pos - AudioDataFiltted->SavedPos) & AudioDataFiltted->Mask;
		AudioDataFiltted->SavedPos = AudioDataFiltted->Pos;
	}

	tick++;
	return 0;
}
