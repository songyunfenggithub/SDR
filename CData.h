#pragma once

#include "public.h"

//#define GET_BUFFER_LENGTH	1024
//16M buffer
#define DATA_BUFFER_LENGTH					0x8000000UL
#define DATA_BUFFER_MASK					0x7FFFFFFUL

//#define ADS1262
//#define W801
#define SDR
//#define USART
//#define USART_INMP441
//#define SOUNDCARD

#define ADC_DATA_MAX_ZOOM_BIT				8

#define FULL_VOTAGE							3.3

#ifdef ADS1262
#define DATA_BYTE_TO_POSITION_MOVEBIT		2   
#define ADC_DATA_SAMPLE_BIT					32
#define ADC_SAMPLE_RATE						1000UL
typedef INT32    ADCDATATYPE, *PADCDATATYPE;
#endif

#ifdef W801
#define DATA_BYTE_TO_POSITION_MOVEBIT		2   
#define ADC_DATA_SAMPLE_BIT					32
#define ADC_SAMPLE_RATE						256UL
typedef INT16    ADCDATATYPE, * PADCDATATYPE;
#endif

#ifdef SOUNDCARD
#define DATA_BYTE_TO_POSITION_MOVEBIT		1
#define ADC_DATA_SAMPLE_BIT					16
#define ADC_SAMPLE_RATE						11025UL
typedef INT16    ADCDATATYPE, *PADCDATATYPE;
#endif

#ifdef SDR
#define DATA_BYTE_TO_POSITION_MOVEBIT		1
#define ADC_DATA_SAMPLE_BIT					16
#define ADC_SAMPLE_RATE						2000000UL
typedef short		ADCDATATYPE,	* PADCDATATYPE;
typedef	float		FILTEDDATATYPE, * PFILTEDDATATYPE;
#endif

#ifdef USART
#define DATA_BYTE_TO_POSITION_MOVEBIT		1   
#define ADC_DATA_SAMPLE_BIT					13
#define ADC_SAMPLE_RATE						5000UL
typedef INT16    ADCDATATYPE, * PADCDATATYPE;
#endif

#ifdef USART_INMP441
#define DATA_BYTE_TO_POSITION_MOVEBIT		2   
#define ADC_DATA_SAMPLE_BIT					32
#define ADC_SAMPLE_RATE						5000UL
typedef INT32    ADCDATATYPE, * PADCDATATYPE;
#endif

enum WHICHSIGNAL {
	SIGNAL_ORIGNAL = 0,
	SIGNAL_FILTTED = 1
};

class CData
{
public:

public:
	ADCDATATYPE	AdcBuff[DATA_BUFFER_LENGTH];
	UINT AdcPos = 0;
	UINT AdcGetCharLength = 0;
	bool AdcGetNew = false;
	UINT AdcSampleRate;

	UINT NumPerSec = 0;
	UINT NumPerSecInProcess = 0;

	FILTEDDATATYPE  FilttedBuff[DATA_BUFFER_LENGTH];
	UINT8	FilttedFlag[DATA_BUFFER_LENGTH] = {0};
	UINT	FilttingPos = 0, FilttedPos = 0, FilttedBuffPos = 0;

	UINT	FilttedForwardBuff[DATA_BUFFER_LENGTH];
	UINT  FilttedForwardPos = 0;

public:
	CData();
	virtual ~CData();

	void GeneratorWave(void);
	static TIMERPROC NumPerSecTimer_Func(void);

};

extern CData clsData;


