

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <exception>
#include "../API/inc/sdrplay_api.h"

#include "mycode.h"

extern sdrplay_api_CallbackFnsT cbFns;


void UpdateParams(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_ReasonForUpdateT reasonForUpdate)
{
	sdrplay_api_ErrT err;
	try {
		if ((err = sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner,
			reasonForUpdate, sdrplay_api_Update_Ext1_None)) !=
			sdrplay_api_Success)
		{
			printf("sdrplay_api_Update %d failed %s\n",
				reasonForUpdate,
				sdrplay_api_GetErrorString(err));
		}
	}
	catch (const char* &e) {

	}
}

char str_DevParams[] = "sdrplay_api_DevParamsT = \r\n"\
"{\r\n"\
"	double ppm = %lf; \r\n"\
"	sdrplay_api_FsFreqT fsFreq = {\r\n"\
"		double fsHz = %lf;\r\n"\
"		unsigned char syncUpdate = %uc;\r\n"\
"		unsigned char reCal = %uc;\r\n"\
"		}\r\n"\
"	sdrplay_api_SyncUpdateT syncUpdate = {\r\n"\
"		unsigned int sampleNum = %u;\r\n"\
"		unsigned int period = %u;\r\n"\
"		}\r\n"\
"	sdrplay_api_ResetFlagsT resetFlags = {\r\n"\
"		unsigned char resetGainUpdate = %uc;\r\n"\
"		unsigned char resetRfUpdate = %uc;\r\n"\
"		unsigned char resetFsUpdate = %uc;\r\n"\
"		}\r\n"\
"	sdrplay_api_TransferModeT mode = %d;   sdrplay_api_ISOCH = 0, sdrplay_api_BULK = 1\r\n"\
"	unsigned int samplesPerPkt = %u;       default: 0 (output param)\r\n"\
"	sdrplay_api_Rsp1aParamsT rsp1aParams;\r\n"\
"	sdrplay_api_Rsp2ParamsT rsp2Params;\r\n"\
"	sdrplay_api_RspDuoParamsT rspDuoParams;\r\n"\
"	sdrplay_api_RspDxParamsT rspDxParams;\r\n"\
"}\r\n\r\n";

char str_RxChannelParams[] = "sdrplay_api_RxChannelParamsT =\r\n"\
"{\r\n"\
"   sdrplay_api_TunerParamsT        tunerParams; \r\n"\
"   sdrplay_api_ControlParamsT      ctrlParams;\r\n"\
"   sdrplay_api_Rsp1aTunerParamsT   rsp1aTunerParams;\r\n"\
"   sdrplay_api_Rsp2TunerParamsT    rsp2TunerParams;\r\n"\
"   sdrplay_api_RspDuoTunerParamsT  rspDuoTunerParams;\r\n"\
"   sdrplay_api_RspDxTunerParamsT   rspDxTunerParams;\r\n"\
"}\r\n\r\n";

char str_TunerParams[] = "sdrplay_api_TunerParamsT = \r\n"\
"{\r\n"\
"   sdrplay_api_Bw_MHzT bwType = %d;          // default: BW_0_200\r\n"\
"   sdrplay_api_If_kHzT ifType = %d;          // default: IF_Zero    IF_Undefined = -1,IF_Zero = 0,IF_0_450 = 450,IF_1_620 = 1620,IF_2_048 = 2048\r\n"\
"   sdrplay_api_LoModeT loMode = %d;          // default: LO_Auto    LO_Undefined = 0,_LO_Auto = 1,LO_120MHz = 2,LO_144MHz = 3,LO_168MHz = 4\r\n"\
"   sdrplay_api_GainT gain = \r\n"\
"   {\r\n"\
"       int gRdB = %d;                              // default: 50\r\n"\
"       unsigned char LNAstate = %uc;               // default: 0\r\n"\
"       unsigned char syncUpdate = %uc;             // default: 0\r\n"\
"       sdrplay_api_MinGainReductionT minGr = %d;   // default: NORMAL_MIN_GR; EXTENDED_MIN_GR = 0 (0 - 59),NORMAL_MIN_GR = 20 (20 - 59)\r\n"\
"       sdrplay_api_GainValuesT gainVals =          // output parameter\r\n"\
"		{\r\n"\
"			float curr = %lf;\r\n"\
"			float max = %lf;\r\n"\
"			float min = %lf;\r\n"\
"		}\r\n"\
"   }\r\n"\
"    sdrplay_api_RfFreqT rfFreq = \r\n"\
"   {\r\n"\
"       double rfHz = %lf;                          // default: 200000000.0\r\n"\
"       unsigned char syncUpdate = %uc;            // default: 0\r\n"\
"   }\r\n"\
"    sdrplay_api_DcOffsetTunerT dcOffsetTuner;\r\n"\
"   {\r\n"\
"       unsigned char dcCal = %uc;                 // default: 3 (Periodic mode)\r\n"\
"       unsigned char speedUp = %uc;               // default: 0 (No speedup)\r\n"\
"       int trackTime = %d;                        // default: 1    (=> time in uSec = (72 * 3 * trackTime) / 24e6       = 9uSec)\r\n"\
"       int refreshRateTime  = %d;                 // default: 2048 (=> time in uSec = (72 * 3 * refreshRateTime) / 24e6 = 18432uSec)\r\n"\
"   }\r\n"\
"}\r\n\r\n";

