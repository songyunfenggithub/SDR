#include "stdafx.h"
#include "windows.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>

#include <map>

#include "Debug.h"
#include "CFilter.h"
#include "CDataFromSDR.h"
#include "CWinSDR.h"

#include "CSDR.h"

#include "SDRPlay_API.3.09/API/inc/sdrplay_api.h"

#pragma comment(lib,"SDRPlay_API.3.09/API/x64/sdrplay_api.lib")

using namespace DEVICES;

CSDR clsSDR;


SDR_ENUM_MAP sdrplay_api_TransferModeT_map =
{
	{ "sdrplay_api_ISOCH",	sdrplay_api_ISOCH },
	{ "sdrplay_api_BULK",	sdrplay_api_BULK }
};

SDR_ENUM_MAP sdrplay_api_Bw_MHzT_map =
{
	{ "sdrplay_api_BW_Undefined",	sdrplay_api_BW_Undefined },
	{ "sdrplay_api_BW_0_200",		sdrplay_api_BW_0_200 },
	{ "sdrplay_api_BW_0_300",		sdrplay_api_BW_0_300 },
	{ "sdrplay_api_BW_0_600",		sdrplay_api_BW_0_600 },
	{ "sdrplay_api_BW_1_536",		sdrplay_api_BW_1_536 },
	{ "sdrplay_api_BW_5_000",		sdrplay_api_BW_5_000 },
	{ "sdrplay_api_BW_6_000",		sdrplay_api_BW_6_000 },
	{ "sdrplay_api_BW_7_000",		sdrplay_api_BW_7_000 },
	{ "sdrplay_api_BW_8_000",		sdrplay_api_BW_8_000 }
};

SDR_ENUM_MAP sdrplay_api_If_kHzT_map =
{
	{ "sdrplay_api_IF_Undefined",	sdrplay_api_IF_Undefined },
	{ "sdrplay_api_IF_Zero",		sdrplay_api_IF_Zero },
	{ "sdrplay_api_IF_0_450",		sdrplay_api_IF_0_450 },
	{ "sdrplay_api_IF_1_620", 		sdrplay_api_IF_1_620 },
	{ "sdrplay_api_IF_2_048", 		sdrplay_api_IF_2_048 }
};

SDR_ENUM_MAP sdrplay_api_LoModeT_map =
{
	{ "sdrplay_api_LO_Undefined",	sdrplay_api_LO_Undefined },
	{ "sdrplay_api_LO_Auto",		sdrplay_api_LO_Auto },
	{ "sdrplay_api_LO_120MHz",		sdrplay_api_LO_120MHz },
	{ "sdrplay_api_LO_144MHz",		sdrplay_api_LO_144MHz },
	{ "sdrplay_api_LO_168MHz",		sdrplay_api_LO_168MHz }
};

SDR_ENUM_MAP sdrplay_api_MinGainReductionT_map =
{
	{ "sdrplay_api_EXTENDED_MIN_GR",sdrplay_api_EXTENDED_MIN_GR },
	{ "sdrplay_api_NORMAL_MIN_GR",	sdrplay_api_NORMAL_MIN_GR }
};

SDR_ENUM_MAP sdrplay_api_TunerSelectT_map =
{
	{ "sdrplay_api_Tuner_Neither",	sdrplay_api_Tuner_Neither },
	{ "sdrplay_api_Tuner_A",		sdrplay_api_Tuner_A },
	{ "sdrplay_api_Tuner_B",		sdrplay_api_Tuner_B },
	{ "sdrplay_api_Tuner_Both",		sdrplay_api_Tuner_Both }
};


// Control parameter enums
SDR_ENUM_MAP sdrplay_api_AgcControlT_map =
{
	{ "sdrplay_api_AGC_DISABLE",	sdrplay_api_AGC_DISABLE },
	{ "sdrplay_api_AGC_100HZ",		sdrplay_api_AGC_100HZ },
	{ "sdrplay_api_AGC_50HZ",		sdrplay_api_AGC_50HZ },
	{ "sdrplay_api_AGC_5HZ",		sdrplay_api_AGC_5HZ },
	{ "sdrplay_api_AGC_CTRL_EN",	sdrplay_api_AGC_CTRL_EN }
};

SDR_ENUM_MAP sdrplay_api_AdsbModeT_map =
{
	{ "sdrplay_api_ADSB_DECIMATION",					sdrplay_api_ADSB_DECIMATION },
	{ "sdrplay_api_ADSB_NO_DECIMATION_LOWPASS",			sdrplay_api_ADSB_NO_DECIMATION_LOWPASS },
	{ "sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_2MHZ",	sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_2MHZ },
	{ "sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_3MHZ",	sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_3MHZ }
};

SDR_ENUM_MAP sdrplay_api_decimationFactor_map =
{
	{ "decimationFactor1", decimationFactor1 },
	{ "decimationFactor2", decimationFactor2 },
	{ "decimationFactor4", decimationFactor4 },
	{ "decimationFactor8", decimationFactor8 },
	{ "decimationFactor16", decimationFactor16 },
	{ "decimationFactor32", decimationFactor32 },
	{ "decimationFactor64", decimationFactor64 }
};

SDR_ENUM_MAP sdrplay_ap_sampleRate_map =
{
	{ "sampleRate_2_M", sampleRate_2_M },
	{ "sampleRate_4_M", sampleRate_4_M },
	{ "sampleRate_6_M", sampleRate_6_M },
	{ "sampleRate_8_M", sampleRate_8_M },
	{ "sampleRate_10_M", sampleRate_10_M },
};

const char ppmComment[] = 
"To specify a correction factor used to account for offsets from the nominal in the crystal oscillator.\r\n"\
"Parts per million offset (e.g. +/- 1 ppm specifies a +/- 24Hz error for a 24MHz crystal).";

const char fsFreqComment[] =
"fsHz double\r\n"\
"Sample frequency or sample frequency offset in Hz.Once absolute value has "\
"been calculated(if required), it must be in the range 2MHz to 10MHz.\r\n"\
"Valid Setpoint Values vs Sample Rate\r\n"\
"- 72 <= setpoint_dBfs <= -20dB(or 0dB depending on setting of sdrplay_api_GainT.minGr) for sample rates < 8.064 MSPS\r\n"\
"- 60 <= setpoint_dBfs <= -20dB(or 0dB depending on setting of sdrplay_api_GainT.minGr) for sample rates in the range 8.064 – 9.216 MSPS\r\n"\
"- 48 <= setpoint_dBfs <= -20dB(or 0dB depending on setting of sdrplay_api_GainT.minGr) for sample rates > 9.216 MSPS)\r\n"
"\r\n"\
"syncUpdate unsigned char\r\n"\
"Indicates if the sample frequency update is to be applied immediately or delayed "\
"until the next synchronous update point as configured in calls to "\
"mir_sdr_SetSyncUpdateSampleNum() and mir_sdr_SetSyncUpdatePeriod() :\r\n"\
"0 → Immediate\r\n"\
"1 → Synchronous\r\n\r\n"\
"reCal unsigned char\r\n"\
"Recalibration of the PLL. Note: this is normally done only when the nominal "\
"sample frequency is set in mir_sdr_Init() and should be set to 0 elsewhere:\r\n"\
"0 → no recalibration is made\r\n"\
"1 → force a recalibration of the PLL";

const char syncUpdateComment[] =
"sampleNum unsigned int :\r\n"\
"Sample number of next synchronous update point.\r\n"\
"Configures the sample number of the next synchronous update point. This is typically determined from "\
"the use of the firstSampleNum parameter returned in the mir_sdr_ReadPacket() function call.If the "\
"latency incurred over the USB causes this sample number to be set too late, the hardware will adjust "\
"automatically to correct for this.\r\n\r\n"\
"period unsigned int :\r\n"\
"Defines the period between synchronous update points can be set between 1 and 1000000 samples.\r\n"\
"The value set in this call is automatically added to the sample number of the last synchronous update "\
"point to determine the next one.Note – this function should be called before mir_sdr_SetSyncUpdateSampleNum().";

