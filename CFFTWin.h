#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

#include "CMessage.h"

#define FFT_WIN_CLASS		"FFT_WIN_CLASS"

#define FFT_DEEP	0x10

class CFFT;

class CFFTWin
{
public:
	const char* Tag = NULL;

	GetStrFunction GetStrFunc = NULL;

	HWND hWnd = NULL;
	HMENU hMenu = NULL;
	HMENU hMenuSpect = NULL;

	UINT uTimerId;

	INT MouseX;
	INT MouseY;

	RECT WinRect;

	char strMouse[1024];

	CMessage msgs;

	int VScrollPos = 0, VScrollHeight = 0;
	int HScrollPos = 0, HScrollWidth = 0;
	double HScrollZoom = 1.0;
	double VScrollZoom = 1.0;

	UINT* SampleRate = NULL;

	CFFT* fft = NULL;
	BUFF_DATA_TYPE buff_type = BUFF_DATA_TYPE::short_type;
	void*	DataBuff;
	UINT*	DataBuffPos = NULL;
	UINT	data_buff_data_bits = 16;
	UINT	data_buff_length_mask;

	UINT FFTSize = 8192;
	UINT HalfFFTSize = FFTSize / 2;
	UINT FFTStep = 8192;
	UINT average_Deep = FFT_DEEP;

	double DBMin = -24.0;

	UINT FFTHeight = 512;

	HANDLE  hDrawMutex;
	INT		SpectrumY = -1;
	HDC		hDCFFT = NULL;
	HBITMAP hBMPFFT = NULL;
	bool isSpectrumZoomedShow = false;
	bool isDrawLogSpectrum = false;

public:
	CFFTWin();
	~CFFTWin();

	void Init(void);
	void UnInit(void);

	void RegisterWindowsClass(void);
	void Paint(void);
	bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
	void OnMouse(void);
	void GetRealClientRect(PRECT lprc);
	void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

	void PaintFFT(HDC hdc, CFFT* fft);
	void PaintFFTBriefly(HDC hdc, CFFT* fft);

	void BrieflyBuff(void);
	void PaintSpectrum(CFFT* fft);
	void SpectrumToWin(HDC hDC);
	void InitDrawBuff(void);

	void RestoreValue(void);
	void SaveValue(void);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK DlgFFTSetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};
