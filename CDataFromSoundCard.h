#pragma once

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <commdlg.h>

class CDataFromSoundCard
{
public:

	WAVEFORMATEX	FormatEx;

	typedef struct INDATATAG {
		BOOL		fInOpen;
		HWAVEIN		hWaveIn;
		WAVEHDR		WaveInHdr;
	} INDATA, * PINDATA;

	INDATA InData;

	typedef struct OUTDATATAG {
		BOOL		fOutOpen;
		HWAVEOUT	hWaveOut;
		WAVEHDR		WaveOutHdr;
	} OUTDATA, * POUTDATA;

	OUTDATA	OutData;

public:
	CDataFromSoundCard();
	~CDataFromSoundCard();

	void OpenIn(void);
	void CloseIn(void);

	void OpenOut(DWORD dwPos, DWORD dwEndPos);
	void PauseOut(void);
	void CloseOut(void);

	static void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

};

extern CDataFromSoundCard clsGetDataSDC;