const char resetFlagsComment[] =
"resetGainUpdate unsigned char :\r\n"\
"Reset Gain Reduction update logic : \r\n"\
"0 → do not reset\r\n"\
"1 → reset"\
"\r\n\r\n"\
"resetRfUpdate unsigned char :\r\n"\
"Reset Tuner Frequency update logic :\r\n"\
"0 → do not reset\r\n"\
"1 → reset"\
"\r\n\r\n"\
"resetFsUpdate unsigned char :\r\n"\
"Reset Sample Frequency update logic : \r\n"\
"0 → do not reset\r\n"\
"1 → reset"\
"\r\n\r\n"\
"If it is detected that an update to one or more of Gain Reduction, Tuner Frequency or Sample "\
"Frequency has not completed within some application specific timeout period, the logic prohibiting "\
"further updates can be reset using this function.More than one update type can be reset in each call, "\
"and once reset, new updates can be scheduled.";

const char TransferModeComment[] =
"Mode selected:\r\n"\
"mir_sdr_ISOCH → Isochronous mode(default on non - ARM CPU platforms)\r\n"\
"mir_sdr_BULK → Bulk mode(default on ARM CPU platforms)\r\n\r\n"\
"Used to change USB streaming data transfer mode. Typically, this should not need to be changed and "\
"data loss may occur if the mode is changed.\r\n"\
"Note: for ARM processor based platforms this function only allows mir_sdr_BULK to be usedand any "\
"other requested value will return an mir_sdr_OutOfRange error.";

const char samplesPerPktComment[] =
"unknow.";

const char bwTypeComment[] =
"Specifies the bandwidth to be used, see list in enumerated type for supported modes.";

const char ifTypeComment[] =
"Specifies the IF to be used, see list in enumerated type for supported modes.";

const char loModeComment[] =
"Allows a particular up - converter(1st) LO frequency to be specifed or selects automatic mode which allows "\
"the API to determine the most appropriate 1st LO frequency across all tuner frequency ranges.This function "\
"must be called before the API is initialized – otherwise use mir_sdr_ReInit()";

const char gain_gRdBComment[] =
"gRdB int :\r\n"\
"Gain reduction or gain reduction offset in dB(see gain reduction tables referenced in section 5.1 for rangesand mappings)\r\n"\
"If the function completes successfully, just before it returns it calls the gain callback function with the updated parameters.\r\n";

const char gain_LNAstateComment[] =
"LNAstate unsigned char :\r\n"\
"for RSP1:\r\n"\
"Frequency(MHz) 0  1  2     3\r\n"\
"0 - 420        0  24 19(1) 43(2)\r\n"\
"420 - 1000     0  7  19(1) 26(2)\r\n"\
"1000 - 2000    0  5  19(1) 24\r\n"\
"note 1 Mixer GR only\r\n"\
"note 2 Includes LNA GR plus mixer GR";

const char gain_syncUpdateComment[] =
"syncUpdate unsigned char gain :\r\n"\
"syncUpdate Indicates if the gain reduction is to be applied immediately or delayed until the "\
"next synchronous update point as configured in calls to "\
"mir_sdr_SetSyncUpdateSampleNum() and mir_sdr_SetSyncUpdatePeriod()\r\n"\
"0 → Immediate\r\n"\
"1 → Synchronous";

const char gain_minGrComment[] =
"minGr sdrplay_api_MinGainReductionT :\r\n"\
"Minimum gain reduction in dB that can be programmed.\r\n"\
"Modifies the default gain reduction parameters required in the tuner.These are only applicable when using mir_sdr_SetGr()\r\n"\
"\r\n"\
"Indicates the minimum IF gain reduction to extend the range :\r\n"\
"mir_sdr_NORMAL_MIN_GR → minimum IF GR of 20, range of 20 to 59\r\n"\
"mir_sdr_EXTENDED_MIN_GR → minimum IF GR of 0, range of 0 to 59\r\n"\
"\r\n"\
"This function provides a mechanism to extend the normally useful IF gain reduction range to include "\
"values less than 20dB when using mir_sdr_RSP_SetGr().It is not recommended to use the extended "\
"range as it is likely to overload to produce signal levels that will overload the ADC input.\r\n"\
"Note : that the extended range cannot be used by the internal AGC algorithmand will be disabled when "\
"enabling the AGC.If the AGC is subsequently disabled, the extended range will not be re - selected automatically.";

const char gain_gainValsComment[] =
"gainVals sdrplay_api_GainValuesT :\r\n"
"curr float, max float, min float, return value, can not change.";

const char rfFreq_rfHzComment[] =
"rfHz double :\r\n"\
"Tuner frequency or tuner frequency offset in Hz.Once absolute value has been "\
"calculated(if required), it must be within the range of the current frequency band "\
"- see frequency allocation tables, section 0.\r\n"\
"Band         MinFreq(MHz) MaxFreq(MHz)\r\n"\
"AM           0            60\r\n"\
"VHF          60           120\r\n"\
"Band 3       120          250\r\n"\
"Band X       250          420\r\n"\
"Band 4 / 5   420          1000\r\n"\
"L Band       1000         2000\r\n"\
"--------------------------------\r\n"\
"AM(HiZ Port) 0            60\r\n"\
"RSP2/RSPduo\r\n"\
"only\r\n"\
"\r\n"\
"When using the mir_sdr_StreamInit() or mir_sdr_Init() command any tuner frequency range "\
"supported by the hardware can be programmed and this will configure the front end accordingly.Once "\
"the desired frequency has been programmed the mir_sdr_setRf() command can be used to alter the "\
"tuner frequency.It should be noted though, that the mir_sdr_setRf() command can only change the "\
"frequency within a set of predefined bands.If a frequency is desired that falls outside the current band "\
"then a mir_sdr_Uninit() command must be issued followed by a mir_sdr_Init() command at the new "\
"frequency to force reconfiguration of the front end.The table below shows the frequency bands over "\
"which the mir_sdr_setRf() commands will permit operation.\r\n"\
"Alternatively, with this version of the API, the mir_sdr_Reinit() command can be used to change the "\
"frequency without requiring a mir_sdr_Uninit() / mir_sdr_Init() when crossing band boundaries.";

const char rfFreq_syncUpdateComment[] =
"syncUpdate unsigned char :\r\n"\
"Indicates if the tuner frequency update is to be applied immediately or delayed "\
"until the next synchronous update point as configured in calls to "\
"mir_sdr_SetSyncUpdateSampleNum() and mir_sdr_SetSyncUpdatePeriod() :\r\n"\
"0 → Immediate\r\n"\
"1 → Synchronous\r\n"\
"\r\n"\
"Adjusts the nominal tuner frequency maintained in the internal state of the API. Depending on the "\
"state of the abs parameter, the drfHz parameter is either applied as an offset from the internally stored "\
"state of the API or is used in an absolute manner to modify the internally stored state.This command "\
"will only permit frequency changes that fall within the restrictions of the frequency allocation tables "\
"shown in section 5";


