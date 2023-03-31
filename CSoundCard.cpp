// Wave.cpp: implementation of the CSoundCard class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include <stdio.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <math.h>


#include "public.h"
#include "CWaveData.h"
#include "CWinMain.h"
#include "CSoundCard.h"
#include "MyDebug.h"

CSoundCard clsSoundCard;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CSoundCard::CSoundCard()
{

	hWndWaveValue = NULL;

	OutData.fOutOpen	= FALSE;
	InData.fInOpen		= FALSE;

	FormatEx.cbSize = sizeof( WAVEFORMATEX );
	FormatEx.wFormatTag = WAVE_FORMAT_PCM; 

	FormatEx.nChannels = 1; 
	FormatEx.nSamplesPerSec = SOUNDCARD_SAMPLE;
	FormatEx.nAvgBytesPerSec = SOUNDCARD_SAMPLE;
	FormatEx.nBlockAlign = 1; 
	FormatEx.wBitsPerSample = 8;
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

CSoundCard::~CSoundCard()
{
	if(InData.fInOpen)waveInClose(InData.hWaveIn);
	if(OutData.fOutOpen)waveOutClose(OutData.hWaveOut);
}

void CSoundCard::ClearNoise(void)
{
	DWORD i;
	unsigned char *p = (unsigned char*)outBuffer + 1;
	DWORD dwPrevMax, dwMax, dwPrevMin, dwMin;

	dwPrevMax = dwMax = dwPrevMin = dwMin = 0;
	bool fUp = false;
	double dbHz, dbSec, dbWidth;
	dbHz = dbSec = 0.0;
	
	outBufLength = SOUNDCARD_OUT_BUFFER_LENGTH;

	for(i = 1; i < outBufLength; i++, p++)
	{
		if(*p > *(p - 1))
		{
			if(!fUp)
			{
				//get a min
				dwMin = i;
				fUp = true;
			}
		}
		else if(*p < *(p - 1))
		{
			if(fUp)
			{
				//get a max
				if((i - dwMax) < 8)
				{
					for(DWORD j = 0,l = i - dwMax; j < l; j++)
					{
						*(p - j) = *p;
					}
					/*
				dbWidth = i - dwMax;
				dbSec = dbWidth / FormatEx.nSamplesPerSec;
				dbHz = 1 / dbSec;
				DbgMsg(("Pos: %d, Width: %f, Sec: %f, Hz: %f.", 
					dwMax, dbWidth, dbSec, dbHz));
					*/
				}
				dwMax = i;
				fUp = false;
			}
		}

	}
}

void CSoundCard::ClearNoise2(void)
{
	DWORD i,j,k;
	unsigned char *p2,*p = (unsigned char*)outBuffer + 1;
	DWORD dwPrevMax, dwMax, dwPrevMin, dwMin;

	dwPrevMax = dwMax = dwPrevMin = dwMin = 0;
	bool fUp = false;
	double dbHz, dbSec, dbWidth;
	dbHz = dbSec = 0.0;
	WORD E,L;
	E = 0;
	L = 3;
	
	outBufLength = SOUNDCARD_OUT_BUFFER_LENGTH;

	for(i = 1,j = 0; i < outBufLength; i++, p++, j++)
	{
		p2 = p;
		for(E = k = 0; k < L; k++)
		{
			E += *p2++;
		}
		*p = E / L;
	}
}

void CSoundCard::ClearNoise3(void)
{
	unsigned char *p = (unsigned char*)outBuffer;
	PUCHAR pMin,pPrevMin, pMax, pPrevMax, pEnd;
	DWORD dwH = 0,dwPrevH = 0;

	bool fUp = false;
	double dbHz, dbSec, dbWidth;
	dbHz = dbSec = 0.0;

	int i,j,k;
	i = j = k = 0;
	bool fMax = false;
	PUCHAR pp, pFirstMin;
	pFirstMin = NULL;

	pEnd = outBuffer + outBufLength;

	for(p = outBuffer; p < pEnd; p++)
	{
		if(*p > *(p - 1))
		{
			if(!fUp)
			{
				//get a min
				pMin = p - 1;
				fUp = true;
				if(fMax)
				{
					pFirstMin = pMin;
					fMax = false;
				}
			}
		}
		else if(*p < *(p - 1))
		{
			if(fUp)
			{
				//get a max
				pMax = p - 1;
				dwH = *pMax - *pMin;
				if(dwH > dwPrevH && dwH > 0x10)
				{
					DbgMsg("%d. Pos: %d, Width: %d, High: %d", 
						i++, p - pBuffer,pMax - pMin, dwH);
					//*p = 0xFF;
					fMax = true;

					if(pFirstMin){
					int w = pMin - pFirstMin,
						h = *pFirstMin - *pMin;
					for(pp = pFirstMin; pp < pMin; pp++)
					{
						*pp = *pFirstMin - h + h*(pMin - pp)/w;
					}
					pFirstMin = NULL;
					}

				}
				dwPrevH = dwH;
				fUp = false;
			}
		}

	}
}

LRESULT CALLBACK CSoundCard::DlgGotoProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDOK:
				{
				DWORD dwPos = GetDlgItemInt(hDlg, IDC_EDITGOTO, 0, 0);
				clsWinMain.DrawInfo.dwHZoomedPos = 
					clsWinMain.DrawInfo.iHZoom > 0 ?
					(dwPos << clsWinMain.DrawInfo.iHZoom)
					:
					(dwPos >> - clsWinMain.DrawInfo.iHZoom);
				}
			case IDCANCEL:
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
				break;
			}
			break;
	}
    return FALSE;
}

