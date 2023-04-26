#pragma once

#include "stdafx.h"
#include "locale.h"
#include "stdint.h"

#ifdef _DEBUG

#define OPENCONSOLE		AllocConsole();\
						_tfreopen(_T("CONOUT$"), _T("w+t"), stdout);\
						_tfreopen(_T("CONIN$"), _T("r+t"), stdin);\
						_tsetlocale(LC_ALL, _T("chs"));

#define CLOSECONSOLE	FreeConsole();

#else // _DEBUG
#define OPENCONSOLE	
#define CLOSECONSOLE


#endif // _DEBUG


#define FORWARD_CMD_LENGTH				16

#define FORWARD_CMD_TYPE_FILTER_SETTING				0
#define FORWARD_CMD_TYPE_FILTER_RECORE				1
#define FORWARD_CMD_TYPE_FILTER_DESC_SETTING		2
#define FORWARD_CMD_TYPE_FILTER_DESC_RECORE			3

#define FORWARD_CMD_TYPE_FFT_SET					10
#define FORWARD_CMD_TYPE_FFT_GET_SIGNAL				11

#define FORWARD_CMD_TYPE_FFT_SIGNAL_SAMPLE_RATE		20

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define UP_TO_ZERO(x) (x = x < 0 ? 0 : x)

typedef enum BUFF_DATA_TYPE_ENUM{
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
}BUFF_DATA_TYPE;

typedef void(*GetStrFunction)(char*);

extern HINSTANCE	hInst;

extern BOOLEAN Program_In_Process;
extern BOOLEAN isGetDataExited;

extern  CHAR IniFilePath[];

extern HANDLE  cuda_FFT_hMutexBuff;

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