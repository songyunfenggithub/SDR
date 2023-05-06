
#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include "SDRPlay_API.3.09/API/inc/sdrplay_api.h"


#include "public.h"
#include "CData.h"
#include "CFilter.h"
#include "CWinFFT.h"
#include "CWinSpectrum.h"

#include "CSDR.h"
#include "CDataFromSDR.h"


#pragma comment(lib,"SDRPlay_API.3.09/API/x64/sdrplay_api.lib")

using namespace DEVICES;

CDataFromSDR clsGetDataSDR;

sdrplay_api_CallbackFnsT CDataFromSDR::cbFns;

CDataFromSDR::CDataFromSDR()
{

}

CDataFromSDR::~CDataFromSDR()
{

}

int masterInitialised = 0;
int slaveUninitialised = 0;


void CDataFromSDR::StreamACallback(short* xi, short* xq, sdrplay_api_StreamCbParamsT* params, unsigned int
	numSamples, unsigned int reset, void* cbContext)
{
	if (reset)
		printf("sdrplay_api_StreamACallback: numSamples=%d\n", numSamples);
	// Process stream callback data here

	//printf("firstSampleNum:%d\r\n", params->firstSampleNum);
	/*
	printf("xi:%p, xq:%p, params{firstSampleNum:%d, numSamples:%d, fsChanged:%d, grChanged:%d, rfChanged£º%d}, numSamples:%d, reset:%d, cbContext:%p\r\n",
		xi, xq, 
		params->firstSampleNum, 
		params->numSamples,
		params->fsChanged,
		params->grChanged,
		params->rfChanged,
		numSamples, 
		reset, 
		cbContext
	);
	*/

	int m1 = (AdcData->Len << AdcData->MoveBit) - AdcData->CharPos;
	int m2 = 0;
	if (m1 > numSamples) {
		m1 = numSamples;
		memset(AdcBuffMarks + (AdcData->CharPos >> AdcData->MoveBit), 0, m1 >> AdcData->MoveBit);
		memcpy((char*)AdcData->Buff + AdcData->CharPos, xi, m1);
		AdcData->CharPos += m1;
	}
	else {
		m2 = numSamples - m1;
		memset(AdcBuffMarks + (AdcData->CharPos >> AdcData->MoveBit), 0, m1 >> AdcData->MoveBit);
		memset(AdcBuffMarks, 0, m2 >> AdcData->MoveBit);
		memcpy((char*)AdcData->Buff + AdcData->CharPos, xi, m1);
		memcpy((char*)AdcData->Buff, xi + m1, m2);
		AdcData->CharPos = m2;
	}
	AdcBuffMarks[((AdcData->CharPos - numSamples) >> AdcData->MoveBit) & AdcData->Mask] = 1;

	m1 = (AdcData->Len << AdcData->MoveBit) - AdcData->CharPos;
	m2 = 0;
	if (m1 > numSamples) {
		m1 = numSamples;
		memset(AdcBuffMarks + (AdcData->CharPos >> AdcData->MoveBit), 0, m1 >> AdcData->MoveBit);
		memcpy((char*)AdcData->Buff + AdcData->CharPos, xq, m1);
		AdcData->CharPos += m1;
	}
	else {
		m2 = numSamples - m1;
		memset(AdcBuffMarks + (AdcData->CharPos >> AdcData->MoveBit), 0, m1 >> AdcData->MoveBit);
		memset(AdcBuffMarks, 0, m2 >> AdcData->MoveBit);
		memcpy((char*)AdcData->Buff + AdcData->CharPos, xq, m1);
		memcpy((char*)AdcData->Buff, xq + m1, m2);
		AdcData->CharPos = m2;
	}
	AdcBuffMarks[((AdcData->CharPos - numSamples) >> AdcData->MoveBit) & AdcData->Mask] = 2;

	//StringToHex((char*)(AdcData->Buff) + AdcData->CharPos, 20);
	//printf("%d\n", len);

	if ((AdcData->Len << AdcData->MoveBit) == AdcData->CharPos)AdcData->CharPos = 0;
	//if ((DATA_BUFFER_LENGTH << DATA_BYTE_TO_POSITION_MOVEBIT) == AdcData->CharPos) AdcData->CharPos = 0;

	AdcData->Pos = AdcData->CharPos >> AdcData->MoveBit;
	//printf("pos:%d\n", AdcData->Pos);
	AdcData->GetNew = true;

	return;
}

