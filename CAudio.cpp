
#include "stdafx.h"
#include "resource.h"
#include <iostream>

#include <stdio.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <commdlg.h>
#include <math.h>

#include "public.h"
#include "myDebug.h"

#include "CWaveFilter.h"
#include "CData.h"
#include "cuda_CFilter.cuh"
#include "CAudio.h"

CAudio mAudio;

CAudio::CAudio()
{
	OPENCONSOLE;
	Init();
}

CAudio::~CAudio()
{
	UnInit();
}


void CAudio::Init(void)
{

	SampleRate = SOUNDCARD_SAMPLE;

	OutData.fOutOpen = FALSE;
	InData.fInOpen = FALSE;

	FormatEx.cbSize = sizeof(WAVEFORMATEX);
	FormatEx.wFormatTag = WAVE_FORMAT_PCM;

	FormatEx.nSamplesPerSec = SampleRate;
	FormatEx.nChannels = 1;
	FormatEx.wBitsPerSample = 16;
	FormatEx.nBlockAlign = FormatEx.wBitsPerSample / 8 * FormatEx.nChannels;
	FormatEx.nAvgBytesPerSec = FormatEx.nBlockAlign * FormatEx.nSamplesPerSec;
	/*
		FormatEx.nChannels = 1;
		FormatEx.nSamplesPerSec = 44100L;
		FormatEx.nAvgBytesPerSec = 44100L;
		FormatEx.nBlockAlign = 1;
		FormatEx.wBitsPerSample = 8;
	/*
		FormatEx.nChannels = 1;
		FormatEx.nSamplesPerSec = 44100L;
		FormatEx.nAvgBytesPerSec = 88200L;
		FormatEx.nBlockAlign = 2;
		FormatEx.wBitsPerSample = 16;

	/*
		FormatEx.nChannels = 2;
		FormatEx.nSamplesPerSec = 44100L;
		FormatEx.nAvgBytesPerSec = 176400L;
		FormatEx.nBlockAlign = 4;
		FormatEx.wBitsPerSample = 16;
	/*

		FormatEx.nChannels = 1;
		FormatEx.nSamplesPerSec = 44100L;
		FormatEx.nAvgBytesPerSec = 44100L;
		FormatEx.nBlockAlign = 1;
		FormatEx.wBitsPerSample = 8;
		*/
}

void CAudio::UnInit(void)
{

}


void CALLBACK CAudio::waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case WIM_OPEN:

		break;
	case WIM_CLOSE:
		waveInUnprepareHeader(mAudio.InData.hWaveIn, &mAudio.InData.WaveInHdr1, sizeof(WAVEHDR));
		waveInUnprepareHeader(mAudio.InData.hWaveIn, &mAudio.InData.WaveInHdr2, sizeof(WAVEHDR));

		//mAudio.FreeIn();
		break;
	case WIM_DATA:
		if (mAudio.inBeginPos >= mAudio.inEndPos) {
			mAudio.CloseIn();
			return;
		}
		LPWAVEHDR pWaveHdr = (LPWAVEHDR)dwParam1;
		if (pWaveHdr->dwBytesRecorded != pWaveHdr->dwBufferLength) {
			DbgMsg("Sound Card Record dwBytesRecorded %d : $d\r\n", pWaveHdr->dwBytesRecorded, pWaveHdr->dwBufferLength);
		}
		if (mAudio.inEndPos - mAudio.inBeginPos <= 0) {
			mAudio.CloseIn();
			break;
		}
		pWaveHdr->lpData = (char*)(mAudio.inBuff + mAudio.inBeginPos);
		pWaveHdr->dwBufferLength = mAudio.FormatEx.nBlockAlign * 
			((mAudio.inEndPos - mAudio.inBeginPos) < SOUNDCARD_STEP_LENGTH ? (mAudio.inEndPos - mAudio.inBeginPos) : SOUNDCARD_STEP_LENGTH);
		waveInAddBuffer(hwi, pWaveHdr, sizeof(WAVEHDR));

		mAudio.inBeginPos += pWaveHdr->dwBufferLength / mAudio.FormatEx.nBlockAlign;
		mAudio.inBuffPos = mAudio.inBeginPos;
		break;
	}
}

