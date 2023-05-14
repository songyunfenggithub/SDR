#pragma once
#include "stdafx.h"
#include "stdint.h"

#include <stdlib.h>
#include <string>

#include <commctrl.h>

namespace WINS {

#define WIN_TOOLS_CLASS		"WIN_TOOLS_CLASS"

#define BUTTON_PLAY_STOP_ID			0
#define BUTTON_FILTER_BUILD_ID		1

	class CWinTools
	{
	public:
		typedef struct TOOL_TIPS_STRUCT {
			UINT id;
			LPSTR text;
		} TOOL_TIPS;

	public:
		CWinTools();
		~CWinTools();

		void Init(void);
		void UnInit(void);
		
		HWND CreateToolbar(HWND hWnd, LPTBBUTTON tbb, UINT numButtons, TOOL_TIPS* tips, UINT numTips);
		HWND CreateComboBox(HWND hWnd);
		HWND CreateRebar(HWND hWnd);
		void CreateRebarBand(HWND hWndReBar, LPSTR title, UINT ID, UINT cx, UINT iImage, HWND hWndChild);

		LRESULT RelabelButton(HWND hWndToolbar, UINT id, LPSTR text);
		BOOL DoNotify(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HWND MakeReBar(HWND hWnd);
	};
}

extern WINS::CWinTools clsWinTools;