char str_ControlParams[] = "sdrplay_api_ControlParamsT = \r\n"\
"{\r\n"\
"   sdrplay_api_DcOffsetT dcOffset = \r\n"\
"   {\r\n"\
"       unsigned char DCenable = %uc;          // default: 1\r\n"\
"       unsigned char IQenable = %uc;          // default: 1\r\n"\
"   }\r\n"\
"   sdrplay_api_DecimationT decimation;\r\n"\
"   {\r\n"\
"       unsigned char enable = %uc;            // default: 0\r\n"\
"       unsigned char decimationFacto = %ucr;  // default: 1\r\n"\
"       unsigned char wideBandSignal = %uc;    // default: 0\r\n"\
"   }\r\n"\
"   sdrplay_api_AgcT agc = \r\n"\
"   {\r\n"\
"       sdrplay_api_AgcControlT enable = %d;    // default: sdrplay_api_AGC_50HZ; AGC_DISABLE = 0, AGC_100HZ = 1, AGC_50HZ = 2, AGC_5HZ = 3, AGC_CTRL_EN = 4;\r\n"\
"       int setPoint_dBfs = %d;                 // default: -60\r\n"\
"       unsigned short attack_ms = %d;          // default: 0\r\n"\
"       unsigned short decay_ms = %d;           // default: 0\r\n"\
"       unsigned short decay_delay_ms = %d;     // default: 0\r\n"\
"       unsigned short decay_threshold_dB = %d; // default: 0\r\n"\
"       int syncUpdate = %d;                    // default: 0\r\n"\
"   }\r\n"\
"   sdrplay_api_AdsbModeT adsbMode = %d;  //default: sdrplay_api_ADSB_DECIMATION; ADSB_DECIMATION = 0,ADSB_NO_DECIMATION_LOWPASS = 1,ADSB_NO_DECIMATION_BANDPASS_2MHZ = 2,ADSB_NO_DECIMATION_BANDPASS_3MHZ = 3\r\n"\
"}\r\n\r\n";

void printf_DevParams(sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
	typedef struct
	{
		double ppm;                         // default: 0.0
		sdrplay_api_FsFreqT fsFreq;
		typedef struct
		{
			double fsHz;                        // default: 2000000.0
			unsigned char syncUpdate;           // default: 0
			unsigned char reCal;                // default: 0
		} sdrplay_api_FsFreqT;
		sdrplay_api_SyncUpdateT syncUpdate;
		typedef struct
		{
			unsigned int sampleNum;             // default: 0
			unsigned int period;                // default: 0
		} sdrplay_api_SyncUpdateT;
		sdrplay_api_ResetFlagsT resetFlags;
		typedef struct
		{
			unsigned char resetGainUpdate;      // default: 0
			unsigned char resetRfUpdate;        // default: 0
			unsigned char resetFsUpdate;        // default: 0
		} sdrplay_api_ResetFlagsT;
		sdrplay_api_TransferModeT mode;     // default: sdrplay_api_ISOCH
		unsigned int samplesPerPkt;         // default: 0 (output param)
		sdrplay_api_Rsp1aParamsT rsp1aParams;
		sdrplay_api_Rsp2ParamsT rsp2Params;
		sdrplay_api_RspDuoParamsT rspDuoParams;
		sdrplay_api_RspDxParamsT rspDxParams;
	} sdrplay_api_DevParamsT;
	*/

	sdrplay_api_DevParamsT* devParams = deviceParams->devParams;
	printf(str_DevParams, 
		devParams->ppm, 
		devParams->fsFreq.fsHz,
		devParams->fsFreq.syncUpdate,
		devParams->fsFreq.reCal,
		devParams->syncUpdate.sampleNum,
		devParams->syncUpdate.period,
		devParams->resetFlags.resetGainUpdate,
		devParams->resetFlags.resetRfUpdate,
		devParams->resetFlags.resetFsUpdate,
		devParams->mode,
		devParams->samplesPerPkt
		);
}

sdrplay_api_ReasonForUpdateT ReasonForUpdate = sdrplay_api_Update_None;

