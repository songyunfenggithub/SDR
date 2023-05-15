
#include "stdafx.h"
#include <iostream>

#pragma comment(lib,"setupapi.lib")  

#include "public.h"
#include "Debug.h"
#include "CData.h"
#include "CDataFromUSB.h"

using namespace DEVICES;

CDataFromUSB clsGetDataUSB;

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)

#define NOISY(_x_) DbgMsg _x_ ;


CDataFromUSB::CDataFromUSB()
{

}

CDataFromUSB::~CDataFromUSB()
{

}

HANDLE CDataFromUSB::OpenOneDevice(_In_  HDEVINFO HardwareDeviceInfo,_In_ PSP_DEVICE_INTERFACE_DATA DeviceInfoData, _In_ PSTR devName)
/*++
Routine Description:

	Given the HardwareDeviceInfo, representing a handle to the plug and
	play information, and deviceInfoData, representing a specific usb device,
	open that device and fill in all the relevant information in the given
	USB_DEVICE_DESCRIPTOR structure.

Arguments:

	HardwareDeviceInfo:  handle to info obtained from Pnp mgr via SetupDiGetClassDevs()
	DeviceInfoData:      ptr to info obtained via SetupDiEnumDeviceInterfaces()

Return Value:

	return HANDLE if the open and initialization was successfull,
		else INVLAID_HANDLE_VALUE.

--*/
{
	PSP_DEVICE_INTERFACE_DETAIL_DATA     functionClassDeviceData = NULL;
	ULONG                                predictedLength = 0;
	ULONG                                requiredLength = 0;
	HANDLE                               hOut = INVALID_HANDLE_VALUE;

	//
	// allocate a function class device data structure to receive the
	// goods about this particular device.
	//
	SetupDiGetDeviceInterfaceDetail(
		HardwareDeviceInfo,
		DeviceInfoData,
		NULL, // probing so no output buffer yet
		0, // probing so output buffer length of zero
		&requiredLength,
		NULL); // not interested in the specific dev-node


	predictedLength = requiredLength;
	// sizeof (SP_FNCLASS_DEVICE_DATA) + 512;

	functionClassDeviceData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(predictedLength);
	if (NULL == functionClassDeviceData) {
		return INVALID_HANDLE_VALUE;
	}
	functionClassDeviceData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	//
	// Retrieve the information from Plug and Play.
	//
	if (!SetupDiGetDeviceInterfaceDetail(
		HardwareDeviceInfo,
		DeviceInfoData,
		functionClassDeviceData,
		predictedLength,
		&requiredLength,
		NULL)) {
		free(functionClassDeviceData);
		return INVALID_HANDLE_VALUE;
	}

	(void)StringCchCopy(devName, MAX_LENGTH, functionClassDeviceData->DevicePath);
	DbgMsg("Attempting to open %s\n", devName);

	hOut = CreateFile(
		functionClassDeviceData->DevicePath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		0, // No special attributes
		NULL); // No template file

	if (INVALID_HANDLE_VALUE == hOut) {
		DbgMsg("FAILED to open %s\n", devName);
	}
	free(functionClassDeviceData);
	return hOut;
}


