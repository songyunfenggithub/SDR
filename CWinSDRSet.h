#pragma once

namespace METHOD {
	class CFFT;
}
using namespace METHOD;


namespace WINS {

	namespace SDR_SET {

#define WIN_SDR_SET_CLASS		"WIN_SDR_SET_CLASS"

		class CWinControls;

		class CWinSDRSet
		{
		public:

			HWND hWnd = NULL;
			RECT WinRect = { 0 };
			RECT WinSetRt = { 0 };

			CWinSDRSet* SDRSets[100] = { 0 };
			UINT WinCount = 0;

			INT VScrollPos = 0;
			UINT VScrollRang = 0;
			INT HScrollPos = 0;
			UINT HScrollRang = 0;
			UINT index = 0;

			CWinControls* m_WinControls = NULL;

		public:
			CWinSDRSet();
			~CWinSDRSet();

			void RegisterWindowsClass(void);
			void OpenWindow(void);

			void Paint(void);
			bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
			void KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);
			bool DoNotify(UINT msg, WPARAM wParam, LPARAM lParam);

			void buildControls(void);

			CWinSDRSet* buildSets(CWinSDRSet* winSet);
			void DrawSets(void);

			void RestoreValue(void);
			void SaveValue(void);

			static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		};
	}
}

extern WINS::SDR_SET::CWinSDRSet clsWinSDRSet;