/*
	// Reasons for master only mode
	sdrplay_api_Update_Dev_Fs                      = 0x00000001,
	sdrplay_api_Update_Dev_Ppm                     = 0x00000002,
	sdrplay_api_Update_Dev_SyncUpdate              = 0x00000004,
	sdrplay_api_Update_Dev_ResetFlags              = 0x00000008,

    // Reasons for master and slave mode
	// Note: sdrplay_api_Update_Tuner_Gr MUST be the first value defined in this section!
	sdrplay_api_Update_Tuner_Gr                    = 0x00008000,
	sdrplay_api_Update_Tuner_GrLimits              = 0x00010000,
	sdrplay_api_Update_Tuner_Frf                   = 0x00020000,
	sdrplay_api_Update_Tuner_BwType                = 0x00040000,
	sdrplay_api_Update_Tuner_IfType                = 0x00080000,
	sdrplay_api_Update_Tuner_DcOffset              = 0x00100000,
	sdrplay_api_Update_Tuner_LoMode                = 0x00200000,

	sdrplay_api_Update_Ctrl_DCoffsetIQimbalance    = 0x00400000,
	sdrplay_api_Update_Ctrl_Decimation             = 0x00800000,
	sdrplay_api_Update_Ctrl_Agc                    = 0x01000000,
	sdrplay_api_Update_Ctrl_AdsbMode               = 0x02000000,
	sdrplay_api_Update_Ctrl_OverloadMsgAck         = 0x04000000,
*/


void scanf_DevParams_ppm(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	sdrplay_api_DevParamsT* devParams = deviceParams->devParams;
	double ppm;
	printf("ÇëÊäÈëdouble devParams->ppm:\r\n");
	scanf("%lf", &ppm);
	printf("ppm:%lf\r\n", ppm);
	devParams->ppm = ppm;
	sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Dev_Ppm,
		sdrplay_api_Update_Ext1_None);
}

void scanf_DevParams_fsFreq(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
/*
typedef struct 
{
    double fsHz;                        // default: 2000000.0
    unsigned char syncUpdate;           // default: 0
    unsigned char reCal;                // default: 0
} sdrplay_api_FsFreqT;
*/
	sdrplay_api_DevParamsT* devParams = deviceParams->devParams;
	sdrplay_api_FsFreqT* fsFreq = &devParams->fsFreq;
	double fsHz;                        // default: 2000000.0
	unsigned char syncUpdate;           // default: 0
	unsigned char reCal;                // default: 0

	printf("ÇëÊäÈë devParams->FsFreq:\r\n"\
		"double fsHz;                        // default: 2000000.0 2,000,000.0 - 10,000,000.0Hz\r\n"\
		"unsigned char syncUpdate;           // default: 0\r\n"\
		"unsigned char reCal;                // default: 0\r\n");
	scanf("%lf%hhu%hhu", &fsHz, &syncUpdate, &reCal);
	printf("fsHz = %lf, syncUpdate = %hhu, reCal:%hhu\r\n", fsHz, syncUpdate, reCal);
	fsFreq->fsHz = fsHz;
	fsFreq->syncUpdate = syncUpdate;
	fsFreq->reCal = reCal;
	sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Dev_Fs,
		sdrplay_api_Update_Ext1_None);
}


void scanf_DevParams_syncUpdate(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef struct
{
	unsigned int sampleNum;             // default: 0
	unsigned int period;                // default: 0
} sdrplay_api_SyncUpdateT;
	*/
	sdrplay_api_DevParamsT* devParams = deviceParams->devParams;
	sdrplay_api_SyncUpdateT* syncUpdate = &devParams->syncUpdate;
	unsigned int sampleNum;             // default: 0
	unsigned int period;                // default: 0

	printf("ÇëÊäÈë devParams->syncUpdate:\r\n"\
		"unsigned int sampleNum;             // default: 0\r\n"\
		"unsigned int period;                // default: 0\r\n");
	scanf("%u%u", &sampleNum, &period);
	printf("sampleNum = %u, period = %u\r\n", sampleNum, period);
	syncUpdate->sampleNum = sampleNum;
	syncUpdate->period = period;
	sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Dev_SyncUpdate,
		sdrplay_api_Update_Ext1_None);
}


void scanf_DevParams_resetFlags(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef struct
{
	unsigned char resetGainUpdate;      // default: 0
	unsigned char resetRfUpdate;        // default: 0
	unsigned char resetFsUpdate;        // default: 0
} sdrplay_api_ResetFlagsT;
	*/
	sdrplay_api_DevParamsT* devParams = deviceParams->devParams;
	sdrplay_api_ResetFlagsT* resetFlags = &devParams->resetFlags;
	unsigned char resetGainUpdate;      // default: 0
	unsigned char resetRfUpdate;        // default: 0
	unsigned char resetFsUpdate;        // default: 0

	printf("ÇëÊäÈë devParams->resetFlags:\r\n"\
		"unsigned char resetGainUpdate;      // default: 0\r\n"\
		"unsigned char resetRfUpdate;        // default: 0\r\n"\
		"unsigned char resetFsUpdate;        // default: 0");
	scanf("%hhu%hhu%hhu", &resetGainUpdate, &resetRfUpdate, &resetFsUpdate);
	printf("resetGainUpdate = %hhu, resetRfUpdate = %hhu, resetFsUpdate = %hhu\r\n", resetGainUpdate, resetRfUpdate, resetFsUpdate);
	resetFlags->resetGainUpdate = resetGainUpdate;
	resetFlags->resetRfUpdate = resetRfUpdate;
	resetFlags->resetFsUpdate = resetFsUpdate;
	sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Dev_ResetFlags,
		sdrplay_api_Update_Ext1_None);
}