HANDLE CDataFromUSB::OpenUsbDevice(_In_ LPGUID  pGuid, _In_ PSTR outNameBuf)
/*++
Routine Description:

   Do the required PnP things in order to find
   the next available proper device in the system at this time.

Arguments:

	pGuid:      ptr to GUID registered by the driver itself
	outNameBuf: the generated name for this device

Return Value:

	return HANDLE if the open and initialization was successful,
		else INVLAID_HANDLE_VALUE.
--*/
{
	ULONG NumberDevices;
	HANDLE hOut = INVALID_HANDLE_VALUE;
	HDEVINFO                 hardwareDeviceInfo;
	SP_DEVICE_INTERFACE_DATA deviceInfoData;
	ULONG                    i;
	BOOLEAN                  done;
	PUSB_DEVICE_DESCRIPTOR   usbDeviceInst;
	PUSB_DEVICE_DESCRIPTOR* UsbDevices = &usbDeviceInst;
	PUSB_DEVICE_DESCRIPTOR   tempDevDesc;

	*UsbDevices = NULL;
	tempDevDesc = NULL;
	NumberDevices = 0;

	//
	// Open a handle to the plug and play dev node.
	// SetupDiGetClassDevs() returns a device information set that contains
	// info on all installed devices of a specified class.
	//
	hardwareDeviceInfo =
		SetupDiGetClassDevs(pGuid,
			NULL, // Define no enumerator (global)
			NULL, // Define no
			(DIGCF_PRESENT |           // Only Devices present
				DIGCF_DEVICEINTERFACE)); // Function class devices.

	if (hardwareDeviceInfo == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}
	//
	// Take a wild guess at the number of devices we have;
	// Be prepared to realloc and retry if there are more than we guessed
	//
	NumberDevices = 4;
	done = FALSE;
	deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	i = 0;
	while (!done) {
		NumberDevices *= 2;

		if (*UsbDevices) {
			tempDevDesc = (PUSB_DEVICE_DESCRIPTOR)
				realloc(*UsbDevices, (NumberDevices * sizeof(USB_DEVICE_DESCRIPTOR)));
			if (tempDevDesc) {
				*UsbDevices = tempDevDesc;
				tempDevDesc = NULL;
			}
			else {
				free(*UsbDevices);
				*UsbDevices = NULL;
			}
		}
		else {
			*UsbDevices = (PUSB_DEVICE_DESCRIPTOR)calloc(NumberDevices, sizeof(USB_DEVICE_DESCRIPTOR));
		}

		if (NULL == *UsbDevices) {

			// SetupDiDestroyDeviceInfoList destroys a device information set
			// and frees all associated memory.

			SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
			return INVALID_HANDLE_VALUE;
		}

		for (; i < NumberDevices; i++) {
			// SetupDiEnumDeviceInterfaces() returns information about device
			// interfaces exposed by one or more devices. Each call returns
			// information about one interface; the routine can be called
			// repeatedly to get information about several interfaces exposed
			// by one or more devices.

			if (SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
				0, // We don't care about specific PDOs
				pGuid,
				i,
				&deviceInfoData)) {

				hOut = OpenOneDevice(hardwareDeviceInfo, &deviceInfoData, outNameBuf);
				if (hOut != INVALID_HANDLE_VALUE) {
					done = TRUE;
					break;
				}
			}
			else {
				if (ERROR_NO_MORE_ITEMS == GetLastError()) {
					done = TRUE;
					break;
				}
			}
		}
	}

	// SetupDiDestroyDeviceInfoList() destroys a device information set
	// and frees all associated memory.

	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
	free(*UsbDevices);
	return hOut;
}

BOOL CDataFromUSB::GetUsbDeviceFileName(_In_ LPGUID pGuid,	_In_ PSTR outNameBuf)
/*++
Routine Description:

	Given a ptr to a driver-registered GUID, give us a string with the device name
	that can be used in a CreateFile() call.
	Actually briefly opens and closes the device and sets outBuf if successfull;
	returns FALSE if not

Arguments:

	pGuid:      ptr to GUID registered by the driver itself
	outNameBuf: the generated zero-terminated name for this device

Return Value:

	TRUE on success else FALSE

--*/
{
	HANDLE hDev = OpenUsbDevice(pGuid, outNameBuf);

	if (hDev != INVALID_HANDLE_VALUE) {
		CloseHandle(hDev);
		return TRUE;
	}
	return FALSE;
}

