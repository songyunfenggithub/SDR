
#include "stdafx.h"
#include "resource.h"

#include <stdio.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <math.h>


#include "public.h"
#include "CData.h"
#include "CWinMain.h"
#include "CDataFromSoundCard.h"
#include "MyDebug.h"

CDataFromSoundCard		clsGetDataSDC;

#define IN_BUFFER_LENGTH	0x10*1024
#define OUT_BUFFER_LENGTH	0x10*1024

CDataFromSoundCard::CDataFromSoundCard()
{
	OutData.fOutOpen = FALSE;
	InData.fInOpen = FALSE;

	FormatEx.cbSize = sizeof(WAVEFORMATEX);
	FormatEx.wFormatTag = WAVE_FORMAT_PCM;

	FormatEx.nChannels = 1;
	FormatEx.wBitsPerSample = 16;
	FormatEx.nBlockAlign = FormatEx.nChannels * FormatEx.wBitsPerSample / 8;
	FormatEx.nSamplesPerSec = 44100L;
	FormatEx.nAvgBytesPerSec = FormatEx.nSamplesPerSec * FormatEx.nBlockAlign;

	/*
	FormatEx.nChannels = 1;
	FormatEx.wBitsPerSample = 8;
	FormatEx.nBlockAlign = FormatEx.nChannels * FormatEx.wBitsPerSample / 8;
	FormatEx.nSamplesPerSec = 11025L;
	FormatEx.nAvgBytesPerSec = FormatEx.nSamplesPerSec * FormatEx.nBlockAlign;

	FormatEx.nChannels = 1;
	FormatEx.wBitsPerSample = 8;
	FormatEx.nBlockAlign = FormatEx.nChannels * FormatEx.wBitsPerSample / 8;
	FormatEx.nSamplesPerSec = 44100L;
	FormatEx.nAvgBytesPerSec = FormatEx.nSamplesPerSec * FormatEx.nBlockAlign;
	
	FormatEx.nChannels = 1;
	FormatEx.wBitsPerSample = 16;
	FormatEx.nBlockAlign = FormatEx.nChannels * FormatEx.wBitsPerSample / 8;
	FormatEx.nSamplesPerSec = 44100L;
	FormatEx.nAvgBytesPerSec = FormatEx.nSamplesPerSec * FormatEx.nBlockAlign;

	FormatEx.nChannels = 2;
	FormatEx.wBitsPerSample = 16;
	FormatEx.nBlockAlign = FormatEx.nChannels * FormatEx.wBitsPerSample / 8;
	FormatEx.nSamplesPerSec = 44100L;
	FormatEx.nAvgBytesPerSec = FormatEx.nSamplesPerSec * FormatEx.nBlockAlign;

	FormatEx.nChannels = 1;
	FormatEx.wBitsPerSample = 8;
	FormatEx.nBlockAlign = FormatEx.nChannels * FormatEx.wBitsPerSample / 8;
	FormatEx.nSamplesPerSec = 44100L;
	FormatEx.nAvgBytesPerSec = FormatEx.nSamplesPerSec * FormatEx.nBlockAlign;
	*/


}

CDataFromSoundCard::~CDataFromSoundCard()
{
	if (InData.fInOpen)waveInClose(InData.hWaveIn);
	if (OutData.fOutOpen)waveOutClose(OutData.hWaveOut);
}

void CALLBACK CDataFromSoundCard::waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case WIM_OPEN:

		break;
	case WIM_CLOSE:
		waveInUnprepareHeader(clsGetDataSDC.InData.hWaveIn, &clsGetDataSDC.InData.WaveInHdr, sizeof(WAVEHDR));
		break;
	case WIM_DATA:
		LPWAVEHDR pWaveHdr = (LPWAVEHDR)dwParam1;
		clsData.AdcGetCharLength += pWaveHdr->dwBytesRecorded;
		//clsData.AdcPos += pWaveHdr->dwBytesRecorded / clsGetDataSDC.FormatEx.nBlockAlign;
		//if (clsData.AdcPos > DATA_BUFFER_LENGTH) clsData.AdcPos -= DATA_BUFFER_LENGTH;
		if (clsData.AdcGetCharLength >= (DATA_BUFFER_LENGTH << DATA_BYTE_TO_POSITION_MOVEBIT))
			clsData.AdcGetCharLength -= DATA_BUFFER_LENGTH << DATA_BYTE_TO_POSITION_MOVEBIT;
		clsData.AdcPos = clsData.AdcGetCharLength >> DATA_BYTE_TO_POSITION_MOVEBIT;
		clsData.AdcGetNew = true;

		pWaveHdr->lpData = (char*)clsData.AdcBuff + clsData.AdcGetCharLength;
		waveInAddBuffer(hwi, pWaveHdr, sizeof(WAVEHDR));

		break;
	}
}

