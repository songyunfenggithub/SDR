// Wave.h: interface for the CSoundCard class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVE_H__33D80B25_F366_4436_B3A4_A0F55957D1FF__INCLUDED_)
#define AFX_WAVE_H__33D80B25_F366_4436_B3A4_A0F55957D1FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <minwindef.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <commdlg.h>

#define SOUNDCARD_IN_STEP_LENGTH	0x1000
#define SOUNDCARD_IN_BUFFER_LENGTH	0x100000
#define SOUNDCARD_OUT_STEP_LENGTH	0x1000
#define SOUNDCARD_OUT_BUFFER_LENGTH	0x10000

//#define SOUNDCARD_SAMPLE	44100L
#define SOUNDCARD_SAMPLE	11025L


class CSoundCard  
{
public:

	WAVEFORMATEX	FormatEx; 

	unsigned char inBuffer[SOUNDCARD_IN_BUFFER_LENGTH];
	unsigned char outBuffer[SOUNDCARD_OUT_BUFFER_LENGTH];
	DWORD	outBufLength = 0;
	DWORD	outPos = 0, outEndPos = 0;
	DWORD	inPos = 0, inEndPos = 0;

	HWND	hWndWaveValue;
	
	HWND	hWndPlayStopPosition;
	DWORD	dwPlayStopPosition;

typedef struct INDATATAG
{
	BOOL		fInOpen;
	HWAVEIN		hWaveIn; 
    WAVEHDR		WaveInHdr1; 
	WAVEHDR		WaveInHdr2;
} INDATA,*PINDATA;

	INDATA InData;

typedef struct OUTDATATAG
{
	BOOL		fOutOpen;
	HWAVEOUT	hWaveOut; 
    WAVEHDR		WaveOutHdr1;
	WAVEHDR		WaveOutHdr2;
} OUTDATA,*POUTDATA;

	OUTDATA	OutData;

public:
	CSoundCard();
	~CSoundCard();

	void OpenIn(UINT pos, UINT endPos);
	void CloseIn(void);
	void FreeIn(void);

	void OpenOut(DWORD dwPos, DWORD dwEndPos);
	void PauseOut(void);
	void CloseOut(void);
	void FreeOut(void);
	void MyWave(PTCHAR pBuf);

	void GeneratorWave(void);
	void GetWave(unsigned char*p, DWORD dwLength);
	void GetWave2(HDC hDC, unsigned char*p, DWORD dwLength);

	static void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

	static LRESULT CALLBACK DlgGotoProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK DlgPropertiesProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK DlgWaveValueProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK DlgPlayStopPosProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


};

extern CSoundCard clsSoundCard;

#endif // !defined(AFX_WAVE_H__33D80B25_F366_4436_B3A4_A0F55957D1FF__INCLUDED_)