HANDLE CDataFromUSB::open_dev()
/*++
Routine Description:

	Called by dumpUsbConfig() to open an instance of our device

Arguments:

	None

Return Value:

	Device handle on success else NULL

--*/
{
	HANDLE hDEV = OpenUsbDevice((LPGUID)&GUID_CLASS_USBSAMP_USB,
		completeDeviceName);

	if (hDEV == INVALID_HANDLE_VALUE) {
		DbgMsg("Failed to open (%s) = %u", completeDeviceName, GetLastError());
	}
	else {
		DbgMsg("DeviceName = (%s)\n", completeDeviceName);
	}

	return hDEV;

}


HANDLE CDataFromUSB::open_file(_In_ PSTR filename)
/*++
Routine Description:

	Called by main() to open an instance of our device after obtaining its name

Arguments:

	None

Return Value:

	Device handle on success else NULL

--*/
{
	int success = 1;
	HANDLE h;

	if (!GetUsbDeviceFileName(
		(LPGUID)&GUID_CLASS_USBSAMP_USB,
		completeDeviceName))
	{
		NOISY(("Failed to GetUsbDeviceFileName err - %u\n", GetLastError()));
		return  INVALID_HANDLE_VALUE;
	}

	(void)StringCchCat(completeDeviceName, MAX_LENGTH, "\\");

	if (FAILED(StringCchCat(completeDeviceName, MAX_LENGTH, filename))) {
		NOISY(("Failed to open handle - possibly long filename\n"));
		return INVALID_HANDLE_VALUE;
	}

	DbgMsg("completeDeviceName = (%s)\n", completeDeviceName);

	h = CreateFile(completeDeviceName,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (h == INVALID_HANDLE_VALUE) {
		NOISY(("Failed to open (%s) = %u", completeDeviceName, GetLastError()));
		success = 0;
	}
	else {
		NOISY(("Opened successfully.\n"));
	}

	return h;
}

void CDataFromUSB::usage()
/*++
Routine Description:

	Called by main() to dump usage info to the console when
	the app is called with no parms or with an invalid parm

Arguments:

	None

Return Value:

	None

--*/
{
	static int i = 1;

	if (i) {
		DbgMsg("Usage for Read/Write test:\n");
		DbgMsg("-r [n] where n is number of bytes to read\n");
		DbgMsg("-w [n] where n is number of bytes to write\n");
		DbgMsg("-c [n] where n is number of iterations (default = 1)\n");
		DbgMsg("-i [s] where s is the input pipe\n");
		DbgMsg("-o [s] where s is the output pipe\n");
		DbgMsg("-v verbose -- dumps read data\n");
		DbgMsg("-x to skip validation of read and write data\n");

		DbgMsg("\nUsage for USB and Endpoint info:\n");
		DbgMsg("-u to dump USB configuration and pipe info \n");
		i = 0;
	}
}


void CDataFromUSB::parse(_In_ int argc,	_In_reads_(argc) char* argv[])
/*++
Routine Description:

	Called by main() to parse command line parms

Arguments:

	argc and argv that was passed to main()

Return Value:

	Sets global flags as per user function request

--*/
{
	int i;

	if (argc < 2) // give usage if invoked with no parms
		usage();

	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-' ||
			argv[i][0] == '/') {
			switch (argv[i][1]) {
			case 'r':
			case 'R':
				if (i + 1 >= argc) {
					usage();
					exit(1);
				}
				else {
#pragma warning(suppress: 6385)
					ReadLen = atoi(&argv[i + 1][0]);
					if (ReadLen == 0) {
						usage();
						exit(1);
					}
					fRead = TRUE;
				}
				i++;
				break;
			case 'w':
			case 'W':
				if (i + 1 >= argc) {
					usage();
					exit(1);
				}
				else {
					WriteLen = atoi(&argv[i + 1][0]);
					if (WriteLen == 0) {
						usage();
						exit(1);
					}
					fWrite = TRUE;
				}
				i++;
				break;
			case 'c':
			case 'C':
				if (i + 1 >= argc) {
					usage();
					exit(1);
				}
				else {
					IterationCount = atoi(&argv[i + 1][0]);
					if (IterationCount == 0) {
						usage();
						exit(1);
					}
				}
				i++;
				break;
			case 'i':
			case 'I':
				if (i + 1 >= argc) {
					usage();
					exit(1);
				}
				else {
					(void)StringCchCopy(inPipe, MAX_LENGTH, &argv[i + 1][0]);
				}
				i++;
				break;
			case 'u':
			case 'U':
				fDumpUsbConfig = TRUE;
				i++;
				break;
			case 'v':
			case 'V':
				fDumpReadData = TRUE;
				i++;
				break;
			case 'x':
			case 'X':
				fCompareData = FALSE;
				i++;
				break;
			case 'o':
			case 'O':
				if (i + 1 >= argc) {
					usage();
					exit(1);
				}
				else {
					(void)StringCchCopy(outPipe, MAX_LENGTH, &argv[i + 1][0]);
				}
				i++;
				break;
			default:
				usage();
			}
		}
	}
}

