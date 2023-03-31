#pragma once

/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

Module Name:

	BulkUsr.h

Abstract:

Environment:

	User & Kernel mode

--*/

#ifndef _USER_H
#define _USER_H

#include <initguid.h>

// {6068EB61-98E7-4c98-9E20-1F068295909A}
DEFINE_GUID(GUID_CLASS_USBSAMP_USB,
	0x873fdf, 0x61a8, 0x11d1, 0xaa, 0x5e, 0x0, 0xc0, 0x4f, 0xb1, 0x72, 0x8b);

#define IOCTL_INDEX             0x0000


#define IOCTL_USBSAMP_GET_CONFIG_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX,     \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_USBSAMP_RESET_DEVICE          CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX + 1, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_USBSAMP_RESET_PIPE            CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX + 2, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#endif


#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)

#include <windows.h>

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "devioctl.h"
#include "strsafe.h"

#pragma warning(disable:4200)  //
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <setupapi.h>
#include <basetyps.h>
#include "usbdi.h"

#define MAX_LENGTH 256

class CDataFromUSB
{
public:
	
	char inPipe[MAX_LENGTH] = "PIPE01";     // pipe name for bulk input pipe on our test board
	char outPipe[MAX_LENGTH] = "PIPE00";    // pipe name for bulk output pipe on our test board
	char completeDeviceName[MAX_LENGTH] = "";  //generated from the GUID registered by the driver itself

	BOOL fDumpUsbConfig = FALSE;    // flags set in response to console command line switches
	BOOL fDumpReadData = FALSE;
	BOOL fRead = FALSE;
	BOOL fWrite = FALSE;
	BOOL fCompareData = TRUE;

	int gDebugLevel = 1;      // higher == more verbose, default is 1, 0 turns off all

	ULONG IterationCount = 1; //count of iterations of the test we are to perform
	int WriteLen = 0;         // #bytes to write
	int ReadLen = 0;          // #bytes to read

	HANDLE hThread;

	bool	GetDataWorking = true;


public:
	CDataFromUSB();
	~CDataFromUSB();

	HANDLE	OpenOneDevice(_In_  HDEVINFO HardwareDeviceInfo, _In_ PSP_DEVICE_INTERFACE_DATA DeviceInfoData, _In_ PSTR devName);
	HANDLE	OpenUsbDevice(_In_ LPGUID  pGuid, _In_ PSTR outNameBuf);
	BOOL	GetUsbDeviceFileName(_In_ LPGUID pGuid, _In_ PSTR outNameBuf);
	HANDLE	open_file(_In_ PSTR filename);
	HANDLE	open_dev();
	void	usage();
	void	parse(_In_ int argc, _In_reads_(argc) char* argv[]);
	BOOL	compare_buffs(_In_reads_bytes_(length) char* buff1, _In_reads_bytes_(length) char* buff2, _In_ int length);
	void	dump(UCHAR* b, int len);
	char*	usbDescriptorTypeString(UCHAR bDescriptorType);
	char*	usbEndPointTypeString(UCHAR bmAttributes);
	char*	usbConfigAttributesString(UCHAR bmAttributes);
	void	print_USB_CONFIGURATION_DESCRIPTOR(PUSB_CONFIGURATION_DESCRIPTOR cd);
	void	print_USB_INTERFACE_DESCRIPTOR(PUSB_INTERFACE_DESCRIPTOR id, UINT ix);
	void	print_USB_ENDPOINT_DESCRIPTOR(PUSB_ENDPOINT_DESCRIPTOR ed, int i);
	void	print_USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR(PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR secd, int i);
	void	rw_dev(HANDLE hDEV);
	int		dumpUsbConfig();


	void USBGetData(void);
	static LPTHREAD_START_ROUTINE GetDataUSBThreadFun(LPVOID lp);

};

extern CDataFromUSB clsGetDataUSB;