void CALLBACK CAudio::waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case WOM_OPEN:
		break;
	case WOM_CLOSE:
		waveOutUnprepareHeader(mAudio.OutData.hWaveOut, &mAudio.OutData.WaveOutHdr1, sizeof(WAVEHDR));
		waveOutUnprepareHeader(mAudio.OutData.hWaveOut, &mAudio.OutData.WaveOutHdr2, sizeof(WAVEHDR));
		break;
	case WOM_DONE:
		
		if (mAudio.outEndPos <= mAudio.outBeginPos) {
			mAudio.CloseOut();
		}
		else {
			LPWAVEHDR pWaveHdr = (LPWAVEHDR)dwParam1;
			pWaveHdr->lpData = (char*)(mAudio.outBuff + mAudio.outBeginPos);
			pWaveHdr->dwBufferLength = mAudio.FormatEx.nBlockAlign * 
				((mAudio.outEndPos - mAudio.outBeginPos) < SOUNDCARD_STEP_LENGTH ? (mAudio.outEndPos - mAudio.outBeginPos) : SOUNDCARD_STEP_LENGTH);
			waveOutPrepareHeader(mAudio.OutData.hWaveOut, pWaveHdr, sizeof(WAVEHDR));
			waveOutWrite(mAudio.OutData.hWaveOut, pWaveHdr, sizeof(WAVEHDR));

			mAudio.outBeginPos += pWaveHdr->dwBufferLength / mAudio.FormatEx.nBlockAlign;
		}
		break;

	}
}

void CAudio::CloseIn(void)
{
	if (InData.fInOpen)
	{
		waveInStop(InData.hWaveIn);
		waveInClose(InData.hWaveIn);
	}
	InData.fInOpen = FALSE;
}

void CAudio::PauseOut(void)
{
	waveOutPause(OutData.hWaveOut);
}

void CAudio::CloseOut(void)
{
	if (OutData.fOutOpen)
	{
		waveOutPause(OutData.hWaveOut);
		//waveOutReset(OutData.hWaveOut);	
	//	waveOutClose(OutData.hWaveOut);	
	}
	OutData.fOutOpen = FALSE;
}

void CAudio::OpenIn(UINT pos, UINT endPos)
{
	UINT        wResult;

	if (waveInOpen((LPHWAVEIN)&InData.hWaveIn, WAVE_MAPPER,
		(LPWAVEFORMATEX)&FormatEx,
		(DWORD_PTR)waveInProc, 0L, CALLBACK_FUNCTION))
	{
		//MessageBox(clsWinMain.hWnd, "Failed to open waveform input device.", NULL, MB_OK | MB_ICONEXCLAMATION);
		printf("Failed to open waveform input device.\r\n");
		return;
	}

	InData.fInOpen = TRUE;

	inBeginPos = pos;
	inEndPos = endPos;

	InData.WaveInHdr1.lpData = (char*)(inBuff + inBeginPos);
	InData.WaveInHdr1.dwBufferLength = mAudio.FormatEx.nBlockAlign * 
		((inEndPos - inBeginPos) < SOUNDCARD_STEP_LENGTH ? (inEndPos - inBeginPos) : SOUNDCARD_STEP_LENGTH);
	InData.WaveInHdr1.dwFlags = WHDR_DONE;
	InData.WaveInHdr1.dwLoops = 0L;
	waveInPrepareHeader(InData.hWaveIn, &InData.WaveInHdr1, sizeof(WAVEHDR));
	waveInAddBuffer(InData.hWaveIn, &InData.WaveInHdr1, sizeof(WAVEHDR));
	wResult = waveInStart(InData.hWaveIn);

	inBeginPos += InData.WaveInHdr1.dwBufferLength / mAudio.FormatEx.nBlockAlign;
	if ((inEndPos - inBeginPos) <= 0) return;

	InData.WaveInHdr2.lpData = (char*)(inBuff + inBeginPos);
	InData.WaveInHdr2.dwBufferLength = mAudio.FormatEx.nBlockAlign * 
		((inEndPos - inBeginPos) < SOUNDCARD_STEP_LENGTH ? (inEndPos - inBeginPos) : SOUNDCARD_STEP_LENGTH);
	InData.WaveInHdr2.dwFlags = WHDR_DONE;
	InData.WaveInHdr2.dwLoops = 0L;
	waveInPrepareHeader(InData.hWaveIn, &InData.WaveInHdr2, sizeof(WAVEHDR));
	waveInAddBuffer(InData.hWaveIn, &InData.WaveInHdr2, sizeof(WAVEHDR));

	inBeginPos += InData.WaveInHdr2.dwBufferLength / mAudio.FormatEx.nBlockAlign;
}