void printf_TunerParams(sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
	typedef struct
	{
		sdrplay_api_Bw_MHzT bwType;          // default: sdrplay_api_BW_0_200
		sdrplay_api_If_kHzT ifType;          // default: sdrplay_api_IF_Zero
		sdrplay_api_LoModeT loMode;          // default: sdrplay_api_LO_Auto
		sdrplay_api_GainT gain;
		typedef struct
		{
			int gRdB;                            // default: 50
			unsigned char LNAstate;              // default: 0
			unsigned char syncUpdate;            // default: 0
			sdrplay_api_MinGainReductionT minGr; // default: sdrplay_api_NORMAL_MIN_GR
			sdrplay_api_GainValuesT gainVals;    // output parameter
			typedef struct
			{
				float curr;
				float max;
				float min;
			} sdrplay_api_GainValuesT;
		} sdrplay_api_GainT;
		sdrplay_api_RfFreqT rfFreq;
		typedef struct
		{
			double rfHz;                         // default: 200000000.0
			unsigned char syncUpdate;            // default: 0
		} sdrplay_api_RfFreqT;
		sdrplay_api_DcOffsetTunerT dcOffsetTuner;
		typedef struct
		{
			unsigned char dcCal;                 // default: 3 (Periodic mode)
			unsigned char speedUp;               // default: 0 (No speedup)
			int trackTime;                       // default: 1    (=> time in uSec = (72 * 3 * trackTime) / 24e6       = 9uSec)
			int refreshRateTime;                 // default: 2048 (=> time in uSec = (72 * 3 * refreshRateTime) / 24e6 = 18432uSec)
		} sdrplay_api_DcOffsetTunerT;
	} sdrplay_api_TunerParamsT;
	*/

	sdrplay_api_TunerParamsT* tunerParams = &deviceParams->rxChannelA->tunerParams;
	printf(str_TunerParams,
		tunerParams->bwType,
		tunerParams->ifType,
		tunerParams->loMode,
		tunerParams->gain.gRdB,
		tunerParams->gain.LNAstate,
		tunerParams->gain.syncUpdate,
		tunerParams->gain.minGr,
		tunerParams->gain.gainVals.curr,
		tunerParams->gain.gainVals.max,
		tunerParams->gain.gainVals.min,
		tunerParams->rfFreq.rfHz,
		tunerParams->rfFreq.syncUpdate,
		tunerParams->dcOffsetTuner.dcCal,
		tunerParams->dcOffsetTuner.speedUp,
		tunerParams->dcOffsetTuner.trackTime,
		tunerParams->dcOffsetTuner.refreshRateTime
	);
}

void scanf_TunerParams_bwType(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef enum
{
	sdrplay_api_BW_Undefined = 0,
	sdrplay_api_BW_0_200     = 200,
	sdrplay_api_BW_0_300     = 300,
	sdrplay_api_BW_0_600     = 600,
	sdrplay_api_BW_1_536     = 1536,
	sdrplay_api_BW_5_000     = 5000,
	sdrplay_api_BW_6_000     = 6000,
	sdrplay_api_BW_7_000     = 7000,
	sdrplay_api_BW_8_000     = 8000
} sdrplay_api_Bw_MHzT;
	*/
	sdrplay_api_Bw_MHzT Bw[] = 
	{
		sdrplay_api_BW_Undefined,
		sdrplay_api_BW_0_200,
		sdrplay_api_BW_0_300,
		sdrplay_api_BW_0_600,
		sdrplay_api_BW_1_536,
		sdrplay_api_BW_5_000,
		sdrplay_api_BW_6_000,
		sdrplay_api_BW_7_000,
		sdrplay_api_BW_8_000
	};
	sdrplay_api_TunerParamsT* tunerParams = &deviceParams->rxChannelA->tunerParams;
	printf(
		"ÇëÊäÈë tunerParams->bwType:\r\n"\
		"0 sdrplay_api_BW_Undefined = 0,\r\n"\
		"1 sdrplay_api_BW_0_200 = 200,\r\n"\
		"2 sdrplay_api_BW_0_300 = 300,\r\n"\
		"3 sdrplay_api_BW_0_600 = 600,\r\n"\
		"4 sdrplay_api_BW_1_536 = 1536,\r\n"\
		"5 sdrplay_api_BW_5_000 = 5000,\r\n"\
		"6 sdrplay_api_BW_6_000 = 6000,\r\n"\
		"7 sdrplay_api_BW_7_000 = 7000,\r\n"\
		"8 sdrplay_api_BW_8_000 = 8000\r\n"
	);
	unsigned int Num;
	scanf("%u", &Num);
	printf("bwType = %d\r\n", Bw[Num]);
	tunerParams->bwType = Bw[Num];
	sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Tuner_BwType,
		sdrplay_api_Update_Ext1_None);
}