void CALLBACK CDataFromSoundCard::waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case WOM_OPEN:
		break;
	case WOM_CLOSE:
		waveOutUnprepareHeader(clsGetDataSDC.OutData.hWaveOut, &clsGetDataSDC.OutData.WaveOutHdr, sizeof(WAVEHDR));
		break;
	case WOM_DONE:
		clsGetDataSDC.CloseOut();
		break;

	}
}

void CDataFromSoundCard::CloseIn(void)
{
	if (InData.fInOpen)
	{
		waveInStop(InData.hWaveIn);
		waveInClose(InData.hWaveIn);
	}
	InData.fInOpen = FALSE;
}

void CDataFromSoundCard::PauseOut(void)
{
	waveOutPause(OutData.hWaveOut);
}

void CDataFromSoundCard::CloseOut(void)
{
	if (OutData.fOutOpen)
	{
		//waveOutPause(OutData.hWaveOut);
		//waveOutReset(OutData.hWaveOut);	
		waveOutClose(OutData.hWaveOut);
	}
	OutData.fOutOpen = FALSE;
}

void CDataFromSoundCard::OpenIn(void)
{
	UINT        wResult;

	if (waveInOpen((LPHWAVEIN)&InData.hWaveIn, WAVE_MAPPER,
		(LPWAVEFORMATEX)&FormatEx,
		(DWORD_PTR)waveInProc, 0L, CALLBACK_FUNCTION))
	{
		MessageBox(clsWinMain.hWnd,
			"Failed to open waveform input device.",
			NULL, MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	InData.fInOpen = TRUE;

	InData.WaveInHdr.lpData = (char*)clsData.AdcBuff;
	InData.WaveInHdr.dwBufferLength = IN_BUFFER_LENGTH;
	InData.WaveInHdr.dwFlags = WHDR_DONE;
	InData.WaveInHdr.dwLoops = 0L;
	InData.WaveInHdr.lpNext = NULL;

	waveInPrepareHeader(InData.hWaveIn, &InData.WaveInHdr, sizeof(WAVEHDR));
	waveInAddBuffer(InData.hWaveIn, &InData.WaveInHdr, sizeof(WAVEHDR));

	wResult = waveInStart(InData.hWaveIn);
}

void CDataFromSoundCard::OpenOut(DWORD dwPos, DWORD dwEndPos)
{
	UINT  wResult;

	if (dwEndPos < dwPos || dwEndPos >= DATA_BUFFER_LENGTH)dwEndPos = DATA_BUFFER_LENGTH;

	if (waveOutOpen((LPHWAVEOUT)&OutData.hWaveOut, WAVE_MAPPER,
		(LPWAVEFORMATEX)&FormatEx,
		(DWORD_PTR)waveOutProc, 0L, CALLBACK_FUNCTION))
	{
		MessageBox(clsWinMain.hWnd,
			"Failed to open waveform output device.",
			NULL, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	OutData.fOutOpen = TRUE;

	OutData.WaveOutHdr.lpData = (char*)clsData.AdcBuff + dwPos * FormatEx.nBlockAlign;
	OutData.WaveOutHdr.dwBufferLength = (dwEndPos - dwPos) * FormatEx.nBlockAlign;
	OutData.WaveOutHdr.dwFlags = 0L;
	OutData.WaveOutHdr.dwLoops = 0L;

	waveOutPrepareHeader(OutData.hWaveOut, &OutData.WaveOutHdr, sizeof(WAVEHDR));
	wResult = waveOutWrite(OutData.hWaveOut, &OutData.WaveOutHdr, sizeof(WAVEHDR));

	//	if (wResult != 0) waveOutUnprepareHeader(OutData.hWaveOut, &OutData.WaveOutHdr, sizeof(WAVEHDR)); 
}