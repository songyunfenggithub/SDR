
#pragma once

#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

#include "CMessage.h"
#include "CScreenButton.h"

#define FFT_ONE_WIN_CLASS		"FFT_ONE_WIN"

class CWinOneFFT
{
public:

	UINT	uTimerId;

	UINT MouseX;
	UINT MouseY;
	double Hz = 0.0;

	UINT WinWidth;

	HWND hWnd = NULL;

	double* pCore = NULL;
	int CoreLength;

	bool FFTNeedReDraw = true;
	BOOL bFFTHold = false;

	BOOL bFFTOrignalShow = true;
	BOOL bFFTOrignalLogShow = true;
	BOOL bFFTFilttedShow = true;
	BOOL bFFTFilttedLogShow = true;

	char strMouse[1024];

	CMessage msgs;

	bool readyForInitBuff = false;

	CScreenButton::BUTTON* rfButton;
	CScreenButton::BUTTON* rfStepButton;

public:
	CWinOneFFT();
	~CWinOneFFT();

	void RegisterWindowsClass(void);
	void OpenWindow(void);

	void Paint(HWND hWnd);
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnMouse(HWND hWnd);
	void GetRealClientRect(HWND hWnd, PRECT lprc);
	VOID BrieflyBuff(WHICHSIGNAL which);
	void PaintBriefly(HDC hdc);

	void rfButtonInit(CScreenButton::BUTTON* pButton);
	void rfButtonRefresh(CScreenButton::BUTTON* pButton, INT64 value);
	static void rfButtonOnMouse(CScreenButton::BUTTON* pButton, UINT message, WPARAM wParam, LPARAM lParam);
	static void rfButtonDraw(CScreenButton::BUTTON* pButton, HDC hdc, RECT* srcRt);

	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

extern CWinOneFFT clsWinOneFFT;