const char dcOffsetTunerComment[] =
"dcCal unsigned char : default: 3 (Periodic mode)\r\n"\
"DC offset correction mode :\r\n"\
"0 → static\r\n"\
"1 → Periodic 1 (Correction applied periodically every 6mS)\r\n"\
"2 → Periodic 2 (Correction applied periodically every 12mS)\r\n"\
"3 → Periodic 3 (Correction applied periodically every 24mS)\r\n"\
"4 → one shot mode(correction applied each time gain update performed)\r\n"\
"5 → continuous\r\n"\
"\r\n"\
"speedUp unsigned char : default: 0 (No speedup)\r\n"\
"speedUp Speed up mode :\r\n"\
"0 → disabled\r\n"\
"1 → enabled\r\n"\
"\r\n"\
"trackTime int :\r\n"\
"default:1 (= > time in uSec = (dcCal * 3 * trackTime) = 9uSec)\r\n"\
"refreshRateTime int :\r\n"\
"default:2048 (= > time in uSec = (dcCal * 3 * refreshRateTime) = 18432uSec)\r\n";

const char dcOffsetComment[] =
"DCenable unsigned char : \r\n"\
"DC offset correction control :\r\n"\
"0 → DC correction disabled\r\n"\
"1 → DC correction enabled(default)\r\n"\
"\r\n"\
"IQenable unsigned char : \r\n"\
"IQ correction control :\r\n"\
"0 → IQ correction disabled\r\n"\
"1 → IQ correction enabled(default)\r\n"\
"\r\n"\
"Individual enables for DC offset correctionand IQ imbalance correction.";

const char decimationComment[] =
"enable unsigned char : \r\n"\
"Decimation control\r\n"\
"0 → Decimation disabled(default)\r\n"\
"1 → Decimation enabled\r\n"\
"\r\n"\
"decimationFactor unsigned char : "\
"decimationFactor Decimation factor(2, 4, 8, 16 or 32 only).\r\n"\
"\r\n"\
"wideBandSignal unsigned char : "\
"widebandSignal Filter control :\r\n"\
"0 → Use averaging(default)\r\n"\
"1 → Use half - band filter\r\n"\
"\r\n"\
"Used to control whether decimation is enabled or not. Valid decimation factors are 2, 4, 8, 16, 32 or 64 "\
"only.If other values are specified then decimation will not be enabled.If wide band mode is selected, "\
"the decimation algorithm uses a sequence of half - band filters to achieve the required decimation, "\
"otherwise a box - filter is used which is much more efficient but may cause roll - off in the passband of "\
"the received signal depending on bandwidth.Otherwise, a simple block averaging is used to reduce the "\
"CPU load, but with increased in - band roll - off.\r\n"\
"Note: Requires internal stream thread to have been created via mir_sdr_StreamInit() for decimation to "\
"be enabled.Also for IQ output in Low IF mode, enable must = 0 as the decimation is automatic within "\
"the API.";

const char agcComment[] =
"enable sdrplay_api_AgcControlT :\r\n"\
"\r\n"\
"setPoint_dBfs int :\r\n"\
"Specifies the required set point in dBfs.\r\n"\
"Valid Setpoint Values vs Sample Rate\r\n"\
"- 72 <= setpoint_dBfs <= -20dB(or 0dB depending on setting of sdrplay_api_GainT.minGr) for sample rates < 8.064 MSPS\r\n"\
"- 60 <= setpoint_dBfs <= -20dB(or 0dB depending on setting of sdrplay_api_GainT.minGr) for sample rates in the range 8.064 – 9.216 MSPS\r\n"\
"- 48 <= setpoint_dBfs <= -20dB(or 0dB depending on setting of sdrplay_api_GainT.minGr) for sample rates > 9.216 MSPS)\r\n"
"\r\n"\
"attack_ms unsigned short :\r\n"\
"\r\n"\
"decay_ms unsigned short :\r\n"\
"\r\n"\
"decay_delay_ms unsigned short :\r\n"\
"\r\n"\
"decay_threshold_dB unsigned short :\r\n"\
"\r\n"\
"syncUpdate int agc :\r\n"\
"Update control : \r\n"\
"0 → Immediate\r\n"\
"1 → Synchronous";

const char adsbModeComment[] =
"\r\n"\
;