LRESULT CALLBACK CSoundCard::DlgPlayStopPosProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemInt(hDlg,	IDC_EDIT_PLAY_STOP_POSITION, 
					clsSoundCard.dwPlayStopPosition, TRUE);
				return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDC_STATIC_PLAY_STOP_POSITION:
				if (HIWORD(wParam) == STN_CLICKED) 
                {
					SetDlgItemInt(hDlg,	IDC_EDIT_PLAY_STOP_POSITION, 
						clsWinMain.DrawInfo.dwHZoomedPos, TRUE);
				}
				break;
			case IDOK:
				{
				clsSoundCard.dwPlayStopPosition = GetDlgItemInt(hDlg, 
					IDC_EDIT_PLAY_STOP_POSITION, 0, 0);
				clsSoundCard.dwPlayStopPosition = 
					clsWinMain.DrawInfo.iHZoom > 0 ?
					(clsSoundCard.dwPlayStopPosition << clsWinMain.DrawInfo.iHZoom)
					:
					(clsSoundCard.dwPlayStopPosition >> - clsWinMain.DrawInfo.iHZoom);
				}
			case IDCANCEL:
				EndDialog(hDlg, LOWORD(wParam));
				clsSoundCard.hWndPlayStopPosition = NULL;
				return TRUE;
				break;
			}
			break;
	}
    return FALSE;
}