void CDataFromSDR::StreamBCallback(short* xi, short* xq, sdrplay_api_StreamCbParamsT* params, unsigned int
	numSamples, unsigned int reset, void* cbContext)
{
	if (reset)
		printf("sdrplay_api_StreamBCallback: numSamples=%d\n", numSamples);
	// Process stream callback data here - this callback will only be used in dual tuner mode
	return;
}

void CDataFromSDR::EventCallback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner,
	sdrplay_api_EventParamsT* params, void* cbContext)
{
	switch (eventId)
	{
	case sdrplay_api_GainChange:
		printf("sdrplay_api_EventCb: %s, tuner=%s gRdB=%d lnaGRdB=%d systemGain=%.2f\r\n",
			"sdrplay_api_GainChange", (tuner == sdrplay_api_Tuner_A) ? "sdrplay_api_Tuner_A" :
			"sdrplay_api_Tuner_B", params->gainParams.gRdB, params->gainParams.lnaGRdB,
			params->gainParams.currGain);
		break;
	case sdrplay_api_PowerOverloadChange:
		printf("sdrplay_api_PowerOverloadChange: tuner=%s powerOverloadChangeType=%s\r\n",
			(tuner == sdrplay_api_Tuner_A) ? "sdrplay_api_Tuner_A" : "sdrplay_api_Tuner_B",
			(params->powerOverloadParams.powerOverloadChangeType ==
				sdrplay_api_Overload_Detected) ? "sdrplay_api_Overload_Detected" :
			"sdrplay_api_Overload_Corrected");
		// Send update message to acknowledge power overload message received
		sdrplay_api_Update(clsGetDataSDR.chosenDevice->dev, tuner, sdrplay_api_Update_Ctrl_OverloadMsgAck,
			sdrplay_api_Update_Ext1_None);
		break;
	case sdrplay_api_RspDuoModeChange:
		printf("sdrplay_api_EventCb: %s, tuner=%s modeChangeType=%s\r\n",
			"sdrplay_api_RspDuoModeChange", (tuner == sdrplay_api_Tuner_A) ?
			"sdrplay_api_Tuner_A" : "sdrplay_api_Tuner_B",
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterInitialised) ?
			"sdrplay_api_MasterInitialised" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveAttached) ?
			"sdrplay_api_SlaveAttached" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveDetached) ?
			"sdrplay_api_SlaveDetached" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveInitialised) ?
			"sdrplay_api_SlaveInitialised" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveUninitialised) ?
			"sdrplay_api_SlaveUninitialised" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterDllDisappeared) ?
			"sdrplay_api_MasterDllDisappeared" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveDllDisappeared) ?
			"sdrplay_api_SlaveDllDisappeared" : "unknown type");
		if (params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterInitialised)
			masterInitialised = 1;
		if (params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveUninitialised)
			slaveUninitialised = 1;
		break;
	case sdrplay_api_DeviceRemoved:
		printf("sdrplay_api_EventCb: %s\r\n", "sdrplay_api_DeviceRemoved");
		break;
	default:
		printf("sdrplay_api_EventCb: %d, unknown event\r\n", eventId);
		break;
	}
}