SDRParams SDR_params[] =
{
	{ 0, "SDR RSP1 sdrplay_api_DeviceParamsT", 0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 1, "设备参数 sdrplay_api_DevParamsT* devParams",0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 2, "ppm double",								0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_DOUBLE, SDR_PAMRAS_DOUBLE, NULL, sdrplay_api_Update_Dev_Ppm, ppmComment, NULL },
	{ 2, "fsFreq sdrplay_api_FsFreqT",				0.0, 0.0, 0.0, 0.0, NULL , SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 3, "fsHz double",								8000000.0, sampleRate_8_M, 2000000.0, 10000000.0, &sdrplay_ap_sampleRate_map, SDR_PAMRAS_ENUM, SDR_PAMRAS_DOUBLE, NULL, sdrplay_api_Update_Dev_Fs, fsFreqComment, CSDR::set_params_SampleRate },
	{ 3, "syncUpdate unsigned char fsFreq",			0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_ENABLE_DISABLE, SDR_PAMRAS_ENABLE_DISABLE, NULL, sdrplay_api_Update_Dev_Fs, fsFreqComment, NULL },
	{ 3, "reCal unsigned char",						0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_UCHAR, SDR_PAMRAS_UCHAR, NULL, sdrplay_api_Update_Dev_Fs, fsFreqComment, NULL },
	{ 2, "syncUpdate sdrplay_api_SyncUpdateT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 3, "sampleNum unsigned int",					0.0, 0.0, 0.0, 1000000.0, NULL, SDR_PAMRAS_UINT, SDR_PAMRAS_UINT, NULL, sdrplay_api_Update_Dev_SyncUpdate, syncUpdateComment, NULL },
	{ 3, "period unsigned int",						0.0, 0.0, 0.0, 1000000.0, NULL, SDR_PAMRAS_UINT, SDR_PAMRAS_UINT, NULL, sdrplay_api_Update_Dev_SyncUpdate, syncUpdateComment, NULL },
	{ 2, "resetFlags sdrplay_api_ResetFlagsT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 3, "resetGainUpdate unsigned char",			0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_ENABLE_DISABLE, SDR_PAMRAS_ENABLE_DISABLE, NULL, sdrplay_api_Update_Dev_ResetFlags, resetFlagsComment, NULL },
	{ 3, "resetRfUpdate unsigned char",				0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_ENABLE_DISABLE, SDR_PAMRAS_ENABLE_DISABLE, NULL, sdrplay_api_Update_Dev_ResetFlags, resetFlagsComment, NULL },
	{ 3, "resetFsUpdate unsigned char",				0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_ENABLE_DISABLE, SDR_PAMRAS_ENABLE_DISABLE, NULL, sdrplay_api_Update_Dev_ResetFlags, resetFlagsComment, NULL },
	{ 2, "mode sdrplay_api_TransferModeT",			sdrplay_api_ISOCH, 0.0, 0.0, 0.0, &sdrplay_api_TransferModeT_map, SDR_PAMRAS_ENUM, SDR_PAMRAS_ENUM, NULL, sdrplay_api_Update_None, TransferModeComment, NULL },
	{ 2, "samplesPerPkt unsigned int",				0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UINT, SDR_PAMRAS_UINT, NULL, sdrplay_api_Update_None, samplesPerPktComment, NULL },
	{ 2, "rsp1aParams sdrplay_api_Rsp1aParamsT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 2, "rsp2Params sdrplay_api_Rsp2ParamsT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 2, "rspDuoParams sdrplay_api_RspDuoParamsT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 2, "rspDxParams sdrplay_api_RspDxParamsT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 1, "接收通道 A 参数 sdrplay_api_RxChannelParamsT* rxChannelA", 0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 2, "tunerParams sdrplay_api_TunerParamsT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 3, "bwType sdrplay_api_Bw_MHzT",					0.0, sdrplay_api_BW_0_200, 0.0, 0.0, &sdrplay_api_Bw_MHzT_map, SDR_PAMRAS_ENUM, SDR_PAMRAS_ENUM, NULL, sdrplay_api_Update_Tuner_BwType, bwTypeComment, NULL },
	{ 3, "ifType sdrplay_api_If_kHzT",					0.0, sdrplay_api_IF_Zero, 0.0, 0.0, &sdrplay_api_If_kHzT_map, SDR_PAMRAS_ENUM, SDR_PAMRAS_ENUM, NULL, sdrplay_api_Update_Tuner_IfType, ifTypeComment, NULL },
	{ 3, "loMode sdrplay_api_LoModeT",					0.0, sdrplay_api_LO_Auto, 0.0, 0.0, &sdrplay_api_LoModeT_map, SDR_PAMRAS_ENUM, SDR_PAMRAS_ENUM, NULL, sdrplay_api_Update_Tuner_LoMode, loModeComment, NULL },
	{ 3, "gain sdrplay_api_GainT",						0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 4, "gRdB int",								0.0, 50.0, 0.0, 59.0, NULL, SDR_PAMRAS_INT, SDR_PAMRAS_INT, NULL, sdrplay_api_Update_Tuner_Gr, gain_gRdBComment, NULL },
	{ 4, "LNAstate unsigned char",					0.0, 0.0, 0.0, 3.0, NULL, SDR_PAMRAS_UCHAR, SDR_PAMRAS_UCHAR, NULL, sdrplay_api_Update_Tuner_Gr, gain_LNAstateComment, NULL },
	{ 4, "syncUpdate unsigned char gain",				0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_ENABLE_DISABLE, SDR_PAMRAS_ENABLE_DISABLE, NULL, sdrplay_api_Update_Tuner_Gr, gain_syncUpdateComment, NULL },
	{ 4, "minGr sdrplay_api_MinGainReductionT",		0.0, sdrplay_api_NORMAL_MIN_GR, 0.0, 0.0, &sdrplay_api_MinGainReductionT_map, SDR_PAMRAS_ENUM, SDR_PAMRAS_ENUM, NULL, sdrplay_api_Update_Tuner_GrLimits, gain_minGrComment, NULL },
	{ 4, "gainVals sdrplay_api_GainValuesT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 5, "curr float",					0.0, 0.0, 0.0, 500.0, NULL, SDR_PAMRAS_FLOAT, SDR_PAMRAS_FLOAT, NULL, sdrplay_api_Update_None, gain_gainValsComment, NULL },
	{ 5, "max float",					0.0, 0.0, 0.0, 500.0, NULL, SDR_PAMRAS_FLOAT, SDR_PAMRAS_FLOAT, NULL, sdrplay_api_Update_None, gain_gainValsComment, NULL },
	{ 5, "min float",					0.0, 0.0, 0.0, 500.0, NULL, SDR_PAMRAS_FLOAT, SDR_PAMRAS_FLOAT, NULL, sdrplay_api_Update_None, gain_gainValsComment, NULL },
	{ 3, "rfFreq sdrplay_api_RfFreqT",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 4, "rfHz double",									0.0, 200000000.0, 0.0, 2000000000.0, NULL, SDR_PAMRAS_DOUBLE, SDR_PAMRAS_DOUBLE, NULL, sdrplay_api_Update_Tuner_Frf, rfFreq_rfHzComment, NULL },
	{ 4, "syncUpdate unsigned char rfFreq",				0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_ENABLE_DISABLE, SDR_PAMRAS_ENABLE_DISABLE, NULL, sdrplay_api_Update_Tuner_Frf, rfFreq_syncUpdateComment, NULL },
	{ 3, "dcOffsetTuner sdrplay_api_DcOffsetTunerT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 4, "dcCal unsigned char",					0.0, 3.0, 0.0, 5.0, NULL, SDR_PAMRAS_UCHAR, SDR_PAMRAS_UCHAR, NULL, sdrplay_api_Update_Tuner_DcOffset, dcOffsetTunerComment, NULL },
	{ 4, "speedUp unsigned char",				0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_UCHAR, SDR_PAMRAS_UCHAR, NULL, sdrplay_api_Update_Tuner_DcOffset, dcOffsetTunerComment, NULL },
	{ 4, "trackTime int",						0.0, 1.0, 0.0, 0.0, NULL, SDR_PAMRAS_INT, SDR_PAMRAS_INT, NULL,	sdrplay_api_Update_Tuner_DcOffset, dcOffsetTunerComment, NULL },
	{ 4, "refreshRateTime int",					0.0, 2048.0, 0.0, 0.0, NULL, SDR_PAMRAS_INT, SDR_PAMRAS_INT, NULL, sdrplay_api_Update_Tuner_DcOffset, dcOffsetTunerComment, NULL },
	{ 2, "ctrlParams sdrplay_api_ControlParamsT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 3, "dcOffset sdrplay_api_DcOffsetT",			0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 4, "DCenable unsigned char",					0.0, 1.0, 0.0, 1.0, NULL, SDR_PAMRAS_ENABLE_DISABLE, SDR_PAMRAS_ENABLE_DISABLE, NULL, sdrplay_api_Update_Ctrl_DCoffsetIQimbalance, dcOffsetComment, NULL },
	{ 4, "IQenable unsigned char",					0.0, 1.0, 0.0, 1.0, NULL, SDR_PAMRAS_ENABLE_DISABLE, SDR_PAMRAS_ENABLE_DISABLE, NULL, sdrplay_api_Update_Ctrl_DCoffsetIQimbalance, dcOffsetComment, NULL },
	{ 3, "decimation sdrplay_api_DecimationT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 4, "enable decimationFactor unsigned char",	0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_ENABLE_DISABLE, SDR_PAMRAS_ENABLE_DISABLE, NULL, sdrplay_api_Update_Ctrl_Decimation, decimationComment, CSDR::set_params_decimationFactorEnable },
	{ 4, "decimationFactor unsigned char",			0.0, decimationFactor1, 0.0, 0.0, &sdrplay_api_decimationFactor_map, SDR_PAMRAS_ENUM, SDR_PAMRAS_UCHAR, NULL, sdrplay_api_Update_Ctrl_Decimation, decimationComment, CSDR::set_params_decimationFactor },
	{ 4, "wideBandSignal unsigned char",			0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_UCHAR, SDR_PAMRAS_UCHAR, NULL, sdrplay_api_Update_Ctrl_Decimation, decimationComment, NULL },
	{ 3, "agc sdrplay_api_AgcT",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 4, "enable sdrplay_api_AgcControlT",			0.0, sdrplay_api_AGC_50HZ, 0.0, 0.0, &sdrplay_api_AgcControlT_map, SDR_PAMRAS_ENUM, SDR_PAMRAS_ENUM, NULL, sdrplay_api_Update_Ctrl_Agc, agcComment, NULL },
	{ 4, "setPoint_dBfs int",						0.0, -60.0, 0.0, 0.0, NULL, SDR_PAMRAS_INT, SDR_PAMRAS_INT, NULL, sdrplay_api_Update_Ctrl_Agc, agcComment, NULL },
	{ 4, "attack_ms unsigned short",				0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_USHORT, SDR_PAMRAS_USHORT, NULL, sdrplay_api_Update_Ctrl_Agc, agcComment, NULL },
	{ 4, "decay_ms unsigned short",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_USHORT, SDR_PAMRAS_USHORT, NULL, sdrplay_api_Update_Ctrl_Agc, agcComment, NULL },
	{ 4, "decay_delay_ms unsigned short",			0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_USHORT, SDR_PAMRAS_USHORT, NULL, sdrplay_api_Update_Ctrl_Agc, agcComment, NULL },
	{ 4, "decay_threshold_dB unsigned short",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_USHORT, SDR_PAMRAS_USHORT, NULL, sdrplay_api_Update_Ctrl_Agc, agcComment, NULL },
	{ 4, "syncUpdate int agc",						0.0, 0.0, 0.0, 1.0, NULL, SDR_PAMRAS_INT, SDR_PAMRAS_INT, NULL, sdrplay_api_Update_Ctrl_Agc, agcComment, NULL },
	{ 3, "adsbMode sdrplay_api_AdsbModeT",			0.0, sdrplay_api_ADSB_DECIMATION, 0.0, 0.0, &sdrplay_api_AdsbModeT_map, SDR_PAMRAS_ENUM, SDR_PAMRAS_ENUM, NULL, sdrplay_api_Update_Ctrl_AdsbMode, adsbModeComment, NULL },
	{ 2, "rsp1aTunerParams sdrplay_api_Rsp1aTunerParamsT  ",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 2, "rsp2TunerParams sdrplay_api_Rsp2TunerParamsT   ",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 2, "rspDuoTunerParams sdrplay_api_RspDuoTunerParamsT ",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 2, "rspDxTunerParams sdrplay_api_RspDxTunerParamsT  ",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, sdrplay_api_Update_None, NULL, NULL },
	{ 1, "接收通道 B 参数 sdrplay_api_RxChannelParamsT* rxChannelB", 0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, SDR_PAMRAS_NONE, NULL, NULL, NULL, NULL }
};

void CSDR::Init_ValueAddr(void)
{
	//{ 0, "SDR RSP1 接收机 sdrplay_api_DeviceParamsT", 0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 1, "设备参数 sdrplay_api_DevParamsT* devParams",0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 2, "ppm double",								0.0, 0.0, 20.0, 0.0, NULL, SDR_PAMRAS_DOUBLE, NULL },
	get_ValueAddr("ppm double", (void*)&clsGetDataSDR.deviceParams->devParams->ppm);
	//{ 2, "fsFreq sdrplay_api_FsFreqT",				0.0, 0.0, 0.0, 0.0, NULL , SDR_PAMRAS_NONE, NULL },
	//{ 3, "fsHz double",								0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_DOUBLE, NULL },
	get_ValueAddr("fsHz double", (void*)&clsGetDataSDR.deviceParams->devParams->fsFreq.fsHz);
	//{ 3, "syncUpdate unsigned char fsFreq",				0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("syncUpdate unsigned char fsFreq", (void*)&clsGetDataSDR.deviceParams->devParams->fsFreq.syncUpdate);
	//{ 3, "reCal unsigned char",						0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("reCal unsigned char", (void*)&clsGetDataSDR.deviceParams->devParams->fsFreq.reCal);
	//{ 2, "syncUpdate sdrplay_api_SyncUpdateT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 3, "sampleNum unsigned int",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UINT, NULL },
	get_ValueAddr("sampleNum unsigned int", (void*)&clsGetDataSDR.deviceParams->devParams->syncUpdate.sampleNum);
	//{ 3, "period unsigned int",						0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UINT, NULL },
	get_ValueAddr("period unsigned int", (void*)&clsGetDataSDR.deviceParams->devParams->syncUpdate.period);
	//{ 2, "resetFlags sdrplay_api_ResetFlagsT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 3, "resetGainUpdate unsigned char",			0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("resetGainUpdate unsigned char", (void*)&clsGetDataSDR.deviceParams->devParams->resetFlags.resetGainUpdate);
	//{ 3, "resetRfUpdate unsigned char",				0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("resetRfUpdate unsigned char", (void*)&clsGetDataSDR.deviceParams->devParams->resetFlags.resetRfUpdate);
	//{ 3, "resetFsUpdate unsigned char",				0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("resetFsUpdate unsigned char", (void*)&clsGetDataSDR.deviceParams->devParams->resetFlags.resetFsUpdate);
	//{ 2, "mode sdrplay_api_TransferModeT",			sdrplay_api_ISOCH, 0.0, 0.0, 0.0, &sdrplay_api_TransferModeT_map, SDR_PAMRAS_ENUM, NULL },
	get_ValueAddr("mode sdrplay_api_TransferModeT", (void*)&clsGetDataSDR.deviceParams->devParams->mode);
	//{ 2, "samplesPerPkt unsigned int",				0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UINT, NULL },
	get_ValueAddr("samplesPerPkt unsigned int", (void*)&clsGetDataSDR.deviceParams->devParams->samplesPerPkt);
	//{ 2, "rsp1aParams sdrplay_api_Rsp1aParamsT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 2, "rsp2Params sdrplay_api_Rsp2ParamsT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 2, "rspDuoParams sdrplay_api_RspDuoParamsT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 2, "rspDxParams sdrplay_api_RspDxParamsT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 1, "接收通道 A 参数 sdrplay_api_RxChannelParamsT* rxChannelA", 0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 2, "tunerParams sdrplay_api_TunerParamsT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 3, "bwType sdrplay_api_Bw_MHzT",					0.0, 0.0, 0.0, 0.0, &sdrplay_api_Bw_MHzT_map, SDR_PAMRAS_ENUM, NULL },
	get_ValueAddr("bwType sdrplay_api_Bw_MHzT", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.bwType);
	//{ 3, "ifType sdrplay_api_If_kHzT",					0.0, 0.0, 0.0, 0.0, &sdrplay_api_If_kHzT_map, SDR_PAMRAS_ENUM, NULL },
	get_ValueAddr("ifType sdrplay_api_If_kHzT", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.ifType);
	//{ 3, "loMode sdrplay_api_LoModeT",					0.0, 0.0, 0.0, 0.0, &sdrplay_api_LoModeT_map, SDR_PAMRAS_ENUM, NULL },
	get_ValueAddr("loMode sdrplay_api_LoModeT", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.loMode);
	//{ 3, "gain sdrplay_api_GainT",						0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 4, "gRdB int",								0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_INT, NULL },
	get_ValueAddr("gRdB int", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.gain.gRdB);
	//{ 4, "LNAstate unsigned char",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("LNAstate unsigned char", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.gain.LNAstate);
	//{ 4, "syncUpdate unsigned char gain",				0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("syncUpdate unsigned char gain", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.gain.syncUpdate);
	//{ 4, "minGr sdrplay_api_MinGainReductionT",		0.0, 0.0, 0.0, 0.0, &sdrplay_api_MinGainReductionT_map, SDR_PAMRAS_ENUM, NULL },
	get_ValueAddr("minGr sdrplay_api_MinGainReductionT", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.gain.minGr);
	//{ 4, "gainVals sdrplay_api_GainValuesT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 5, "curr float",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	get_ValueAddr("curr float", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.gain.gainVals.curr);
	//{ 5, "max float",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	get_ValueAddr("max float", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.gain.gainVals.max);
	//{ 5, "min float",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	get_ValueAddr("min float", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.gain.gainVals.min);
	//{ 3, "rfFreq sdrplay_api_RfFreqT",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 4, "rfHz double",									0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_DOUBLE, NULL },
	get_ValueAddr("rfHz double", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.rfFreq.rfHz);
	//{ 4, "syncUpdate unsigned char rfFreq",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("syncUpdate unsigned char rfFreq", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.rfFreq.syncUpdate);
	//{ 3, "dcOffsetTuner sdrplay_api_DcOffsetTunerT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 4, "dcCal unsigned char",					0.0, 3.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("dcCal unsigned char", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.dcOffsetTuner.dcCal);
	//{ 4, "speedUp unsigned char",				0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("speedUp unsigned char", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.dcOffsetTuner.speedUp);
	//{ 4, "trackTime int",						0.0, 1.0, 0.0, 0.0, NULL, SDR_PAMRAS_INT, NULL },
	get_ValueAddr("trackTime int", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.dcOffsetTuner.trackTime);
	//{ 4, "refreshRateTime int",					0.0, 2048.0, 0.0, 0.0, NULL, SDR_PAMRAS_INT, NULL },
	get_ValueAddr("refreshRateTime int", (void*)&clsGetDataSDR.deviceParams->rxChannelA->tunerParams.dcOffsetTuner.refreshRateTime);
	//{ 2, "ctrlParams sdrplay_api_ControlParamsT",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 3, "dcOffset sdrplay_api_DcOffsetT",			0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 4, "DCenable unsigned char",					0.0, 1.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("DCenable unsigned char", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.dcOffset.DCenable);
	//{ 4, "IQenable unsigned char",					0.0, 1.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("IQenable unsigned char", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.dcOffset.IQenable);
	//{ 3, "decimation sdrplay_api_DecimationT",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 4, "enable decimationFactor unsigned char",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("enable decimationFactor unsigned char", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.decimation.enable);
	//{ 4, "decimationFactor unsigned char",			0.0, 1.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("decimationFactor unsigned char", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.decimation.decimationFactor);
	//{ 4, "wideBandSignal unsigned char",			0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_UCHAR, NULL },
	get_ValueAddr("wideBandSignal unsigned char", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.decimation.wideBandSignal);
	//{ 3, "agc sdrplay_api_AgcT",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 4, "enable sdrplay_api_AgcControlT",			0.0, 0.0, 0.0, 0.0, &sdrplay_api_AgcControlT_map, SDR_PAMRAS_ENUM, NULL },
	get_ValueAddr("enable sdrplay_api_AgcControlT", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.agc.enable);
	//{ 4, "setPoint_dBfs int",						0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_INT, NULL },
	get_ValueAddr("setPoint_dBfs int", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.agc.setPoint_dBfs);
	//{ 4, "attack_ms unsigned short",				0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_USHORT, NULL },
	get_ValueAddr("attack_ms unsigned short", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.agc.attack_ms);
	//{ 4, "decay_ms unsigned short",					0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_USHORT, NULL },
	get_ValueAddr("decay_ms unsigned short", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.agc.decay_ms);
	//{ 4, "decay_delay_ms unsigned short",			0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_USHORT, NULL },
	get_ValueAddr("decay_delay_ms unsigned short", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.agc.decay_delay_ms);
	//{ 4, "decay_threshold_dB unsigned short",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_USHORT, NULL },
	get_ValueAddr("decay_threshold_dB unsigned short", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.agc.decay_threshold_dB);
	//{ 4, "syncUpdate int agc",							0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_INT, NULL },
	get_ValueAddr("syncUpdate int agc", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.agc.syncUpdate);
	//{ 3, "adsbMode sdrplay_api_AdsbModeT",			0.0, 0.0, 0.0, 0.0, &sdrplay_api_AdsbModeT_map, SDR_PAMRAS_ENUM, NULL },
	get_ValueAddr("adsbMode sdrplay_api_AdsbModeT", (void*)&clsGetDataSDR.deviceParams->rxChannelA->ctrlParams.adsbMode);
	//{ 2, "rsp1aTunerParams sdrplay_api_Rsp1aTunerParamsT  ",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 2, "rsp2TunerParams sdrplay_api_Rsp2TunerParamsT   ",		0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 2, "rspDuoTunerParams sdrplay_api_RspDuoTunerParamsT ",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 2, "rspDxTunerParams sdrplay_api_RspDxTunerParamsT  ",	0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL },
	//{ 1, "接收通道 B 参数 sdrplay_api_RxChannelParamsT* rxChannelB", 0.0, 0.0, 0.0, 0.0, NULL, SDR_PAMRAS_NONE, NULL }

}



CSDR::CSDR()
{
	max_index = sizeof(SDR_params) / sizeof(SDRParams);
}

CSDR::~CSDR()
{

}

void CSDR::get_ValueAddr(const char* pdesc, void* p)
{
	int n = sizeof(SDR_params) / sizeof(SDR_params[0]);
	int i;
	for (i = 0; i < n; i++)
	{
		if (strcmp(SDR_params[i].txt, pdesc) == 0) 
		{ 
			SDR_params[i].pValue = p; 
			return; 
		}
	}
}

INT CSDR::get_ValueIndex(const char* pdesc)
{
	int n = sizeof(SDR_params) / sizeof(SDR_params[0]);
	int i;
	for (i = 0; i < n; i++)
	{
		if (strcmp(SDR_params[i].txt, pdesc) == 0)
		{
			return i;
		}
	}
	return -1;
}

void CSDR::buildTreeItems(HWND hWndTreeView, HTREEITEM hItem, int* params_index)
{
	HTREEITEM hItemI;

	static int I = sizeof(SDR_params) / sizeof(SDR_params[0]);

	TVINSERTSTRUCT tvis = { 0 };

	int level = SDR_params[*params_index].level;

	do {
		tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
		tvis.hParent = hItem;
		tvis.item.lParam = (LPARAM)(*params_index);
		if (*params_index == 0) {
			static char s[1000];
			sprintf(s, "%s SerNo=%s hwVer=%d tuner=0x%.2x",
				SDR_params[*params_index].txt,
				clsGetDataSDR.chosenDevice->SerNo,
				clsGetDataSDR.chosenDevice->hwVer,
				clsGetDataSDR.chosenDevice->tuner);
			tvis.item.pszText = s;
		}
		else 
		tvis.item.pszText = SDR_params[*params_index].txt;
		tvis.item.cchTextMax = _tcslen(SDR_params[*params_index].txt);

		hItemI = TreeView_InsertItem(hWndTreeView, &tvis);
		TreeView_SelectItem(hWndTreeView, hItemI);

		HTREEITEM hItems = (HTREEITEM)TreeView_GetSelection(hWndTreeView);
		TVITEM tvi = { 0 };
		tvi.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
		tvi.hItem = hItems;
		TreeView_GetItem(hWndTreeView, &tvi);
		refresh_tree_item(hWndTreeView, &tvi, *params_index);

		(*params_index)++;
		if (*params_index == I) break;
		if (SDR_params[*params_index].level > level)
		{
			buildTreeItems(hWndTreeView, hItemI, params_index);
			TreeView_Expand(hWndTreeView, hItemI, TVE_EXPAND);
		}
	} while (SDR_params[*params_index].level == level && *params_index < I);
}

void CSDR::refresh_tree_item(HWND hWndTreeView, TVITEM* ptvi, int i)
{
	static char s[1000];
	s[0] = 0;

	if (SDR_params[i].pValue == NULL) DbgMsg("refresh_tree_item, %s\r\n", SDR_params[i].txt);

	switch (SDR_params[i].valueType)
	{
	case SDR_PAMRAS_DOUBLE:
		sprintf(s, "%s = %.2lf", SDR_params[i].txt, *(double*)(SDR_params[i].pValue));
		break;
	case SDR_PAMRAS_FLOAT:
		sprintf(s, "%s = %.2f", SDR_params[i].txt, *(float*)(SDR_params[i].pValue));
		break;
	case SDR_PAMRAS_INT:
		sprintf(s, "%s = %d", SDR_params[i].txt, *(int*)(SDR_params[i].pValue));
		break;
	case SDR_PAMRAS_ENUM:
		sprintf(s, "%s = %d : %s", SDR_params[i].txt, *(int*)(SDR_params[i].pValue),
			map_value_to_key(SDR_params[i].pEnumMap, *(int*)(SDR_params[i].pValue)));
		break;
	case SDR_PAMRAS_UINT:
		sprintf(s, "%s = %u", SDR_params[i].txt, *(unsigned int*)(SDR_params[i].pValue));
		break;
	case SDR_PAMRAS_USHORT:
		sprintf(s, "%s = %hu", SDR_params[i].txt, *(unsigned short*)(SDR_params[i].pValue));
		break;
	case SDR_PAMRAS_UCHAR:
		sprintf(s, "%s = %hhu", SDR_params[i].txt, *(unsigned char*)(SDR_params[i].pValue));
		break;
	}
	if (s[0] != 0)
	{
		ptvi->pszText = s;
		TreeView_SetItem(hWndTreeView, ptvi);
	}
}

void CSDR::refresh_input_panel(int index)
{
	char str[100];
	switch (SDR_params[index].valueType)
	{
	case SDR_PAMRAS_DOUBLE:
	case SDR_PAMRAS_FLOAT:
	case SDR_PAMRAS_INT:
	case SDR_PAMRAS_UINT:
	case SDR_PAMRAS_USHORT:
	case SDR_PAMRAS_UCHAR:
	{
		static char s[1000];
		int i = 0;
		switch (SDR_params[index].valueType)
		{
		case SDR_PAMRAS_DOUBLE:
			i += sprintf(s + i, "default = %.2lf, min = %.2lf, max = %.2lf\r\n\r\n", 
				(double)SDR_params[index].default, (double)SDR_params[index].min, (double)SDR_params[index].max);
			sprintf(str, "%.2lf", *(double*)SDR_params[index].pValue);
			SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str);
			break;
		case SDR_PAMRAS_FLOAT:
			i += sprintf(s + i, "default = %.2f, min = %.2f, max = %.2f\r\n\r\n", 
				(float)SDR_params[index].default, (float)SDR_params[index].min, (float)SDR_params[index].max);
			sprintf(str, "%.2f", *(float*)SDR_params[index].pValue);
			SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str);
			break;
		case SDR_PAMRAS_INT:
			i += sprintf(s + i, "default = %d, min = %d, max = %d\r\n\r\n", 
				(int)SDR_params[index].default, (int)SDR_params[index].min, (int)SDR_params[index].max);
			sprintf(str, "%d", *(int*)SDR_params[index].pValue);
			SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str);
			break;
		case SDR_PAMRAS_UINT:
			i += sprintf(s + i, "default = %u, min = %u, max = %u\r\n\r\n", 
				(unsigned int)SDR_params[index].default, (unsigned int)SDR_params[index].min, (unsigned int)SDR_params[index].max);
			SetDlgItemInt(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, *(unsigned int*)SDR_params[index].pValue, false);
			break;
		case SDR_PAMRAS_USHORT:
			i += sprintf(s + i, "default = %hu, min = %hu, max = %hu\r\n\r\n",
				(unsigned short)SDR_params[index].default, (unsigned short)SDR_params[index].min, (unsigned short)SDR_params[index].max);
			SetDlgItemInt(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, *(unsigned short*)SDR_params[index].pValue, false);
			break;
		case SDR_PAMRAS_UCHAR:
			i += sprintf(s + i, "default = %hhu, min = %hhu, max = %hhu\r\n\r\n",
				(unsigned char)SDR_params[index].default, (unsigned char)SDR_params[index].min, (unsigned char)SDR_params[index].max);
			SetDlgItemInt(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, *(unsigned char*)SDR_params[index].pValue, false);
			break;
		}
		SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_STATIC_SDR_PARAMS_TITLE, SDR_params[index].txt);
		if(SDR_params[index].comment != NULL) 
			i += sprintf(s + i, "%s", SDR_params[index].comment);
		SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_STATIC_SDR_PARAMS_COMMENT, s);
		ShowWindow(clsWinSDR.hWndTitle, SW_SHOW);
		ShowWindow(clsWinSDR.hWndCombox, SW_HIDE);
		ShowWindow(clsWinSDR.hWndEdit, SW_SHOW);
		ShowWindow(clsWinSDR.hWndComment, SW_SHOW);
	}
	break;
	case SDR_PAMRAS_ENUM:
		SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_STATIC_SDR_PARAMS_TITLE, SDR_params[index].txt);
		Init_Combox(index);

		ShowWindow(clsWinSDR.hWndTitle, SW_SHOW);
		ShowWindow(clsWinSDR.hWndCombox, SW_SHOW);
		ShowWindow(clsWinSDR.hWndEdit, SW_HIDE);
		ShowWindow(clsWinSDR.hWndComment, SW_SHOW);
		break;
	case SDR_PAMRAS_NONE:
	defult:
		ShowWindow(clsWinSDR.hWndTitle, SW_HIDE);
		ShowWindow(clsWinSDR.hWndCombox, SW_HIDE);
		ShowWindow(clsWinSDR.hWndEdit, SW_HIDE);
		ShowWindow(clsWinSDR.hWndComment, SW_HIDE);
		break;
	}
}

void CSDR::Init_Combox(int index)
{

	static char s[1000];
	SDR_ENUM_MAP *pMap = SDR_params[index].pEnumMap;
	int n = pMap->size();
	int i = 0;
	int itemi = 0;
	i += sprintf(s + i, "default : %s = %d\r\n\r\n", map_value_to_key(pMap, (int)SDR_params[index].default), (int)SDR_params[index].default);

	//delete all item
	SendMessage(clsWinSDR.hWndCombox, (UINT)CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
	for (SDR_ENUM_MAP::iterator it = pMap->begin(); it != pMap->end(); it++)
	{
		i += sprintf(s + i, "%s = %d\r\n", it->first, it->second);
	
		// Add string to combobox.
		SendMessage(clsWinSDR.hWndCombox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)it->first);
		if (*(int*)SDR_params[index].pValue == it->second)
		{
			//SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_COMBO_SDR_PARAMS, it->first);
			SendMessage(clsWinSDR.hWndCombox, (UINT)CB_SETCURSEL, (WPARAM)itemi, (LPARAM)0);
		}
		itemi++;
	}
	SendMessage(clsWinSDR.hWndCombox, (UINT)CB_SELECTSTRING, (WPARAM)0, (LPARAM)s);

	if (SDR_params[index].comment != NULL) i += sprintf(s + i, "\r\n%s", SDR_params[index].comment);
	SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_STATIC_SDR_PARAMS_COMMENT, s);

	// Send the CB_SETCURSEL message to display an initial item 
	//  in the selection field  
	//SendMessage(clsWinSDR.hWndCombox, CB_SETCURSEL, (WPARAM)2, (LPARAM)0);
}

