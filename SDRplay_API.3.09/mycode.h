#pragma once

#include "../API/inc/sdrplay_api.h"


typedef void (*SetFunc)(sdrplay_api_DeviceT*, sdrplay_api_DeviceParamsT*);
extern SetFunc setfuncs[];

// API function definitions


void printf_DevParams(sdrplay_api_DeviceParamsT* deviceParams);
void printf_ControlParams(sdrplay_api_DeviceParamsT* deviceParams);
void printf_TunerParams(sdrplay_api_DeviceParamsT* deviceParams);

void scanf_DevParams(void);

