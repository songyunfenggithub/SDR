#pragma once

typedef enum BUFF_DATA_TYPE_ENUM {
	u_char_type,
	u_short_type,
	u_int_type,
	u_int64_type,
	char_type,
	short_type,
	int_type,
	int64_type,
	float_type,
	double_type
} BUFF_DATA_TYPE;

//#define GET_BUFFER_LENGTH	1024
//16M buffer
#define DATA_BUFFER_LENGTH					0x8000000UL

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
typedef INT32    ADC_DATA_TYPE, *PADC_DATA_TYPE;
#endif

#ifdef W801
#define DATA_BYTE_TO_POSITION_MOVEBIT		2   
#define ADC_DATA_SAMPLE_BIT					32
#define ADC_SAMPLE_RATE						256UL
typedef INT16    ADC_DATA_TYPE, * PADC_DATA_TYPE;
#endif

#ifdef SOUNDCARD
#define DATA_BYTE_TO_POSITION_MOVEBIT		1
#define ADC_DATA_SAMPLE_BIT					16
#define ADC_SAMPLE_RATE						11025UL
typedef INT16    ADC_DATA_TYPE, *PADC_DATA_TYPE;
#endif

#ifdef SDR
#define DATA_BYTE_TO_POSITION_MOVEBIT		1
#define ADC_DATA_SAMPLE_BIT					16
#define ADC_SAMPLE_RATE						2000000UL
typedef short								ADC_DATA_TYPE,	* PADC_DATA_TYPE;
typedef	float								FILTTED_DATA_TYPE, * PFILTEDDATATYPE;
#endif

#ifdef USART
#define DATA_BYTE_TO_POSITION_MOVEBIT		1   
#define ADC_DATA_SAMPLE_BIT					13
#define ADC_SAMPLE_RATE						5000UL
typedef INT16    ADC_DATA_TYPE, * PADC_DATA_TYPE;
#endif

#ifdef USART_INMP441
#define DATA_BYTE_TO_POSITION_MOVEBIT		2   
#define ADC_DATA_SAMPLE_BIT					32
#define ADC_SAMPLE_RATE						5000UL
typedef INT32    ADC_DATA_TYPE, * PADC_DATA_TYPE;
#endif

class CData
{
public:
	void* Buff = NULL;
	const UCHAR* Flag = NULL;

	HPEN hPen = NULL;
	bool bShow = true;

	UINT Len = 0;
	UINT Mask = 0;

	UINT Pos = 0;
	UINT CharPos = 0;
	UINT ProcessPos = 0;
	UINT SavedPos = 0;

	BUFF_DATA_TYPE DataType;
	int SizeOfType = 0;
	int MoveBit = 0;
	int DataBits = 0;

	UINT SampleRate = 0;

	bool GetNew = false;
	UINT NumPerSec = 0;

public:
	CData();
	CData(UINT len, BUFF_DATA_TYPE dataType, INT dataBits, const UCHAR* flag, HPEN hPen);
	~CData();

	void Init(UINT len, BUFF_DATA_TYPE dataType, INT dataBits, const UCHAR* flag, HPEN hPen);
	void UnInit(void);

	void GeneratorWave(void);
	static TIMERPROC NumPerSecTimer_Func(void);

};

extern CData* AdcDataI;
extern CData* AdcDataIFiltted;
//extern CData* AdcDataQ;
//extern CData* AdcDataQFiltted;

extern CData* AudioData;
extern CData* AudioDataFiltted;

extern 	char AdcBuffMarks[DATA_BUFFER_LENGTH];