const char* CSDR::map_value_to_key(SDR_ENUM_MAP *pmap, int value)
{
	for (SDR_ENUM_MAP::iterator it = pmap->begin(); it != pmap->end(); it++)
	{
		if (it->second == value) return it->first;
	}
	return NULL;
}

void CSDR::SDR_params_apply(void)
{
	DbgMsg("SDR_params_apply\r\n");
	//if (SDR_params[sel_SDR_params_index].valueType != SDR_PAMRAS_NONE)
		//clsSDR.refresh_tree_item(clsWinSDR.hWndTreeView, &sel_tvi, sel_SDR_params_index);
	if (SDR_params[sel_SDR_params_index].paramUpdateReason == sdrplay_api_Update_None) return;
	SDR_parmas_changed = false;

	refresh_tree_item(clsWinSDR.hWndTreeView, &sel_tvi, sel_SDR_params_index);

	DbgMsg("sdrplay_api_Update %d.\r\n", SDR_params[sel_SDR_params_index].paramUpdateReason);

	sdrplay_api_ErrT err;
	if ((err = sdrplay_api_Update(clsGetDataSDR.chosenDevice->dev, clsGetDataSDR.chosenDevice->tuner,
		(sdrplay_api_ReasonForUpdateT)SDR_params[sel_SDR_params_index].paramUpdateReason, sdrplay_api_Update_Ext1_None)) !=
		sdrplay_api_Success)
	{
		DbgMsg("sdrplay_api_Update %d failed %s\n",
			SDR_params[sel_SDR_params_index].paramUpdateReason,
			sdrplay_api_GetErrorString(err));
	}
	else 
	{
		if (SDR_params[sel_SDR_params_index].p_set_params_func != NULL)SDR_params[sel_SDR_params_index].p_set_params_func(sel_SDR_params_index);
	}

}