BOOL CDataFromUSB::compare_buffs(_In_reads_bytes_(length) char* buff1,_In_reads_bytes_(length) char* buff2,	_In_ int length)
/*++
Routine Description:

	Called to verify read and write buffers match for loopback test

Arguments:

	buffers to compare and length

Return Value:

	TRUE if buffers match, else FALSE

--*/
{
	int ok = 1;

	if (memcmp(buff1, buff2, length)) {
		// Edi, and Esi point to the mismatching char and ecx indicates the
		// remaining length.
		ok = 0;
	}

	return ok;
}

#define NPERLN 8

void CDataFromUSB::dump(UCHAR* b, int len)
/*++
Routine Description:

	Called to do formatted ascii dump to console of the io buffer

Arguments:

	buffer and length

Return Value:

	none

--*/
{
	ULONG i;
	ULONG longLen = (ULONG)len / sizeof(ULONG);
	PULONG pBuf = (PULONG)b;

	// dump an ordinal ULONG for each sizeof(ULONG)'th byte
	DbgMsg("\n****** BEGIN DUMP LEN decimal %d, 0x%x\n", len, len);
	for (i = 0; i < longLen; i++) {
		DbgMsg("%04X ", *pBuf++);
		if (i % NPERLN == (NPERLN - 1)) {
			DbgMsg("\n");
		}
	}
	if (i % NPERLN != 0) {
		DbgMsg("\n");
	}
	DbgMsg("\n****** END DUMP LEN decimal %d, 0x%x\n", len, len);
}


// Begin, routines for USB configuration dump (Cmdline "rwbulk -u" )


char* CDataFromUSB::usbDescriptorTypeString(UCHAR bDescriptorType)
/*++
Routine Description:

	Called to get ascii string of USB descriptor

Arguments:

		PUSB_ENDPOINT_DESCRIPTOR->bDescriptorType or
		PUSB_DEVICE_DESCRIPTOR->bDescriptorType or
		PUSB_INTERFACE_DESCRIPTOR->bDescriptorType or
		PUSB_STRING_DESCRIPTOR->bDescriptorType or
		PUSB_POWER_DESCRIPTOR->bDescriptorType or
		PUSB_CONFIGURATION_DESCRIPTOR->bDescriptorType

Return Value:

	ptr to string

--*/ {

	switch (bDescriptorType) {

	case USB_DEVICE_DESCRIPTOR_TYPE:
		return "USB_DEVICE_DESCRIPTOR_TYPE";

	case USB_CONFIGURATION_DESCRIPTOR_TYPE:
		return "USB_CONFIGURATION_DESCRIPTOR_TYPE";


	case USB_STRING_DESCRIPTOR_TYPE:
		return "USB_STRING_DESCRIPTOR_TYPE";


	case USB_INTERFACE_DESCRIPTOR_TYPE:
		return "USB_INTERFACE_DESCRIPTOR_TYPE";


	case USB_ENDPOINT_DESCRIPTOR_TYPE:
		return "USB_ENDPOINT_DESCRIPTOR_TYPE";

	case USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR_TYPE:
		return "USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR_TYPE";


#ifdef USB_POWER_DESCRIPTOR_TYPE // this is the older definintion which is actually obsolete
		// workaround for temporary bug in 98ddk, older USB100.h file
	case USB_POWER_DESCRIPTOR_TYPE:
		return "USB_POWER_DESCRIPTOR_TYPE";
#endif

#ifdef USB_RESERVED_DESCRIPTOR_TYPE  // this is the current version of USB100.h as in NT5DDK

	case USB_RESERVED_DESCRIPTOR_TYPE:
		return "USB_RESERVED_DESCRIPTOR_TYPE";

	case USB_CONFIG_POWER_DESCRIPTOR_TYPE:
		return "USB_CONFIG_POWER_DESCRIPTOR_TYPE";

	case USB_INTERFACE_POWER_DESCRIPTOR_TYPE:
		return "USB_INTERFACE_POWER_DESCRIPTOR_TYPE";
#endif // for current nt5ddk version of USB100.h

	default:
		return "??? UNKNOWN!!";
	}
}


