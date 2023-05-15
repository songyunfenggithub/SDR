#pragma once

namespace WINS {

#define WIN_MSG_CLASS		"WIN_MSG_CLASS"

#define MSG_MAX_LENGTH	0x10000
#define MSG_MAX_MASK	(MSG_MAX_LENGTH - 1)

	class CWinMsg
	{
	public:
		HWND hWnd = NULL;
		RECT WinRect = { 0 };
		UINT ScreenMaxLines = 0;

		UINT uTimerId = 0;

		HANDLE hMutex = NULL;
		char* Msgs[MSG_MAX_LENGTH] = { 0 };
		UINT MsgPos = 0;
		
		bool bLogToFile = false;

		INT VScrollPos = 0;
		INT VScrollRang = 0;
		INT HScrollPos = 0;
		INT HScrollRang = 0;
		bool bAutoScroll = true;

	public:
		CWinMsg();
		~CWinMsg();

		void RegisterWindowsClass(void);
		void OpenWindow(void);

		void Paint(void);
		void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);

		void SetMsg(char* msg);

		bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);

		bool DoNotify(UINT msg, WPARAM wParam, LPARAM lParam);

		void RestoreValue(void);
		void SaveValue(void);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};
}

extern WINS::CWinMsg clsWinMsg;
