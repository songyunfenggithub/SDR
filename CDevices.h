
#pragma once

namespace DEVICES {

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
}
extern DEVICES::CDevices clsDevices;