void CAudio::OpenOut(UINT dwBeginPos, UINT dwEndPos)
{
	while(OutData.fOutOpen == true);

	UINT  wResult;

	if (dwEndPos < dwBeginPos || dwEndPos > SOUNDCARD_BUFF_LENGTH)dwEndPos = SOUNDCARD_BUFF_LENGTH;

	if (waveOutOpen((LPHWAVEOUT)&OutData.hWaveOut, WAVE_MAPPER,
		(LPWAVEFORMATEX)&FormatEx,
		(DWORD_PTR)waveOutProc, 0L, CALLBACK_FUNCTION))
	{
		//MessageBox(clsWinMain.hWnd, "Failed to open waveform output device.", NULL, MB_OK | MB_ICONEXCLAMATION); 
		printf("Failed to open waveform output device.\r\n");
		return;
	}

	outBeginPos = dwBeginPos;
	outEndPos = dwEndPos;

	OutData.fOutOpen = TRUE;

	OutData.WaveOutHdr1.lpData = (char*)(outBuff + outBeginPos);// * FormatEx.nBlockAlign; 
	OutData.WaveOutHdr1.dwBufferLength = mAudio.FormatEx.nBlockAlign * 
		((outEndPos - outBeginPos) < SOUNDCARD_STEP_LENGTH ? (outEndPos - outBeginPos) : SOUNDCARD_STEP_LENGTH);
	OutData.WaveOutHdr1.dwFlags = 0L;
	OutData.WaveOutHdr1.dwLoops = 0L;
	waveOutPrepareHeader(OutData.hWaveOut, &OutData.WaveOutHdr1, sizeof(WAVEHDR));
	wResult = waveOutWrite(OutData.hWaveOut, &OutData.WaveOutHdr1, sizeof(WAVEHDR));

	outBeginPos += OutData.WaveOutHdr1.dwBufferLength / mAudio.FormatEx.nBlockAlign;
	if (outBeginPos >= outEndPos) return;

	OutData.WaveOutHdr2.lpData = (char*)(outBuff + outBeginPos);// * FormatEx.nBlockAlign; 
	OutData.WaveOutHdr2.dwBufferLength = mAudio.FormatEx.nBlockAlign * 
		((outEndPos - outBeginPos) < SOUNDCARD_STEP_LENGTH ? (outEndPos - outBeginPos) : SOUNDCARD_STEP_LENGTH);
	OutData.WaveOutHdr2.dwFlags = 0L;
	OutData.WaveOutHdr2.dwLoops = 0L;
	waveOutPrepareHeader(OutData.hWaveOut, &OutData.WaveOutHdr2, sizeof(WAVEHDR));
	wResult = waveOutWrite(OutData.hWaveOut, &OutData.WaveOutHdr2, sizeof(WAVEHDR));

	outBeginPos += OutData.WaveOutHdr2.dwBufferLength / mAudio.FormatEx.nBlockAlign;
}