void CDataFromSDR::open_SDR_device(void)
{
	unsigned int ndev;
	int i;
	float ver = 0.0;
	sdrplay_api_ErrT err;

	int reqTuner = 0;
	int master_slave = 0;
	char c;
	unsigned int chosenIdx = 0;

	// Open API
	if ((err = sdrplay_api_Open()) != sdrplay_api_Success)
	{
		printf("sdrplay_api_Open failed %s\n", sdrplay_api_GetErrorString(err));
		system("PAUSE");
		exit(0);
	}
	else
	{
		// Enable debug logging output
		if ((err = sdrplay_api_DebugEnable(NULL, sdrplay_api_DbgLvl_Verbose)) != sdrplay_api_Success)
		{
			printf("sdrplay_api_DebugEnable failed %s\n", sdrplay_api_GetErrorString(err));
		}
		// Check API versions match
		if ((err = sdrplay_api_ApiVersion(&ver)) != sdrplay_api_Success)
		{
			printf("sdrplay_api_ApiVersion failed %s\n", sdrplay_api_GetErrorString(err));
		}
		if (ver != SDRPLAY_API_VERSION)
		{
			printf("API version don't match (local=%.2f dll=%.2f)\n", SDRPLAY_API_VERSION, ver);
			goto CloseApi;
		}
		// Lock API while device selection is performed
		sdrplay_api_LockDeviceApi();
		// Fetch list of available devices
		if ((err = sdrplay_api_GetDevices(devs, &ndev, sizeof(devs) /
			sizeof(sdrplay_api_DeviceT))) != sdrplay_api_Success)
		{
			printf("sdrplay_api_GetDevices failed %s\n", sdrplay_api_GetErrorString(err));
			goto UnlockDeviceAndCloseApi;
		}
		printf("MaxDevs=%d NumDevs=%d\n", sizeof(devs) / sizeof(sdrplay_api_DeviceT), ndev);
		if (ndev > 0)
		{
			for (i = 0; i < (int)ndev; i++)
			{
				if (devs[i].hwVer == SDRPLAY_RSPduo_ID)
					printf("Dev%d: SerNo=%s hwVer=%d tuner=0x%.2x rspDuoMode=0x%.2x\n", i,
						devs[i].SerNo, devs[i].hwVer, devs[i].tuner, devs[i].rspDuoMode);
				else
					printf("Dev%d: SerNo=%s hwVer=%d tuner=0x%.2x\n", i, devs[i].SerNo,
						devs[i].hwVer, devs[i].tuner);
			}
			// Choose device

			// Pick first device of any type
			for (i = 0; i < (int)ndev; i++)
			{
				chosenIdx = i;
				break;
			}

			if (i == ndev)
			{
				printf("Couldn't find a suitable device to open - exiting\n");
				goto UnlockDeviceAndCloseApi;
			}
			printf("chosenDevice = %d\n", chosenIdx);
			chosenDevice = &devs[chosenIdx];
			// Select chosen device
			if ((err = sdrplay_api_SelectDevice(chosenDevice)) != sdrplay_api_Success)
			{
				printf("sdrplay_api_SelectDevice failed %s\n", sdrplay_api_GetErrorString(err));
				goto UnlockDeviceAndCloseApi;
			}
			// Unlock API now that device is selected
			sdrplay_api_UnlockDeviceApi();
			// Retrieve device parameters so they can be changed if wanted
			if ((err = sdrplay_api_GetDeviceParams(chosenDevice->dev, &deviceParams)) !=
				sdrplay_api_Success)
			{
				printf("sdrplay_api_GetDeviceParams failed %s\n",
					sdrplay_api_GetErrorString(err));
				goto CloseApi;
			}
			// Check for NULL pointers before changing settings
			if (deviceParams == NULL)
			{
				printf("sdrplay_api_GetDeviceParams returned NULL deviceParams pointer\n");
				goto CloseApi;
			}
			// Configure dev parameters
			if (deviceParams->devParams != NULL)
			{
				// Change from default Fs to 8MHz
				deviceParams->devParams->fsFreq.fsHz = 8000000;
			}
			// Configure tuner parameters (depends on selected Tuner which parameters to use)
			chParams = (chosenDevice->tuner == sdrplay_api_Tuner_B) ? deviceParams->rxChannelB :
				deviceParams->rxChannelA;
			if (chParams != NULL)
			{
				chParams->tunerParams.rfFreq.rfHz = 105000000.0;
				chParams->tunerParams.bwType = sdrplay_api_BW_8_000;// sdrplay_api_BW_1_536;
				if (master_slave == 0) // Change single tuner mode to ZIF
				{
					chParams->tunerParams.ifType = sdrplay_api_IF_Zero;// sdrplay_api_IF_Zero;
				}
				chParams->tunerParams.gain.gRdB = 40;
				chParams->tunerParams.gain.LNAstate = 0;
				// Disable AGC
				chParams->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
			}
			else
			{
				printf("sdrplay_api_GetDeviceParams returned NULL chParams pointer\n");
				goto CloseApi;
			}
			// Assign callback functions to be passed to sdrplay_api_Init()
			CDataFromSDR::cbFns.StreamACbFn = CDataFromSDR::StreamACallback;
			CDataFromSDR::cbFns.StreamBCbFn = CDataFromSDR::StreamBCallback;
			CDataFromSDR::cbFns.EventCbFn = CDataFromSDR::EventCallback;
			// Now we're ready to start by calling the initialisation function 
			// This will configure the device and start streaming
			if ((err = sdrplay_api_Init(chosenDevice->dev, &CDataFromSDR::cbFns, NULL)) != sdrplay_api_Success)
			{
				printf("sdrplay_api_Init failed %d %s\n", err, sdrplay_api_GetErrorString(err));
				if (err == sdrplay_api_StartPending) // This can happen if we're starting in master / slave mode as a slave and the master is not yet running
				{
					while (1)
					{
						Sleep(1000);
						if (masterInitialised) // Keep polling flag set in event callback until the master is initialised
						{
							// Redo call - should succeed this time
							if ((err = sdrplay_api_Init(chosenDevice->dev, &CDataFromSDR::cbFns, NULL)) !=
								sdrplay_api_Success)
							{
								printf("sdrplay_api_Init failed %s\n",
									sdrplay_api_GetErrorString(err));
							}
							goto CloseApi;
						}
						printf("Waiting for master to initialise\n");
					}
				}
				else
				{
					sdrplay_api_ErrorInfoT* errInfo = sdrplay_api_GetLastError(NULL);
					if (errInfo != NULL)
						printf("Error in %s: %s(): line %d: %s\n", errInfo->file, errInfo->function, errInfo->line, errInfo->message);
					goto CloseApi;
				}
			}
			//open SDR device Success then return
			printf("sdrplay_api_Init sdrplay_api_Success\r\n");
			SDROpened = true;
			clsSDR.Init_ValueAddr();
			return;
		}


	UnlockDeviceAndCloseApi:
		// Unlock API
		sdrplay_api_UnlockDeviceApi();
	CloseApi:
		// Close API
		sdrplay_api_Close();

		printf("sdr device open falild.\r\n");
		system("PAUSE");
		exit(0);
	}
}