char* CDataFromUSB::usbEndPointTypeString(UCHAR bmAttributes)
/*++
Routine Description:

	Called to get ascii string of endpt descriptor type

Arguments:

		PUSB_ENDPOINT_DESCRIPTOR->bmAttributes

Return Value:

	ptr to string

--*/
{
	UINT typ = bmAttributes & USB_ENDPOINT_TYPE_MASK;


	switch (typ) {
	case USB_ENDPOINT_TYPE_INTERRUPT:
		return "USB_ENDPOINT_TYPE_INTERRUPT";

	case USB_ENDPOINT_TYPE_BULK:
		return "USB_ENDPOINT_TYPE_BULK";

	case USB_ENDPOINT_TYPE_ISOCHRONOUS:
		return "USB_ENDPOINT_TYPE_ISOCHRONOUS";

	case USB_ENDPOINT_TYPE_CONTROL:
		return "USB_ENDPOINT_TYPE_CONTROL";

	default:
		return "??? UNKNOWN!!";
	}
}


char* CDataFromUSB::usbConfigAttributesString(UCHAR bmAttributes)
/*++
Routine Description:

	Called to get ascii string of USB_CONFIGURATION_DESCRIPTOR attributes

Arguments:

		PUSB_CONFIGURATION_DESCRIPTOR->bmAttributes

Return Value:

	ptr to string

--*/
{
	UINT typ = bmAttributes & USB_CONFIG_POWERED_MASK;


	switch (typ) {

	case USB_CONFIG_BUS_POWERED:
		return "USB_CONFIG_BUS_POWERED";

	case USB_CONFIG_SELF_POWERED:
		return "USB_CONFIG_SELF_POWERED";

	case USB_CONFIG_REMOTE_WAKEUP:
		return "USB_CONFIG_REMOTE_WAKEUP";


	default:
		return "??? UNKNOWN!!";
	}
}


