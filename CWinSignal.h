#pragma once
#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

class CMessage;

namespace WINS {

#define WIN_SIGNAL_CLASS		"WIN_SIGNAL_CLASS"

#define SIGNAL_LENGTH		0x10000
#define SIGNAL_LENGTH_MASK	(SIGNAL_LENGTH - 1)

#define SIGNAL_DATA_MAX_ZOOM_BIT				5

	class CWinSignal
	{
	public:
		typedef struct DRAWINFO_STRUCT {
			INT			iHZoom, iHOldZoom, iVZoom, iVOldZoom, iHFit, iVFit;
			INT64		dwHZoomedWidth, dwHZoomedPos, dwVZoomedHeight, dwVZoomedPos, dwVZoomedFullHeight;
			UINT16		wHSclPos, wVSclPos, wHSclMin, wHSclMax, wVSclMin, wVSclMax;
			double		dbVZoom, dbHZoom, FullVotage, VotagePerDIV;
			INT			DrawHeight, DrawWidth;
		} DRAWINFO, * PDRAWINFO;
		DRAWINFO	DrawInfo;


		const char* Tag = NULL;

		HWND hWnd = NULL;
		HMENU hMenu = NULL;

		UINT uTimerId;

		void* DataOrignal = NULL;
		void* DataFiltted = NULL;

		bool bDrawOrignalSignal = true;
		bool bDrawFilttedSignal = true;
		bool bFollowByOrignal = true;
		bool bAutoScroll = true;

		INT MouseX;
		INT MouseY;

		RECT WinRect;

		char strMouse[1024];

		CMessage *msgs;

	public:
		CWinSignal();
		~CWinSignal();

		void Init(void);
		void UnInit(void);

		void RegisterWindowsClass(void);
		void Paint(void);
		bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
		void OnMouse(void);
		void GetRealClientRect(PRECT lprc);

		void DrawSignal_short(HDC hdc, RECT* rt, void* Data, COLORREF Color);
		void DrawSignal_float(HDC hdc, RECT* rt, void* Data, COLORREF Color);


		void CaculateScrolls(void);
		void CaculateHScroll(void);
		void CaculateVScroll(void);
		void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

		void SaveValue(void);
		void RestoreValue(void);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};
}