void CDataFromSDR::close_SDR_device(void)
{
	sdrplay_api_ErrT err;

	if (SDROpened == true)
	{
		SDROpened = false;
		// Finished with device so uninitialise it
		if ((err = sdrplay_api_Uninit(chosenDevice->dev)) != sdrplay_api_Success)
		{
			printf("sdrplay_api_Uninit failed %s\n", sdrplay_api_GetErrorString(err));
			if (err == sdrplay_api_StopPending)
			{
				// We¡¯re stopping in master/slave mode as a master and the slave is still running
				while (1)
				{
					Sleep(1000);
					if (slaveUninitialised)
					{
						// Keep polling flag set in event callback until the slave is uninitialised
						 // Repeat call - should succeed this time
						if ((err = sdrplay_api_Uninit(chosenDevice->dev)) !=
							sdrplay_api_Success)
						{
							printf("sdrplay_api_Uninit failed %s\n",
								sdrplay_api_GetErrorString(err));
						}
						slaveUninitialised = 0;
						goto CloseApi;
					}
					printf("Waiting for slave to uninitialise\n");
				}
			}
			goto CloseApi;
		}
		// Release device (make it available to other applications)
		sdrplay_api_ReleaseDevice(chosenDevice);

	CloseApi:
		// Close API
		sdrplay_api_Close();
	}
	printf("SDR Device Closed.\r\n");
}