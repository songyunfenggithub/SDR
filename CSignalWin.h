#pragma once
#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

#include "CMessage.h"
#include "CScreenButton.h"

#define SIGNAL_WIN_CLASS		"SIGNAL_WIN_CLASS"

#define SIGNAL_LENGTH		0x10000
#define SIGNAL_LENGTH_MASK	(SIGNAL_LENGTH - 1)

#define SIGNAL_DATA_MAX_ZOOM_BIT				5

class CSignalWin
{
public:
	typedef struct tagDRAWINFO
	{

		int			iHZoom, iHOldZoom, iVZoom, iVOldZoom, iHFit, iVFit;
		INT64		dwHZoomedWidth, dwHZoomedPos, dwVZoomedHeight, dwVZoomedPos, dwVZoomedFullHeight;
		UINT16		wHSclPos, wVSclPos, wHSclMin, wHSclMax, wVSclMin, wVSclMax;

		double		dbVZoom;
		double		dbHZoom;
		double		FullVotage;
		double		VotagePerDIV;

		INT		DrawHeight, DrawWidth;

	} DRAWINFO, * PDRAWINFO;

	DRAWINFO	DrawInfo;

	HWND hWnd = NULL;

	UINT uTimerId;

	void* OrignalBuff;
	UINT* OrignalBuffPos;
	BUFF_DATA_TYPE	orignal_buff_type;

	void* FilttedBuff;
	UINT* FilttedBuffPos;
	BUFF_DATA_TYPE	filtted_buff_type;

	UINT		BuffDataBit;
	UINT64		DataLength;
	UINT		DataLengthMask;

	bool bDrawOrignalSignal = true;
	bool bDrawFilttedSignal = true;
	bool bFollowByOrignal = true;
	bool bAutoScroll = true;

	INT MouseX;
	INT MouseY;

	RECT WinRect;

	char strMouse[1024];

	CMessage msgs;

public:
	CSignalWin();
	~CSignalWin();

	void Init(UINT data_bit, UINT data_length);
	void UnInit(void);

	void RegisterWindowsClass(void);
	void Paint(void);
	bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
	void OnMouse(void);
	void GetRealClientRect(PRECT lprc);

	void DrawSignal_short(HDC hdc, RECT* rt, short *Buff, UINT *BuffPos, COLORREF Color);
	void DrawSignal_float(HDC hdc, RECT* rt, float* Buff, UINT* BuffPos, COLORREF Color);


	void CaculateScrolls(void);
	void CaculateHScroll(void);
	void CaculateVScroll(void);
	void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

	void SaveValue(void);
	void RestoreValue(void);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};
