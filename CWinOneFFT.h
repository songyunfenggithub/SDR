
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


	INT VScrollPos = 0, VScrollWidth = 0;
	double VScrollZoom = 1.0;

	HWND hWnd = NULL;
	RECT WinRect;

	bool FFTNeedReDraw = true;
	BOOL bFFTHold = false;

	BOOL bFFTOrignalShow = true;
	BOOL bFFTOrignalLogShow = true;
	BOOL bFFTFilttedShow = true;
	BOOL bFFTFilttedLogShow = true;

	double averageOrignal, averageOrignalLog, averageFiltted, averageFilttedLog;
	double minOrignal, minOrignalLog, minFiltted, minFilttedLog;
	double maxOrignal, maxOrignalLog, maxFiltted, maxFilttedLog;

	char strMouse[1024];

	CMessage msgs;

	bool readyForInitBuff = false;

	CScreenButton* rfButton;
	CScreenButton* rfStepButton;

	CScreenButton* fsButton;
	CScreenButton* decimationFactorButton;
	CScreenButton* BWButton;
	CScreenButton* IFModeButton;
	CScreenButton* LOModeButton;
	CScreenButton* LOcModeButton;
	CScreenButton* averageFilterButton;
	CScreenButton* confirmP1RfGoButton;

	POINT ScreenP1 = { 0 }, ScreenP2 = { 0 };
	bool P1_Use = false, P2_Use = false;
	char strPP[1024];

public:
	CWinOneFFT();
	~CWinOneFFT();

	void RegisterWindowsClass(void);
	void OpenWindow(void);

	void Paint(void);
	bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
	void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);
	void OnMouse(void);
	void GetRealClientRect(PRECT lprc);
	void BrieflyBuff(WHICHSIGNAL which);
	void PaintBriefly(HDC hdc);
	void DrawPoint(HDC hdc, POINT* P, HFONT hFont);
	void P2SubP1(void);

	void SaveValue(void);
	void RestoreValue(void);

	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static void averageFilterButtonFunc(CScreenButton* button);
	static void confirmP1RfGoButtonFunc(CScreenButton* button);
};

extern CWinOneFFT clsWinOneFFT;