LRESULT CALLBACK CSoundCard::DlgWaveValueProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDC_BUTTON_SHOW:
			{
	PUCHAR p = (PUCHAR)clsSoundCard.outBuffer;
	PUCHAR pMin,pPrevMin, pMax, pPrevMax, pEnd;
	DWORD dwH = 0,dwPrevH = 0;

	bool fUp = false;
	double dbHz, dbSec, dbWidth;
	dbHz = dbSec = 0.0;

	int i,j,k;
	i = j = k = 0;

   	HDC		hDC;
    PAINTSTRUCT ps;
	RECT	rt;
	HPEN	hPen;

	hDC = GetDC(hDlg);

	hPen = CreatePen(PS_DOT, 0, RGB(128,128,128));
	SetBkColor(hDC, RGB(0,0,0));
					

	pEnd = clsSoundCard.outBuffer + clsSoundCard.outBufLength;
	for(i = 0; p < pEnd; p++)
	{
		if(*p > *(p - 1))
		{
			if(!fUp)
			{
				//get a min
				pMin = p - 1;
				fUp = true;
			}
		}
		else if(*p < *(p - 1))
		{
			if(fUp)
			{
				//get a max
				pMax = p - 1;
				dwH = *pMax - *pMin;
				if(dwH > dwPrevH && dwH > 0x10)
				{
					MoveToEx (hDC, i, 0, NULL);
					LineTo (hDC, i, dwH);

					MoveToEx (hDC, i, 0xFF, NULL);
					LineTo (hDC, i, 0xFF + (pMax - pMin));
					i++;
				}
				dwPrevH = dwH;
				fUp = false;
			}
		}

	}
			}
			break;
			case IDCANCEL:
				EndDialog(hDlg, LOWORD(wParam));
				clsSoundCard.hWndWaveValue = NULL;
				return TRUE;
			}
			break;
	}
    return FALSE;

}

LRESULT CALLBACK CSoundCard::DlgPropertiesProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			TCHAR s[1024];
			sprintf(s,
"wFormatTag: %d\r\n\
nChannels: %d\r\n\
nSamplesPerSec: %d\r\n\
nAvgBytesPerSec: %d\r\n\
nBlockAlign: %d\r\n\
wBitsPerSample: %d\r\n\
cbSize: %d",
				    clsSoundCard.FormatEx.wFormatTag, 
					clsSoundCard.FormatEx.nChannels, 
					clsSoundCard.FormatEx.nSamplesPerSec,
					clsSoundCard.FormatEx.nAvgBytesPerSec, 
					clsSoundCard.FormatEx.nBlockAlign,
					clsSoundCard.FormatEx.wBitsPerSample,
					clsSoundCard.FormatEx.cbSize
					);
			SetWindowText(GetDlgItem(hDlg, IDC_STATICINFO),s);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDOK:
			case IDCANCEL:
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

void CSoundCard::GetWave(unsigned char *p, DWORD dwLength)
{
	DWORD i;
	//unsigned char *p = (unsigned char*)pBuffer + dwPos + 1;
	DWORD dwPrevMax, dwMax, dwPrevMin, dwMin;
	TCHAR szDebug[500];

	dwPrevMax = dwMax = dwPrevMin = dwMin = 0;
	bool fUp = false;
	double dbHz, dbSec, dbWidth;
	dbHz = dbSec = 0.0;
	for(i = 1; i < dwLength; i++, p++)
	{
		if(*p > *(p - 1))
		{
			if(!fUp)
			{
				//get a min
				dwMin = i;
				fUp = true;
			}
		}
		else if(*p < *(p - 1))
		{
			if(fUp)
			{
				//get a max
				if((*p - *(p - dwMin)) > 0xF)
				{
				dbWidth = i - dwMax;
				dbSec = dbWidth / FormatEx.nSamplesPerSec;
				dbHz = 1 / dbSec;
			//	DbgMsg(("Pos: %d, Width: %f, Sec: %f, Hz: %f.", 
			//		dwMax, dbWidth, dbSec, dbHz));
			//	MessageBox(clsWinMain.hWnd, szDebug, "", IDOK);
//				OutputDebugString(szDebug);
				}
				dwMax = i;
				fUp = false;
			}
		}

	}
}