void scanf_TunerParams_ifType(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef enum
{
	sdrplay_api_IF_Undefined = -1,
	sdrplay_api_IF_Zero      = 0,
	sdrplay_api_IF_0_450     = 450,
	sdrplay_api_IF_1_620     = 1620,
	sdrplay_api_IF_2_048     = 2048
} sdrplay_api_If_kHzT;

	*/
	sdrplay_api_If_kHzT If[] =
	{
		sdrplay_api_IF_Undefined,
		sdrplay_api_IF_Zero,
		sdrplay_api_IF_0_450,
		sdrplay_api_IF_1_620,
		sdrplay_api_IF_2_048
	};
	sdrplay_api_TunerParamsT* tunerParams = &deviceParams->rxChannelA->tunerParams;
	printf(
		"ÇëÊäÈë tunerParams->ifType:\r\n"\
		"0 sdrplay_api_IF_Undefined = -1,\r\n"\
		"1 sdrplay_api_IF_Zero = 0,\r\n"\
		"2 sdrplay_api_IF_0_450 = 450,\r\n"\
		"3 sdrplay_api_IF_1_620 = 1620,\r\n"\
		"4 sdrplay_api_IF_2_048 = 2048\r\n"
	);
	unsigned int Num;
	scanf("%u", &Num);
	printf("ifType = %d\r\n", If[Num]);
	tunerParams->ifType = If[Num];
	sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Tuner_IfType,
		sdrplay_api_Update_Ext1_None);
}

void scanf_TunerParams_loMode(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef enum
{
	sdrplay_api_LO_Undefined = 0,
	sdrplay_api_LO_Auto      = 1,
	sdrplay_api_LO_120MHz    = 2,
	sdrplay_api_LO_144MHz    = 3,
	sdrplay_api_LO_168MHz    = 4
} sdrplay_api_LoModeT;
	*/
	sdrplay_api_LoModeT loMode[] =
	{
		sdrplay_api_LO_Undefined,
		sdrplay_api_LO_Auto,
		sdrplay_api_LO_120MHz,
		sdrplay_api_LO_144MHz,
		sdrplay_api_LO_168MHz
	};
	sdrplay_api_TunerParamsT* tunerParams = &deviceParams->rxChannelA->tunerParams;
	printf(
		"ÇëÊäÈë tunerParams->loMode:\r\n"\
		"0 sdrplay_api_LO_Undefined = 0,\r\n"\
		"1 sdrplay_api_LO_Auto = 1,\r\n"\
		"2 sdrplay_api_LO_120MHz = 2,\r\n"\
		"3 sdrplay_api_LO_144MHz = 3,\r\n"\
		"4 sdrplay_api_LO_168MHz = 4\r\n"
	);
	unsigned int Num;
	scanf("%u", &Num);
	printf("loMode = %d\r\n", loMode[Num]);
	tunerParams->loMode = loMode[Num];
	sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Tuner_LoMode,
		sdrplay_api_Update_Ext1_None);
}

void scanf_TunerParams_gain(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef enum
{
	sdrplay_api_EXTENDED_MIN_GR = 0,
	sdrplay_api_NORMAL_MIN_GR   = 20
} sdrplay_api_MinGainReductionT;

typedef struct
{
	int gRdB;                            // default: 50
	unsigned char LNAstate;              // default: 0
	unsigned char syncUpdate;            // default: 0
	sdrplay_api_MinGainReductionT minGr; // default: sdrplay_api_NORMAL_MIN_GR
	sdrplay_api_GainValuesT gainVals;    // output parameter
} sdrplay_api_GainT;

	*/
	sdrplay_api_MinGainReductionT minGr[] =
	{
		sdrplay_api_EXTENDED_MIN_GR,
		sdrplay_api_NORMAL_MIN_GR
	};
	sdrplay_api_TunerParamsT* tunerParams = &deviceParams->rxChannelA->tunerParams;
	sdrplay_api_GainT *gain = &tunerParams->gain;
	printf(
		"ÇëÊäÈë tunerParams->gain:\r\n"\
		"int gRdB;                            // default: 50\r\n"\
		"unsigned char LNAstate;              // default: 0		input range:0 - 3\r\n"\
		"	LNA GR(dB) by Frequency Range and LNAstate for RSP1:\r\n"\
		"	Frequency(MHz)	LNAstate	0	1	2	3\r\n"\
		"	0 - 420						0	24	19(1)	43(2)\r\n"\
		"	420 - 1000					0	7	19(1)	26(2)\r\n"\
		"	1000 - 2000					0	5	19(1)	24(2)\r\n"\
		"	note:1 Mixer GR only\r\n"\
		"	note:2 Includes LNA GR plus mixer GR\r\n"\
		"unsigned char syncUpdate;            // default: 0\r\n"\
		"sdrplay_api_MinGainReductionT minGr; // default: sdrplay_api_NORMAL_MIN_GR\r\n"\
		"	{\r\n"\
		"		0 sdrplay_api_EXTENDED_MIN_GR = 0,	(0 - 59)\r\n"\
		"		1 sdrplay_api_NORMAL_MIN_GR = 20	(20 - 59)\r\n"\
		"	}\r\n"\
		"sdrplay_api_GainValuesT gainVals;    // output parameter\r\n"
	);
	int gRdB;                            // default: 50
	unsigned char LNAstate;              // default: 0
	unsigned char syncUpdate;            // default: 0
	unsigned int minGrNum;
	scanf("%d%hhu%hhu%u", &gRdB, &LNAstate, &syncUpdate, &minGrNum);
	printf("gain: gRdB = %d, LNAstate = %hhu, syncUpdate = %hhu, minGrNum = %u\r\n", gRdB, LNAstate, syncUpdate, minGr[minGrNum]);
	gain->gRdB = gRdB;
	gain->LNAstate = LNAstate;
	gain->syncUpdate = syncUpdate;
	gain->minGr = minGr[minGrNum];
	UpdateParams(chosenDevice, sdrplay_api_Update_Tuner_Gr);
	UpdateParams(chosenDevice, sdrplay_api_Update_Tuner_GrLimits);

	//sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
	//sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Tuner_GrLimits, sdrplay_api_Update_Ext1_None);
}

