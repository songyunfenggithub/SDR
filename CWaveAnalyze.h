
#pragma once

#include "CSDR.h"
#include "SDRPlay_API.3.09/API/inc/sdrplay_api.h"


class CWaveAnalyze
{
public:
	//double rfHz = 10000000.0;
	double rfHz_Step = 1000.0;

public:
	CWaveAnalyze();
	~CWaveAnalyze();

	void Init_Params(void);

	void set_SDR_SampleRate(UINT Rate);
	void set_SDR_decimationFactorEnable(UCHAR Enable);
	void set_SDR_decimationFactor(sdrplay_api_decimationFactorT decimationFactor);
	void set_SDR_bwType_Bw_MHzT(sdrplay_api_Bw_MHzT bwType_Bw_MHzT);
	void set_SDR_rfHz(double rfHz);

	void set_FFT_FFTSize(UINT FFTSize, UINT FFTStep);

	static TIMERPROC PerSecTimer_Func(void);
};

extern CWaveAnalyze clsWaveAnalyze;