void CSoundCard::GetWave2(HDC hDC,unsigned char *p, DWORD dwLength)
{
	DWORD i;
	//unsigned char *p = (unsigned char*)pBuffer + dwPos + 1;
	DWORD dwPrevMax, dwMax, dwPrevMin, dwMin;
	TCHAR szOut[500];

	RECT rc;
	int iTextX = 0, iTextY = 350;

	dwPrevMax = dwMax = dwPrevMin = dwMin = 0;
	bool fUp = false;
	double dbHz, dbSec, dbWidth;
	dbHz = dbSec = 0.0;
	for(i = 1; i < dwLength; i++, p++)
	{
		if(*p > *(p - 1))
		{
			if(!fUp)
			{
				//get a min
				dwMin = i - 1;
				fUp = true;
			}
		}
		else if(*p < *(p - 1))
		{
			if(fUp)
			{
				if(*(p - 1) - *(outBuffer + dwMin) > 5)
				{
				dbWidth = i - dwMax;
				dbSec = dbWidth / FormatEx.nSamplesPerSec;
				dbHz = 1 / dbSec;
				int iCount = sprintf(szOut,"W:%d,H:%d. ",i - dwMin, *(p - 1) - *(p - i + dwMin));
				DrawText(hDC, szOut, iCount, &rc, DT_LEFT | DT_CALCRECT);
				SetTextColor(hDC,RGB(255,255,0));
				TextOut(hDC,iTextX,iTextY,szOut,iCount);
				iTextX += 100;
				if(iTextX > 800)
				{
					iTextX = 0;
					iTextY += 15;
				}
				dwMax = i - 1;
				}
				fUp = false;
			}
		}

	}
}

void CALLBACK CSoundCard::waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch(uMsg)
	{
	case WIM_OPEN:

		break;
	case WIM_CLOSE:
		waveInUnprepareHeader(clsSoundCard.InData.hWaveIn, &clsSoundCard.InData.WaveInHdr1, sizeof(WAVEHDR)); 
		waveInUnprepareHeader(clsSoundCard.InData.hWaveIn, &clsSoundCard.InData.WaveInHdr2, sizeof(WAVEHDR));

		//clsSoundCard.FreeIn();
		break;
	case WIM_DATA:
		if(clsSoundCard.inBufLength * clsSoundCard.FormatEx.nBlockAlign > SOUNDCARD_IN_BUFFER_LENGTH - SOUNDCARD_IN_STEP_LENGTH)
		{
			clsSoundCard.CloseIn();
			return;
		}
		LPWAVEHDR pWaveHdr = (LPWAVEHDR)dwParam1;
		if (pWaveHdr->dwBytesRecorded != SOUNDCARD_IN_STEP_LENGTH)
			DbgMsg("Sound Card Record dwBytesRecorded %d : $d\r\n", pWaveHdr->dwBytesRecorded, SOUNDCARD_IN_STEP_LENGTH);
		clsSoundCard.inBufLength += pWaveHdr->dwBytesRecorded;
		clsSoundCard.inPos += pWaveHdr->dwBytesRecorded;
		if (clsSoundCard.inEndPos - clsSoundCard.inPos <= 0) {
			clsSoundCard.CloseIn();
			break;
		}
		pWaveHdr->lpData = (char*)clsSoundCard.inBuffer + clsSoundCard.inBufLength;
		pWaveHdr->dwBufferLength = (clsSoundCard.inEndPos - clsSoundCard.inPos) < SOUNDCARD_IN_STEP_LENGTH ? 
			(clsSoundCard.inEndPos - clsSoundCard.inPos) : SOUNDCARD_IN_STEP_LENGTH;
		waveInAddBuffer(hwi, pWaveHdr, sizeof(WAVEHDR));
		break;
	}
}
 
void CALLBACK CSoundCard::waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch(uMsg)
	{
	case WOM_OPEN:
		break;
	case WOM_CLOSE:
		waveOutUnprepareHeader(clsSoundCard.OutData.hWaveOut, &clsSoundCard.OutData.WaveOutHdr1, sizeof(WAVEHDR)); 
		waveOutUnprepareHeader(clsSoundCard.OutData.hWaveOut, &clsSoundCard.OutData.WaveOutHdr2, sizeof(WAVEHDR));
		break;
	case WOM_DONE:
		clsSoundCard.outPos += SOUNDCARD_OUT_STEP_LENGTH;
		if (clsSoundCard.outEndPos - clsSoundCard.outPos <= 0) {
			clsSoundCard.CloseOut();
		}
		else {
			LPWAVEHDR pWaveHdr = (LPWAVEHDR)dwParam1;
			//if(pWaveHdr->dwFlags == 0)
			pWaveHdr->lpData = (char*)clsSoundCard.outBuffer + clsSoundCard.outPos;
			pWaveHdr->dwBufferLength = (clsSoundCard.outEndPos - clsSoundCard.outPos) < SOUNDCARD_OUT_STEP_LENGTH ?
				(clsSoundCard.outEndPos - clsSoundCard.outPos) : SOUNDCARD_OUT_STEP_LENGTH;
			waveOutPrepareHeader(clsSoundCard.OutData.hWaveOut, pWaveHdr, sizeof(WAVEHDR));
			waveOutWrite(clsSoundCard.OutData.hWaveOut, pWaveHdr, sizeof(WAVEHDR));
		}
		break;

	}
}

