#pragma once

#include "stdafx.h"
#include "locale.h"
#include "stdint.h"

typedef struct FFTINFO_STRUCT {
	UINT FFTSize = 0;
	UINT FFTStep = 0;
	UINT HalfFFTSize = 0;
	UINT AverageDeep = 0;
	UINT AverageDeepNum = 0;
	UINT FFTPos = 0;
	UINT FFTCount = 0;
	UINT SavedFFTCount = 0;
	float FFTPerSec = 0.0;
	bool FFTNew = false;
} FFT_INFO;

typedef enum COLOR_PENS_ENUM {
	Pen_Red = 0,
	Pen_Green = 1,
	Pen_Blue = 2,
	Pen_Yellow = 3
} COLOR_PEN;

extern HPEN Pens[4];

#ifdef _DEBUG

#define OPENCONSOLE_SAVED	
#define OPENCONSOLE
#define OPENCONSOLE1		AllocConsole();\
						_tfreopen(_T("CONOUT$"), _T("w+t"), stdout);\
						_tfreopen(_T("CONIN$"), _T("r+t"), stdin);\
						_tsetlocale(LC_ALL, _T("chs"));

#define CLOSECONSOLE
#define CLOSECONSOLE1	FreeConsole();

#define EXIT(x)	exit(x)

#else // _DEBUG

#define OPENCONSOLE_SAVED	
#define OPENCONSOLE	
#define CLOSECONSOLE


#define EXIT(x)		exit(x)


#endif // _DEBUG


//#define FORWARD_CMD_LENGTH				16
//
//#define FORWARD_CMD_TYPE_FILTER_SETTING				0
//#define FORWARD_CMD_TYPE_FILTER_RECORE				1
//#define FORWARD_CMD_TYPE_FILTER_DESC_SETTING		2
//#define FORWARD_CMD_TYPE_FILTER_DESC_RECORE			3
//
//#define FORWARD_CMD_TYPE_FFT_SET					10
//#define FORWARD_CMD_TYPE_FFT_GET_SIGNAL				11
//
//#define FORWARD_CMD_TYPE_FFT_SIGNAL_SAMPLE_RATE		20

#define FFT_SIZE	0x100000
#define FFT_STEP	0x40000
#define FFT_DEEP		0x10

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define UP_TO_ZERO(x) (x = (x) < 0 ? 0 : (x))

typedef void(*GetStrFunction)(char*);

extern HINSTANCE	hInst;

extern BOOLEAN Program_In_Process;
extern BOOLEAN isGetDataExited;

extern  CHAR IniFilePath[];

extern HANDLE  cuda_FFT_hMutexBuff;

extern FFT_INFO FFTInfo_Signal;
extern FFT_INFO FFTInfo_Filtted;
extern FFT_INFO FFTInfo_Audio;
extern FFT_INFO FFTInfo_AudioFiltted;
extern FFT_INFO FFTInfo_Spectrum_Scan;

void WaitForExit(void);

void* set_WinClass(HWND hWnd, LPARAM lParam);
void* get_WinClass(HWND hWnd);
void* set_DlgWinClass(HWND hDlg, LPARAM lParam);
void* get_DlgWinClass(HWND hDlg);

void MainInit(void);

void StringToHex(char* p, int len);
char* DoubleToFormat(double val, int dotlen, char* p);
char* k_formating(double dData, char* tempstr);
char* formatKDouble(double val, double ref, char* unitTag, char* tempstr);
char* formatKKDouble(double val, char* unitTag, char* tempstr);
char* DoubleToFullString(double d, char* ps);
char* fomatLong(UINT64 n, char* str);
char* fomatKINT64(INT64 v, char* str);
char* fomatKINT64Width(INT64 v, int w, char* str);

UINT64 MoveBits(UINT64 u, int bit);

void charsToHex(char* str, int len);