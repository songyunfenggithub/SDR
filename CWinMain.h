
#pragma once

class CData;

namespace WINS {

#define MAX_LOADSTRING 100

	class CWinAudio;
	class CWinFiltted;
	class CWinFilter;
	class CWinSignal;

	class CWinMain
	{
	public:
		HWND hWnd = NULL;
		RECT WinRect = { 0 };

		HMENU hMenu = NULL;
		HMENU hMenuFollow = NULL;

		HWND hWndRebar = NULL;
		HWND hWndToolbar = NULL;
		RECT RebarRect = { 0 };

		CWinAudio* m_audioWin = NULL;
		CWinFiltted* m_filttedWin = NULL;
		CWinFilter* m_FilterWin = NULL;

		CWinSignal* m_signalWin = NULL;

		UINT uTimerId;

	public:
		CWinMain();
		~CWinMain();

		BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
		ATOM RegisterClass(HINSTANCE hInstance);
		BOOL OnCommand(UINT message, WPARAM wParam, LPARAM lParam);

		HWND MakeToolsBar(void);

		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

		void FilterStart(void);
		void FilterStop(void);
		static LPTHREAD_START_ROUTINE CWinMain::FilterStopThread(LPVOID lp);

	};
}
extern WINS::CWinMain clsWinMain;