void scanf_TunerParams_rfFreq(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef struct
{
	double rfHz;                         // default: 200000000.0
	unsigned char syncUpdate;            // default: 0
} sdrplay_api_RfFreqT;
	*/
	sdrplay_api_TunerParamsT* tunerParams = &deviceParams->rxChannelA->tunerParams;
	sdrplay_api_RfFreqT* rfFreq = &tunerParams->rfFreq;
	printf(
		"ÇëÊäÈë tunerParams->rfFreq:\r\n"\
		"double rfHz;                         // default: 200000000.0\r\n"\
		"unsigned char syncUpdate;            // default: 0\r\n"\
	);
	double rfHz;                         // default: 200000000.0
	unsigned char syncUpdate;            // default: 0
	scanf("%lf%hhu", &rfHz, &syncUpdate);
	printf("rfFreq: rfHz = %lf, syncUpdate = %hhu\r\n", rfHz, syncUpdate);
	rfFreq->rfHz = rfHz;
	rfFreq->syncUpdate = syncUpdate;
	UpdateParams(chosenDevice, sdrplay_api_Update_Tuner_Frf);
}

void scanf_TunerParams_dcOffsetTuner(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef struct
{
	unsigned char dcCal;                 // default: 3 (Periodic mode)
	unsigned char speedUp;               // default: 0 (No speedup)
	int trackTime;                       // default: 1    (=> time in uSec = (72 * 3 * trackTime) / 24e6       = 9uSec)
	int refreshRateTime;                 // default: 2048 (=> time in uSec = (72 * 3 * refreshRateTime) / 24e6 = 18432uSec)
} sdrplay_api_DcOffsetTunerT;

	*/
	sdrplay_api_TunerParamsT* tunerParams = &deviceParams->rxChannelA->tunerParams;
	sdrplay_api_DcOffsetTunerT* dcOffsetTuner = &tunerParams->dcOffsetTuner;
	printf(
		"ÇëÊäÈë tunerParams->dcOffsetTuner:\r\n"\
		"unsigned char dcCal;                 // default: 3 (Periodic mode)\r\n"\
		"unsigned char speedUp;               // default: 0 (No speedup)\r\n"\
		"int trackTime;                       // default: 1    (=> time in uSec = (72 * 3 * trackTime) / 24e6       = 9uSec)\r\n"\
		"int refreshRateTime;                 // default: 2048 (=> time in uSec = (72 * 3 * refreshRateTime) / 24e6 = 18432uSec)\r\n"
	);
	unsigned char dcCal;                 // default: 3 (Periodic mode)
	unsigned char speedUp;               // default: 0 (No speedup)
	int trackTime;                       // default: 1    (=> time in uSec = (72 * 3 * trackTime) / 24e6       = 9uSec)
	int refreshRateTime;                 // default: 2048 (=> time in uSec = (72 * 3 * refreshRateTime) / 24e6 = 18432uSec)
	scanf("%hhu%hhu%d%d", &dcCal, &speedUp, &trackTime, &refreshRateTime);
	printf("dcOffsetTuner: dcCal = %hhu, speedUp = %hhu, trackTime = %d, refreshRateTime = %d\r\n", dcCal, speedUp, trackTime, refreshRateTime);
	dcOffsetTuner->dcCal = dcCal;
	dcOffsetTuner->speedUp = speedUp;
	dcOffsetTuner->trackTime = trackTime;
	dcOffsetTuner->refreshRateTime = refreshRateTime;
	sdrplay_api_Update(chosenDevice->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Tuner_DcOffset,
		sdrplay_api_Update_Ext1_None);
}

