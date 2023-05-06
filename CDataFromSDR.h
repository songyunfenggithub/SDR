#pragma once

#include "SDRPlay_API.3.09/API/inc/sdrplay_api.h"

namespace DEVICES {

	class CDataFromSDR
	{
	public:
		sdrplay_api_DeviceT devs[6];

		sdrplay_api_DeviceT* chosenDevice = NULL;
		sdrplay_api_DeviceParamsT* deviceParams = NULL;
		sdrplay_api_RxChannelParamsT* chParams;

		bool SDROpened = false;

		CDataFromSDR();
		~CDataFromSDR();

		static sdrplay_api_CallbackFnsT cbFns;

		static void StreamACallback(short* xi, short* xq, sdrplay_api_StreamCbParamsT* params, unsigned int
			numSamples, unsigned int reset, void* cbContext);
		static void StreamBCallback(short* xi, short* xq, sdrplay_api_StreamCbParamsT* params, unsigned int
			numSamples, unsigned int reset, void* cbContext);
		static void EventCallback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner,
			sdrplay_api_EventParamsT* params, void* cbContext);
		void open_SDR_device(void);
		void close_SDR_device(void);
	};

}

extern DEVICES::CDataFromSDR clsGetDataSDR;