void CDataFromUSB::print_USB_CONFIGURATION_DESCRIPTOR(PUSB_CONFIGURATION_DESCRIPTOR cd)
/*++
Routine Description:

	Called to do formatted ascii dump to console of a USB config descriptor

Arguments:

	ptr to USB configuration descriptor

Return Value:

	none

--*/
{
	DbgMsg("\n===================\nUSB_CONFIGURATION_DESCRIPTOR\n");

	DbgMsg(
		"bLength = 0x%x, decimal %u\n", cd->bLength, cd->bLength
	);

	DbgMsg(
		"bDescriptorType = 0x%x ( %s )\n", cd->bDescriptorType, usbDescriptorTypeString(cd->bDescriptorType)
	);

	DbgMsg(
		"wTotalLength = 0x%x, decimal %u\n", cd->wTotalLength, cd->wTotalLength
	);

	DbgMsg(
		"bNumInterfaces = 0x%x, decimal %u\n", cd->bNumInterfaces, cd->bNumInterfaces
	);

	DbgMsg(
		"bConfigurationValue = 0x%x, decimal %u\n", cd->bConfigurationValue, cd->bConfigurationValue
	);

	DbgMsg(
		"iConfiguration = 0x%x, decimal %u\n", cd->iConfiguration, cd->iConfiguration
	);

	DbgMsg(
		"bmAttributes = 0x%x ( %s )\n", cd->bmAttributes, usbConfigAttributesString(cd->bmAttributes)
	);

	DbgMsg(
		"MaxPower = 0x%x, decimal %u\n", cd->MaxPower, cd->MaxPower
	);
}


void CDataFromUSB::print_USB_INTERFACE_DESCRIPTOR(PUSB_INTERFACE_DESCRIPTOR id, UINT ix)
/*++
Routine Description:

	Called to do formatted ascii dump to console of a USB interface descriptor

Arguments:

	ptr to USB interface descriptor

Return Value:

	none

--*/
{
	DbgMsg("\n-----------------------------\nUSB_INTERFACE_DESCRIPTOR #%u\n", ix);


	DbgMsg(
		"bLength = 0x%x\n", id->bLength
	);


	DbgMsg(
		"bDescriptorType = 0x%x ( %s )\n", id->bDescriptorType, usbDescriptorTypeString(id->bDescriptorType)
	);


	DbgMsg(
		"bInterfaceNumber = 0x%x\n", id->bInterfaceNumber
	);
	DbgMsg(
		"bAlternateSetting = 0x%x\n", id->bAlternateSetting
	);
	DbgMsg(
		"bNumEndpoints = 0x%x\n", id->bNumEndpoints
	);
	DbgMsg(
		"bInterfaceClass = 0x%x\n", id->bInterfaceClass
	);
	DbgMsg(
		"bInterfaceSubClass = 0x%x\n", id->bInterfaceSubClass
	);
	DbgMsg(
		"bInterfaceProtocol = 0x%x\n", id->bInterfaceProtocol
	);
	DbgMsg(
		"bInterface = 0x%x\n", id->iInterface
	);
}


void CDataFromUSB::print_USB_ENDPOINT_DESCRIPTOR(PUSB_ENDPOINT_DESCRIPTOR ed, int i)
/*++
Routine Description:

	Called to do formatted ascii dump to console of a USB endpoint descriptor

Arguments:

	ptr to USB endpoint descriptor,
		index of this endpt in interface desc

Return Value:

	none

--*/
{
	DbgMsg(
		"------------------------------\nUSB_ENDPOINT_DESCRIPTOR for Pipe%02d\n", i
	);

	DbgMsg(
		"bLength = 0x%x\n", ed->bLength
	);

	DbgMsg(
		"bDescriptorType = 0x%x ( %s )\n", ed->bDescriptorType, usbDescriptorTypeString(ed->bDescriptorType)
	);

	if (USB_ENDPOINT_DIRECTION_IN(ed->bEndpointAddress)) {
		DbgMsg(
			"bEndpointAddress= 0x%x ( INPUT )\n", ed->bEndpointAddress
		);
	}
	else {
		DbgMsg(
			"bEndpointAddress= 0x%x ( OUTPUT )\n", ed->bEndpointAddress
		);
	}

	DbgMsg(
		"bmAttributes= 0x%x ( %s )\n", ed->bmAttributes, usbEndPointTypeString(ed->bmAttributes)
	);

	DbgMsg(
		"wMaxPacketSize= 0x%x, decimal %u\n", ed->wMaxPacketSize, ed->wMaxPacketSize
	);

	DbgMsg(
		"bInterval = 0x%x, decimal %u\n", ed->bInterval, ed->bInterval
	);
}


