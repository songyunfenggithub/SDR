
#pragma once

namespace WINS {

	namespace SDR_SET {

#define WIN_CONTROLS_CLASS		"WIN_CONTROLS_CLASS"

#define FONT_SIZE				14
#define PARAMS_ITEM_HEIGHT		23
#define PARAMS_ITEM_CONTROL_POSTION		300
#define PARAMS_ITEM_CONTROL_WIDTH		200

		class CWinControls
		{
		public:

			HWND hWnd = NULL;
			RECT WinRect = { 0 };

			HWND hWndControls[100] = { 0 };
			HWND hWndControlsDraw[100] = { 0 };

		public:
			CWinControls();
			~CWinControls();

			void RegisterWindowsClass(void);
			void OpenWindow(void);

			void Paint(void);
			bool DoNotify(UINT msg, WPARAM wParam, LPARAM lParam);
			bool OnCommand(UINT message, WPARAM wParam, LPARAM lParam);

			void CheckApply(UINT index);
			void SDR_params_apply(UINT index);

			static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		};
	}
}