void CSDR::edit_check_range(void)
{
	if (sel_SDR_params_index < 0) return;

	static char str[100];
	GetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str, 0);

	int index = sel_SDR_params_index;
	switch (SDR_params[index].valueType)
	{
	case SDR_PAMRAS_DOUBLE:
	{
		GetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str, 99);
		double dd = std::stold(str);
		if(SDR_params[index].max != 0) dd = dd < SDR_params[index].min ? SDR_params[index].min : (dd > SDR_params[index].max ? SDR_params[index].max : dd);
		double d = *(double*)SDR_params[index].pValue;
		if (d != dd)
		{
			sprintf(str, "%.2lf", dd);
			SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str);
			*(double*)(SDR_params[index].pValue) = dd;
			refresh_tree_item(clsWinSDR.hWndTreeView, &clsSDR.sel_tvi, index);
			SDR_parmas_changed = true;
		}
	}
		break;
	case SDR_PAMRAS_FLOAT:
	{
		GetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str, 99);
		float dd = std::stof(str);
		if (SDR_params[index].max != 0) dd = dd < (float)SDR_params[index].min ? (float)SDR_params[index].min : 
			(dd > (float)SDR_params[index].max ? (float)SDR_params[index].max : dd);
		float d = *(float*)SDR_params[index].pValue;
		if (d != dd)
		{
			sprintf(str, "%.2f", dd);
			SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str);
			*(float*)(SDR_params[index].pValue) = dd;
			refresh_tree_item(clsWinSDR.hWndTreeView, &clsSDR.sel_tvi, index);
			SDR_parmas_changed = true;
		}
	}
		break;
	case SDR_PAMRAS_INT:
	{
		GetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str, 99);
		int dd = std::stoi(str);
		if (SDR_params[index].max != 0) dd = dd < (int)SDR_params[index].min ? (int)SDR_params[index].min : 
			(dd > (int)SDR_params[index].max ? (int)SDR_params[index].max : dd);
		int d = *(int*)SDR_params[index].pValue;
		if (d != dd)
		{
			sprintf(str, "%d", dd);
			SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str);
			*(int*)(SDR_params[index].pValue) = dd;
			refresh_tree_item(clsWinSDR.hWndTreeView, &clsSDR.sel_tvi, index);
			SDR_parmas_changed = true;
		}
	}
		break;
	case SDR_PAMRAS_UINT:
	{
		GetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str, 99);
		unsigned int dd = (unsigned int)std::stoul(str);
		if (SDR_params[index].max != 0) dd = dd < (unsigned int)SDR_params[index].min ? (unsigned int)SDR_params[index].min : 
			(dd > (unsigned int)SDR_params[index].max ? (unsigned int)SDR_params[index].max : dd);
		unsigned int d = *(unsigned int*)SDR_params[index].pValue;
		if (d != dd)
		{
			sprintf(str, "%u", dd);
			SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str);
			*(unsigned int*)(SDR_params[index].pValue) = dd;
			refresh_tree_item(clsWinSDR.hWndTreeView, &clsSDR.sel_tvi, index);
			SDR_parmas_changed = true;
		}
	}
	break;
	case SDR_PAMRAS_USHORT:
	{
		GetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str, 99);
		unsigned short dd = (unsigned short)std::stoul(str);
		if (SDR_params[index].max != 0) dd = dd < (unsigned short)SDR_params[index].min ? (unsigned short)SDR_params[index].min : 
			(dd > (unsigned short)SDR_params[index].max ? (unsigned short)SDR_params[index].max : dd);
		unsigned short d = *(unsigned short*)SDR_params[index].pValue;
		if (d != dd)
		{
			sprintf(str, "%hu", dd);
			SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str);
			*(unsigned short*)(SDR_params[index].pValue) = dd;
			refresh_tree_item(clsWinSDR.hWndTreeView, &clsSDR.sel_tvi, index);
			SDR_parmas_changed = true;
		}
	}
	break;
	case SDR_PAMRAS_UCHAR:
	{
		GetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str, 99);
		unsigned char dd = (unsigned char)std::stoul(str);
		if (SDR_params[index].max != 0) dd = dd < (unsigned char)SDR_params[index].min ? (unsigned char)SDR_params[index].min : 
			(dd > (unsigned char)SDR_params[index].max ? (unsigned char)SDR_params[index].max : dd);
		unsigned char d = *(unsigned char*)SDR_params[index].pValue;
		if (d != dd)
		{
			sprintf(str, "%hhu", dd);
			SetDlgItemText(clsWinSDR.hDlgSDRParams, IDC_EDIT_SDR_PARAMS, str);
			*(unsigned char*)(SDR_params[index].pValue) = dd;
			refresh_tree_item(clsWinSDR.hWndTreeView, &clsSDR.sel_tvi, index);
			SDR_parmas_changed = true;
		}
	}
	break;
	case SDR_PAMRAS_ENUM:
	{
		int ItemIndex = SendMessage((HWND)clsWinSDR.hWndCombox, (UINT)CB_GETCURSEL,
			(WPARAM)0, (LPARAM)0);
		char  ListItem[256] = { 0 };
		(TCHAR)SendMessage((HWND)clsWinSDR.hWndCombox, (UINT)CB_GETLBTEXT,
			(WPARAM)ItemIndex, (LPARAM)ListItem);
		SDR_ENUM_MAP *pmap = SDR_params[index].pEnumMap;
		SDR_ENUM_MAP::iterator it; 
		for (it = pmap->begin(); it != pmap->end(); it++)
		{
			if (strcmp(it->first, ListItem) == 0)break;
		}
		
		//if (it != pmap->end()) 
		{
			int dd = (int)it->second;
			if (*(int*)(SDR_params[index].pValue) != dd)
			{
				*(int*)(SDR_params[index].pValue) = dd;
				refresh_tree_item(clsWinSDR.hWndTreeView, &clsSDR.sel_tvi, index);
				SDR_parmas_changed = true;
			}
		}
	}
	case SDR_PAMRAS_NONE:
	defalut:
		break;
	}
}

