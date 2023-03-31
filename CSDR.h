#pragma once

#include "stdafx.h"
#include "windows.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>

#include <string>
#include <map>

#include "CWaveData.h"


typedef void (*set_params_func)(int index);

typedef std::map<const char*, int>	SDR_ENUM_MAP;

typedef enum {
	decimationFactor1 = 1,
	decimationFactor2 = 2,
	decimationFactor4 = 4,
	decimationFactor8 = 8,
	decimationFactor16 = 16,
	decimationFactor32 = 32,
	decimationFactor64 = 64
}sdrplay_api_decimationFactorT;

typedef enum {
	SDR_PAMRAS_NONE,
	SDR_PAMRAS_ENUM,
	SDR_PAMRAS_FLOAT,
	SDR_PAMRAS_DOUBLE,
	SDR_PAMRAS_INT,
	SDR_PAMRAS_UINT,
	SDR_PAMRAS_USHORT,
	SDR_PAMRAS_UCHAR
}SDR_PAMRAS_TYPE;

typedef struct tagSDRParams
{
	int level;
	char* txt;
	double data, default, min, max;
	SDR_ENUM_MAP* pEnumMap;
	SDR_PAMRAS_TYPE	valueType;
	void* pValue;
	int paramUpdateReason;
	const char* comment;
	set_params_func p_set_params_func;
}SDRParams;


extern SDRParams SDR_params[];


class CSDR
{
public:
	
	int sel_SDR_params_index = -1;
	TVITEM sel_tvi = { 0 };
	bool SDR_parmas_changed = false;
	bool editInCheck = false;

//	double SdrSampleRate = ADC_SAMPLE_RATE;
//	unsigned char DecimationFactorEnable = 0;
//	unsigned char DecimationFactor = 1;

public:
	CSDR();
	~CSDR();

	void buildTreeItems(HWND hWndTreeView, HTREEITEM hItem, int* i);

	const char* CSDR::map_value_to_key(SDR_ENUM_MAP* pmap, int value);

	void refresh_tree_item(HWND hWndTreeView, TVITEM* ptvi, int i);
	void refresh_input_panel(int index);
	void get_ValueAddr(const char* pdesc, void* p);
	INT  get_ValueIndex(const char* pdesc);
	void Init_ValueAddr(void);
	void Init_Combox(int index);
	void SDR_params_apply(void);
	void edit_check_range(void);
	
	static void set_params_SampleRate(int index);
	static void set_params_decimationFactorEnable(int index);
	static void set_params_decimationFactor(int index);
};

extern CSDR clsSDR;