void printf_ControlParams(sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
	typedef struct
	{
		sdrplay_api_DcOffsetT dcOffset;
		typedef struct
		{
			unsigned char DCenable;          // default: 1
			unsigned char IQenable;          // default: 1
		} sdrplay_api_DcOffsetT;
		sdrplay_api_DecimationT decimation;
		typedef struct
		{
			unsigned char enable;            // default: 0
			unsigned char decimationFactor;  // default: 1
			unsigned char wideBandSignal;    // default: 0
		} sdrplay_api_DecimationT;
		sdrplay_api_AgcT agc;
		typedef struct
		{
			sdrplay_api_AgcControlT enable;    // default: sdrplay_api_AGC_50HZ
			int setPoint_dBfs;                 // default: -60
			unsigned short attack_ms;          // default: 0
			unsigned short decay_ms;           // default: 0
			unsigned short decay_delay_ms;     // default: 0
			unsigned short decay_threshold_dB; // default: 0
			int syncUpdate;                    // default: 0
		} sdrplay_api_AgcT;
		sdrplay_api_AdsbModeT adsbMode;  //default: sdrplay_api_ADSB_DECIMATION
	} sdrplay_api_ControlParamsT;
	*/

	sdrplay_api_ControlParamsT* ctrlParams = &deviceParams->rxChannelA->ctrlParams;
	printf(str_ControlParams,
		ctrlParams->dcOffset.DCenable,
		ctrlParams->dcOffset.IQenable,
		ctrlParams->decimation.enable,
		ctrlParams->decimation.decimationFactor,
		ctrlParams->decimation.wideBandSignal,
		ctrlParams->agc.enable,
		ctrlParams->agc.setPoint_dBfs,
		ctrlParams->agc.attack_ms,
		ctrlParams->agc.decay_ms,
		ctrlParams->agc.decay_delay_ms,
		ctrlParams->agc.decay_threshold_dB,
		ctrlParams->agc.syncUpdate,
		ctrlParams->adsbMode
	);
}

void scanf_ControlParams_dcOffset(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef struct
{
	unsigned char DCenable;          // default: 1
	unsigned char IQenable;          // default: 1
} sdrplay_api_DcOffsetT;
	*/
	sdrplay_api_ControlParamsT* ctrlParams = &deviceParams->rxChannelA->ctrlParams;
	sdrplay_api_DcOffsetT* dcOffset = &ctrlParams->dcOffset;
	printf(
		"ÇëÊäÈë ctrlParams->dcOffset:\r\n"\
		"unsigned char DCenable;          // default: 1\r\n"\
		"unsigned char IQenable;          // default: 1\r\n"
	);
	unsigned char DCenable;          // default: 1
	unsigned char IQenable;          // default: 1
	scanf("%hhu%hhu", &DCenable, &IQenable);
	printf("rfFreq: DCenable = %lf, syncUIQenablepdate = %hhu\r\n", DCenable, IQenable);
	dcOffset->DCenable = DCenable;
	dcOffset->IQenable = IQenable;
	UpdateParams(chosenDevice, sdrplay_api_Update_Ctrl_DCoffsetIQimbalance);
}

void scanf_ControlParams_decimation(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef struct
{
	unsigned char enable;            // default: 0
	unsigned char decimationFactor;  // default: 1
	unsigned char wideBandSignal;    // default: 0
} sdrplay_api_DecimationT;
	*/
	sdrplay_api_ControlParamsT* ctrlParams = &deviceParams->rxChannelA->ctrlParams;
	sdrplay_api_DecimationT* decimation = &ctrlParams->decimation;
	printf(
		"ÇëÊäÈë ctrlParams->decimation:\r\n"\
		"unsigned char enable;            // default: 0\r\n"\
		"unsigned char decimationFactor;  // default: 1\r\n"\
		"unsigned char wideBandSignal;    // default: 0\r\n"
	);
	unsigned char enable;            // default: 0
	unsigned char decimationFactor;  // default: 1
	unsigned char wideBandSignal;    // default: 0
	scanf("%hhu%hhu%hhu", &enable, &decimationFactor, &wideBandSignal);
	printf("decimation: enable = %lf, decimationFactor = %hhu, wideBandSignal = %hhu\r\n", enable, decimationFactor, wideBandSignal);
	decimation->enable = enable;
	decimation->decimationFactor = decimationFactor;
	decimation->wideBandSignal = wideBandSignal;
	UpdateParams(chosenDevice, sdrplay_api_Update_Ctrl_Decimation);
}