void CSoundCard::CloseIn(void) 
{
	if(InData.fInOpen)
	{
		waveInStop(InData.hWaveIn);	
		waveInClose(InData.hWaveIn);
	}
	InData.fInOpen = FALSE;
} 

void CSoundCard::PauseOut(void) 
{
	waveOutPause(OutData.hWaveOut);	
} 

void CSoundCard::CloseOut(void) 
{
	if(OutData.fOutOpen)
	{
		waveOutPause(OutData.hWaveOut);
		//waveOutReset(OutData.hWaveOut);	
	//	waveOutClose(OutData.hWaveOut);	
	}
	OutData.fOutOpen = FALSE;
} 

void CSoundCard::OpenIn(UINT pos, UINT endPos) 
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
 
	inBufLength = 0;
	inPos = pos;
	inEndPos = endPos;

    InData.WaveInHdr1.lpData = (char*)inBuffer + inPos;
    InData.WaveInHdr1.dwBufferLength = (inEndPos - inPos) < SOUNDCARD_IN_STEP_LENGTH ? (inEndPos - inPos) : SOUNDCARD_IN_STEP_LENGTH;
    InData.WaveInHdr1.dwFlags = WHDR_DONE;
    InData.WaveInHdr1.dwLoops = 0L; 

	waveInPrepareHeader(InData.hWaveIn, &InData.WaveInHdr1, sizeof(WAVEHDR));
	waveInAddBuffer(InData.hWaveIn, &InData.WaveInHdr1, sizeof(WAVEHDR));
	wResult = waveInStart(InData.hWaveIn);

	inPos += SOUNDCARD_IN_STEP_LENGTH;
	if ((inEndPos - inPos) <= 0) return;
    
	InData.WaveInHdr2.lpData = (char*)inBuffer + inPos;
	InData.WaveInHdr2.dwBufferLength = (inEndPos - inPos) < SOUNDCARD_IN_STEP_LENGTH ? (inEndPos - inPos) : SOUNDCARD_IN_STEP_LENGTH;
	InData.WaveInHdr2.dwFlags = WHDR_DONE;
	InData.WaveInHdr2.dwLoops = 0L;

	waveInPrepareHeader(InData.hWaveIn, &InData.WaveInHdr2, sizeof(WAVEHDR));
	waveInAddBuffer(InData.hWaveIn, &InData.WaveInHdr2, sizeof(WAVEHDR));

}

