// Devices.h: interface for the CDevices class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVICES_H__05699515_10A8_46E9_8499_1528DF74C52C__INCLUDED_)
#define AFX_DEVICES_H__05699515_10A8_46E9_8499_1528DF74C52C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDevices  
{
public:
	UINT		guWaveInId;


public:
	CDevices();
	virtual ~CDevices();

	MMRESULT FormatChoose(LPWAVEFORMATEX pwfx);
	MMRESULT ACMAPI acmFormatChoose(LPACMFORMATCHOOSE pafmtc);
	MMRESULT MyWaveInGetDevCaps(UINT uDevId, LPWAVEINCAPS pwic);
	BOOL DisplayWaveInDevCaps(HWND hedit, UINT uDevId, LPWAVEINCAPS pwic, LPWAVEFORMATEX pwfx);
	int MEditPrintF(HWND hedit, PTSTR pszFormat, ...);
	BOOL GetFormatDescription(LPWAVEFORMATEX pwfx, LPTSTR pszFormatTag, LPTSTR pszFormat);
	BOOL GetErrorString(MMRESULT mmr, LPTSTR pszError);
	MMRESULT ACMAPI acmFormatDetails(HACMDRIVER had, LPACMFORMATDETAILS pafd, DWORD fdwDetails);
	MMRESULT ACMAPI acmFormatTagDetails(HACMDRIVER had, LPACMFORMATTAGDETAILS paftd, DWORD fdwDetails);
	BOOL acmThunkInitialize(void);

	static LRESULT CALLBACK WaveDeviceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};

extern CDevices	clsDevices;

#endif // !defined(AFX_DEVICES_H__05699515_10A8_46E9_8499_1528DF74C52C__INCLUDED_)