void scanf_ControlParams_agc(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef enum
{
    sdrplay_api_AGC_DISABLE  = 0,
    sdrplay_api_AGC_100HZ    = 1,
    sdrplay_api_AGC_50HZ     = 2,
    sdrplay_api_AGC_5HZ      = 3,
    sdrplay_api_AGC_CTRL_EN  = 4
} sdrplay_api_AgcControlT;
typedef struct
{
	sdrplay_api_AgcControlT enable;    // default: sdrplay_api_AGC_50HZ
	int setPoint_dBfs;                 // default: -60
	unsigned short attack_ms;          // default: 0
	unsigned short decay_ms;           // default: 0
	unsigned short decay_delay_ms;     // default: 0
	unsigned short decay_threshold_dB; // default: 0
	int syncUpdate;                    // default: 0
} sdrplay_api_AgcT;
	*/
	sdrplay_api_ControlParamsT* ctrlParams = &deviceParams->rxChannelA->ctrlParams;
	sdrplay_api_AgcT* agc = &ctrlParams->agc;
	printf(
		"ÇëÊäÈë ctrlParams->agc:\r\n"\
		"sdrplay_api_AgcControlT enable;    // default: sdrplay_api_AGC_50HZ\r\n"\
		"	{\r\n"\
		"		0 sdrplay_api_AGC_DISABLE = 0,\r\n"\
		"		1 sdrplay_api_AGC_100HZ = 1,\r\n"\
		"		2 sdrplay_api_AGC_50HZ = 2,\r\n"\
		"		3 sdrplay_api_AGC_5HZ = 3,\r\n"\
		"		4 sdrplay_api_AGC_CTRL_EN = 4\r\n"\
		"	}\r\n"\
		"int setPoint_dBfs;                 // default: -60\r\n"\
		"unsigned short attack_ms;          // default: 0\r\n"\
		"unsigned short decay_ms;           // default: 0\r\n"\
		"unsigned short decay_delay_ms;     // default: 0\r\n"\
		"unsigned short decay_threshold_dB; // default: 0\r\n"\
		"int syncUpdate;                    // default: 0\r\n"
	);
	sdrplay_api_AgcControlT enable[] =    // default: sdrplay_api_AGC_50HZ
	{
		sdrplay_api_AGC_DISABLE,
		sdrplay_api_AGC_100HZ,
		sdrplay_api_AGC_50HZ,
		sdrplay_api_AGC_5HZ,
		sdrplay_api_AGC_CTRL_EN
	}; 
	unsigned int enableNum;
	int setPoint_dBfs;                 // default: -60
	unsigned short attack_ms;          // default: 0
	unsigned short decay_ms;           // default: 0
	unsigned short decay_delay_ms;     // default: 0
	unsigned short decay_threshold_dB; // default: 0
	int syncUpdate;                    // default: 0
	scanf("%u%d%hu%hu%hu%hu%d", &enableNum, &setPoint_dBfs, &attack_ms, &decay_ms, &decay_delay_ms, &decay_threshold_dB, &syncUpdate);
	printf("agc: enable = %d, setPoint_dBfs = %d, attack_ms = %hu, decay_ms = %hu, decay_delay_ms = %hu, decay_threshold_dB = %hu, syncUpdate = %d", 
		enable[enableNum], setPoint_dBfs, attack_ms, decay_ms, decay_delay_ms, decay_threshold_dB, syncUpdate);
	agc->enable = enable[enableNum];
	agc->setPoint_dBfs = setPoint_dBfs;
	agc->attack_ms = attack_ms;
	agc->decay_ms = decay_ms;
	agc->decay_delay_ms = decay_delay_ms;
	agc->decay_threshold_dB = decay_threshold_dB;
	agc->syncUpdate = syncUpdate;
	UpdateParams(chosenDevice, sdrplay_api_Update_Ctrl_Agc);
}

void scanf_ControlParams_adsbMode(sdrplay_api_DeviceT* chosenDevice, sdrplay_api_DeviceParamsT* deviceParams)
{
	/*
typedef enum
{
	sdrplay_api_ADSB_DECIMATION                  = 0,
	sdrplay_api_ADSB_NO_DECIMATION_LOWPASS       = 1,
	sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_2MHZ = 2,
	sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_3MHZ = 3
} sdrplay_api_AdsbModeT;
	*/
	sdrplay_api_ControlParamsT* ctrlParams = &deviceParams->rxChannelA->ctrlParams;
	printf(
		"ÇëÊäÈë ctrlParams->adsbMode:\r\n"\
		"sdrplay_api_ADSB_DECIMATION = 0, \r\n"\
		"sdrplay_api_ADSB_NO_DECIMATION_LOWPASS = 1,\r\n"\
		"sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_2MHZ = 2, \r\n"\
		"sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_3MHZ = 3\r\n"
	);
	sdrplay_api_AdsbModeT adsbMode[] =
	{
		sdrplay_api_ADSB_DECIMATION,
		sdrplay_api_ADSB_NO_DECIMATION_LOWPASS,
		sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_2MHZ,
		sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_3MHZ
	};
	unsigned int adsbModeNum;
	scanf("%u", &adsbModeNum);
	printf("adsbMode = %d\r\n", adsbMode[adsbModeNum]);
	ctrlParams->adsbMode = adsbMode[adsbModeNum];
	UpdateParams(chosenDevice, sdrplay_api_Update_Ctrl_AdsbMode);
}

#define SETFUNCS_NUM	10
SetFunc setfuncs[] = 
{ 
	scanf_DevParams_ppm, 
	scanf_DevParams_fsFreq,
	scanf_DevParams_syncUpdate,
	scanf_DevParams_resetFlags,

	scanf_TunerParams_bwType,
	scanf_TunerParams_ifType,
	scanf_TunerParams_loMode,
	scanf_TunerParams_gain,
	scanf_TunerParams_rfFreq,
	scanf_TunerParams_dcOffsetTuner,

	scanf_ControlParams_dcOffset,
	scanf_ControlParams_decimation,
	scanf_ControlParams_agc,
	scanf_ControlParams_adsbMode
};