void CSoundCard::OpenOut(DWORD dwPos, DWORD dwEndPos) 
{ 
	UINT  wResult; 

	if(dwEndPos < dwPos || dwEndPos > outBufLength)dwEndPos = outBufLength;
	
    if (waveOutOpen((LPHWAVEOUT)&OutData.hWaveOut, WAVE_MAPPER, 
                    (LPWAVEFORMATEX)&FormatEx, 
                    (DWORD_PTR)waveOutProc, 0L, CALLBACK_FUNCTION))
    { 
        MessageBox(clsWinMain.hWnd, 
                   "Failed to open waveform output device.", 
                   NULL, MB_OK | MB_ICONEXCLAMATION); 
        return; 
    } 

	outPos = dwPos;
	outEndPos = dwEndPos;

	OutData.fOutOpen = TRUE;

	OutData.WaveOutHdr1.lpData = (char*)outBuffer + outPos;// * FormatEx.nBlockAlign; 
	OutData.WaveOutHdr1.dwBufferLength = (outEndPos - outPos) < SOUNDCARD_OUT_STEP_LENGTH ? (outEndPos - outPos) : SOUNDCARD_OUT_STEP_LENGTH;
	OutData.WaveOutHdr1.dwFlags = 0L;
	OutData.WaveOutHdr1.dwLoops = 0L;
	waveOutPrepareHeader(OutData.hWaveOut, &OutData.WaveOutHdr1, sizeof(WAVEHDR));
	wResult = waveOutWrite(OutData.hWaveOut, &OutData.WaveOutHdr1, sizeof(WAVEHDR));

	outPos += SOUNDCARD_OUT_STEP_LENGTH;
	if (outPos > outEndPos) return;

	OutData.WaveOutHdr2.lpData = (char*)outBuffer + outPos;// * FormatEx.nBlockAlign; 
	OutData.WaveOutHdr2.dwBufferLength = (outEndPos - outPos) < SOUNDCARD_OUT_STEP_LENGTH ? (outEndPos - outPos) : SOUNDCARD_OUT_STEP_LENGTH;
	OutData.WaveOutHdr2.dwFlags = 0L;
	OutData.WaveOutHdr2.dwLoops = 0L;
	waveOutPrepareHeader(OutData.hWaveOut, &OutData.WaveOutHdr2, sizeof(WAVEHDR));
    wResult = waveOutWrite(OutData.hWaveOut, &OutData.WaveOutHdr2, sizeof(WAVEHDR)); 
    
//	if (wResult != 0)FreeOut();
} 

void CSoundCard::FreeOut(void)
{
//	waveOutUnprepareHeader(OutData.hWaveOut, &OutData.WaveOutHdr, sizeof(WAVEHDR)); 
}

void CSoundCard::FreeIn(void)
{
//	waveInUnprepareHeader(InData.hWaveIn, &InData.WaveInHdr, sizeof(WAVEHDR)); 
}

void CSoundCard::GeneratorWave(void)
{
	static double dbW1 = 0.0, dbW2 = 0.0;

	double dbHz1,dbHz2,dbStep1,dbStep2;
	double pi = 3.1415926535;

	DWORD	dwI, Sample11, Sample41;

	Sample11 = 11025;
	Sample41 = 44100;

	dbHz1 = 200.6;
	dbHz2 = 500.7;

	
	dbStep1 = dbHz1*2*pi / Sample11;
	dbStep2 = dbHz2*2*pi / Sample11;


	CHAR* p = (CHAR*)outBuffer;

	outBufLength = SOUNDCARD_OUT_BUFFER_LENGTH;

	for(dwI = 0; dwI < outBufLength; dwI++)
	{
		*(p++) = (CHAR)(sin(dbW1 += dbStep1) * 0x4F + 0x80 +
			sin(dbW2 += dbStep2) * 0x1F);
	}
}
 
void CSoundCard::MyWave(PTCHAR pBuf)
{
	return;
	static double dbW1 = 0.0, dbW2 = 0.0;

	double dbHz1,dbHz2,dbStep1,dbStep2;
	double pi = 3.1415926535;

	DWORD	dwI;

	dbHz1 = 350.3;
	dbHz2 = 1000.5;

	dbStep1 = dbHz1*2*pi / SOUNDCARD_SAMPLE;
	dbStep2 = dbHz2*2*pi / SOUNDCARD_SAMPLE;


	unsigned char* p = outBuffer;

	outBufLength = 1 * 1024 * 1024;

	for(dwI = 0; dwI < outBufLength; dwI++)
	{
		*(p++) =	sin(dbW1+=dbStep1)*0x4F + 0x80 + 
					sin(dbW2+=dbStep2)*0x1F;
	}
}