void CDataFromUSB::print_USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR(PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR secd, int i)
/*++
Routine Description:

	Called to do formatted ascii dump to console of a USB super speed endpoint companion descriptor

Arguments:

	secd - ptr to USB endpoint descriptor,
	i    - index of this endpt in interface desc

Return Value:

	none

--*/
{
	DbgMsg(
		"------------------------------\nUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR for Pipe%02d\n", i
	);

	DbgMsg(
		"bLength = 0x%x\n", secd->bLength
	);

	DbgMsg(
		"bDescriptorType = 0x%x ( %s )\n", secd->bDescriptorType, usbDescriptorTypeString(secd->bDescriptorType)
	);

	DbgMsg(
		"bMaxBurst = 0x%x, decimal %u\n", secd->bMaxBurst, secd->bMaxBurst
	);

	DbgMsg(
		"bmAttributes = 0x%x \n", secd->bmAttributes.AsUchar
	);

	DbgMsg(
		"wBytesPerInterval = 0x%x, decimal %u\n", secd->wBytesPerInterval, secd->wBytesPerInterval
	);

}

void CDataFromUSB::rw_dev(HANDLE hDEV)
/*++
Routine Description:

	Called to do formatted ascii dump to console of  USB
	configuration, interface, and endpoint descriptors
	(Cmdline "rwbulk -u" )

Arguments:

	handle to device

Return Value:

	none

--*/
{
	UINT success;
	int siz, nBytes;
	UCHAR buf[256] = { 0 };
	PUSB_COMMON_DESCRIPTOR    commonDesc = NULL;
	PUSB_CONFIGURATION_DESCRIPTOR cd;
	BOOL  displayUnknown;

	siz = sizeof(buf);

	if (hDEV == INVALID_HANDLE_VALUE) {
		NOISY(("DEV not open"));
		return;
	}

	success = DeviceIoControl(hDEV,
		IOCTL_USBSAMP_GET_CONFIG_DESCRIPTOR,
		buf,
		siz,
		buf,
		siz,
		(PULONG)&nBytes,
		NULL);

	NOISY(("request complete, success = %u nBytes = %d\n", success, nBytes));

	if (success) {

		UINT  j = 0, k = 0, n;
		PUCHAR pch;
		PUCHAR descEnd;

		pch = buf;
		n = 0;

		cd = (PUSB_CONFIGURATION_DESCRIPTOR)pch;

		descEnd = (PUCHAR)cd + cd->wTotalLength;

		commonDesc = (PUSB_COMMON_DESCRIPTOR)cd;

		while ((PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
			(PUCHAR)commonDesc + commonDesc->bLength <= descEnd)
		{
			displayUnknown = FALSE;

			switch (commonDesc->bDescriptorType)
			{
			case USB_CONFIGURATION_DESCRIPTOR_TYPE:
				if (commonDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR))
				{
					NOISY(("Configuration Descriptor's bLength filed does not match its size\n"));
					displayUnknown = TRUE;
					break;
				}
				n = 0;
				print_USB_CONFIGURATION_DESCRIPTOR((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc);
				break;

			case USB_INTERFACE_DESCRIPTOR_TYPE:
				if (commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR))
				{
					NOISY(("Interface Descriptor's bLength filed does not match its size\n"));
					displayUnknown = TRUE;
					break;
				}
				j = 0;
				k = 0;
				print_USB_INTERFACE_DESCRIPTOR((PUSB_INTERFACE_DESCRIPTOR)commonDesc, n++);
				break;

			case USB_ENDPOINT_DESCRIPTOR_TYPE:
				if (commonDesc->bLength != sizeof(USB_ENDPOINT_DESCRIPTOR))
				{
					NOISY(("Endpoint Descriptor's bLength filed does not match its size\n"));
					displayUnknown = TRUE;
					break;
				}
				print_USB_ENDPOINT_DESCRIPTOR((PUSB_ENDPOINT_DESCRIPTOR)commonDesc, j++);
				break;

			case USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR_TYPE:
				if (commonDesc->bLength < sizeof(USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR))
				{
					NOISY(("SuperSpeed Endpoint Companion Descriptor's bLength field does not match its size\n"));
					displayUnknown = TRUE;
					break;
				}
				print_USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR((PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR)commonDesc, k++);
				break;

			default:
				displayUnknown = TRUE;
				break;
			}

			if (displayUnknown)
			{
				NOISY(("Test application finds a unknown descriptor.\n"));
			}

			commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);

		}

	}

	return;
}