void CALLBACK CAudio::waveOutProc2(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case WOM_OPEN:
		break;
	case WOM_CLOSE:
		for (int i = 0; i < SOUNDCARD_WAVEHDR_DEEP; i++)
		{
			waveOutUnprepareHeader(mAudio.hWaveOut, &mAudio.WaveOutHdrs[i], sizeof(WAVEHDR));
		}
		break;
	case WOM_DONE:
		mAudio.waveHDRPosTail++;
		mAudio.waveHDRPosTail &= SOUNDCARD_WAVEHDR_DEEP_MASK;
		break;
	}
}

void CAudio::StartOpenOut(void)
{
	if (boutOpened == true) return;
	UINT  wResult;
	FormatEx.nSamplesPerSec = SampleRate;
	FormatEx.nChannels = 1;
	FormatEx.wBitsPerSample = 16;
	FormatEx.nBlockAlign = FormatEx.wBitsPerSample / 8 * FormatEx.nChannels;
	FormatEx.nAvgBytesPerSec = FormatEx.nBlockAlign * FormatEx.nSamplesPerSec;
	if (waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER,
		(LPWAVEFORMATEX)&FormatEx,
		(DWORD_PTR)waveOutProc2, 0L, CALLBACK_FUNCTION))
	{
		printf("Failed to open waveform output device.\r\n");
		return;
	}
	waveHDRPosHead = 0;
	waveHDRPosTail = 0;
	boutOpened = true;
}

void CAudio::StopOpenOut(void)
{
	if (boutOpened == true) {
		//waveOutPause(hWaveOut);
		//waveOutReset(hWaveOut);	
		boutOpened = false;
		waveOutClose(hWaveOut);	
	}
}

INT CAudio::WriteToOut(UINT pos)
{
	if (boutOpened == false) return -1;

	WAVEHDR* wavehdr = &WaveOutHdrs[waveHDRPosHead++];
	waveHDRPosHead &= SOUNDCARD_WAVEHDR_DEEP_MASK;

	wavehdr->lpData = (char*)(outBuff + pos);// * FormatEx.nBlockAlign; 
	wavehdr->dwBufferLength = mAudio.FormatEx.nBlockAlign * SOUNDCARD_STEP_LENGTH;
	wavehdr->dwFlags = 0L;
	wavehdr->dwLoops = 0L;
	waveOutPrepareHeader(hWaveOut, wavehdr, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, wavehdr, sizeof(WAVEHDR));
	
	//if (((waveHDRPosHead - waveHDRPosTail) & SOUNDCARD_WAVEHDR_DEEP_MASK) > 2) DbgMsg("waveHDRPos=%d\r\n", (waveHDRPosHead - waveHDRPosTail) & SOUNDCARD_WAVEHDR_DEEP_MASK);

	return (waveHDRPosHead - waveHDRPosTail) & SOUNDCARD_WAVEHDR_DEEP_MASK;
}

void CAudio::GeneratorWave(void)
{
	static double dbW1 = 0.0, dbW2 = 0.0;

	double dbHz1, dbHz2, dbStep1, dbStep2;
	double pi = 3.1415926535;

	DWORD	dwI, Sample11, Sample41;

	Sample11 = 11025;
	Sample41 = 44100;

	dbHz1 = 200.6;
	dbHz2 = 500.7;


	dbStep1 = dbHz1 * 2 * pi / Sample11;
	dbStep2 = dbHz2 * 2 * pi / Sample11;


	CHAR* p = (CHAR*)outBuff;

	for (dwI = 0; dwI < SOUNDCARD_BUFF_LENGTH; dwI++)
	{
		*(p++) = (CHAR)(sin(dbW1 += dbStep1) * 0x4F + 0x80 +
			sin(dbW2 += dbStep2) * 0x1F);
	}
}