void CSDR::set_params_SampleRate(int index)
{
	//clsSDR.SdrSampleRate = *(double*)SDR_params[index].pValue;
	//AdcDataI->SampleRate = clsSDR.DecimationFactorEnable != 0 ? clsSDR.SdrSampleRate / clsSDR.DecimationFactor : clsSDR.SdrSampleRate;
	
	clsMainFilterI.SrcData->SampleRate = clsGetDataSDR.chParams->ctrlParams.decimation.enable != 0 ?
		clsGetDataSDR.deviceParams->devParams->fsFreq.fsHz / clsGetDataSDR.chParams->ctrlParams.decimation.decimationFactor : 
		clsGetDataSDR.deviceParams->devParams->fsFreq.fsHz;
	//clsMainFilterI.SrcData->SampleRate = clsMainFilterI.SrcData->SampleRate >> 1;
	clsMainFilterI.ReBuildFilterCore();

	//clsMainFilterQ.SrcData->SampleRate = clsMainFilterI.SrcData->SampleRate;
	//clsMainFilterQ.ReBuildFilterCore();

	DbgMsg("set_params_SampleRate\r\n");
}

void CSDR::set_params_decimationFactorEnable(int index)
{
	DbgMsg("set_params_decimationFactorEnable\r\n");
	//set_params_decimationFactor(clsSDR.get_ValueIndex("decimationFactor unsigned char"));
	set_params_SampleRate(clsSDR.get_ValueIndex("fsHz double"));
}

void CSDR::set_params_decimationFactor(int index)
{
	DbgMsg("set_params_decimationFactor\r\n");
	set_params_SampleRate(clsSDR.get_ValueIndex("fsHz double"));
}