int  CDataFromUSB::dumpUsbConfig()
/*++
Routine Description:

	Called to do formatted ascii dump to console of  USB
	configuration, interface, and endpoint descriptors
	(Cmdline "rwbulk -u" )

Arguments:

	none

Return Value:

	none

--*/
{
	HANDLE hDEV = open_dev();

	if (hDEV != INVALID_HANDLE_VALUE)
	{
		rw_dev(hDEV);
		CloseHandle(hDEV);
	}

	return 0;

}
//  End, routines for USB configuration and pipe info dump  (Cmdline "rwbulk -u" )


LPTHREAD_START_ROUTINE CDataFromUSB::GetDataUSBThreadFun(LPVOID lp)
{
	OPENCONSOLE_SAVED;
	clsGetDataUSB.USBGetData();
	//CLOSECONSOLE;
	return 0;
}

void CDataFromUSB::USBGetData(void)
{
	char* pinBuf = NULL;
	char* poutBuf = NULL;
	int    nBytesRead;
	int    nBytesWrite;
	ULONG  i = 0;
	UINT   success;
	HANDLE hRead = INVALID_HANDLE_VALUE;
	HANDLE hWrite = INVALID_HANDLE_VALUE;
	ULONG  fail = 0L;

	dumpUsbConfig();

	hRead = open_file(inPipe);
	pinBuf = (char*)malloc(ReadLen);

	hWrite = open_file(outPipe);
	poutBuf = (char*)malloc(WriteLen);

	//
	// send the write
	//
	WriteFile(hWrite, poutBuf, WriteLen, (PULONG)&nBytesWrite, NULL);
	DbgMsg("<%s> W (%04.4u) : request %06.6d bytes -- %06.6d bytes written\n",
					outPipe, i, WriteLen, nBytesWrite);

	int ecount = 0;
	while (Program_In_Process)
	{

		success = ReadFile(hRead, (char*)(AdcDataI->Buff) + AdcDataI->CharPos, 64, (PULONG)&nBytesRead, NULL);

		if (success)
		{
			if (nBytesRead != 64)
			{
				DbgMsg("<%s> R (%04.4u) : request %06.6d bytes -- %06.6d bytes read\n",
					inPipe, i++, ReadLen, nBytesRead);

				DbgMsg("Dumping read buffer\n");
				dump((PUCHAR)pinBuf, nBytesRead);
				pinBuf[nBytesRead] = 0;
				DbgMsg("%s\n", pinBuf);
				EXIT(0);
			}
		}
		else
		{
			ecount++;
			if(ecount % 2560000 == 0) DbgMsg("usb read data error! %d\n", GetLastError());
		}
		AdcDataI->CharPos += nBytesRead;
		if (AdcDataI->CharPos == (AdcDataI->Len << AdcDataI->MoveBit)) AdcDataI->CharPos = 0;
		AdcDataI->Pos = AdcDataI->CharPos >> 2;
		AdcDataI->GetNew = true;
	}

	// close devices if needed
	if (hRead != INVALID_HANDLE_VALUE)CloseHandle(hRead);
	if (hWrite != INVALID_HANDLE_VALUE)CloseHandle(hWrite);

	isGetDataExited = true;
}
