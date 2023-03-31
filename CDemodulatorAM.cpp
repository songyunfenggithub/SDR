

#include "CDemodulatorAM.h"

CDemodulatorAM clsDemodulatorAm;

CDemodulatorAM::CDemodulatorAM()
{
	RestoreValue();
}

CDemodulatorAM::~CDemodulatorAM()
{
	SaveValue();
}

void CDemodulatorAM::build_AM_Filter_Core(void)
{
	pFilterInfo = new CWaveFilter::FILTERINFO;
	char s[1000];
	sprintf(s, "129,0,0;2,%d,100", clsWaveData.AdcSampleRate >> (DEMODULATOR_AM_FILTER_SAMPLERATE_OFFSET_BIT + DEMODULATOR_AM_DECIMATION_FACTOR_BIT));
	clsWaveFilter.setFilterCoreDesc(pFilterInfo, s);
	
	//clsWaveFilter.setFilterCoreDesc(pFilterInfo, AMFilterCoreDesc);

}

void CDemodulatorAM::SaveValue(void)
{
	WritePrivateProfileString("CDemodulatorAM", "AMFilterCoreDesc", pFilterInfo->CoreDescStr, IniFilePath);
}

void CDemodulatorAM::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	GetPrivateProfileString("CDemodulatorAM", "AMFilterCoreDesc", "1", AMFilterCoreDesc, FILTER_DESC_LENGTH, IniFilePath);
}