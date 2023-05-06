
#pragma once

namespace WINS {

#define MAX_LOADSTRING 100

	class CAudioWin;
	class CFilttedWin;
	class CFilterWin;

	class CWinMain
	{
	public:
		HWND hWnd = NULL;
		HMENU hMenu = NULL;
		TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
		TCHAR szWindowClass[MAX_LOADSTRING];						// The title bar text

		CAudioWin* m_audioWin = NULL;
		CFilttedWin* m_filttedWin = NULL;
		CFilterWin* m_FilterWin = NULL;

		typedef struct tagDRAWINFO
		{

			int			iHZoom, iHOldZoom, iVZoom, iVOldZoom, iHFit, iVFit;
			UINT64		dwDataWidth, dwDataHeight;
			INT64		dwHZoomedWidth, dwHZoomedPos, dwVZoomedHeight, dwVZoomedPos;
			UINT16		wHSclPos, wVSclPos, wHSclMin, wHSclMax, wVSclMin, wVSclMax;

			double		dbVZoom;
			double		dbHZoom;
			double		FullVotage;
			double		VotagePerDIV;
			BOOL		fAutoScroll;
			UINT		uTimerId;

			BOOL		bOrignalSignalShow, bFilttedSignalShow;

			bool		fFollowByOrignal = true;

		} DRAWINFO, * PDRAWINFO;

		DRAWINFO	DrawInfo;

	public:
		CWinMain();
		~CWinMain();

		BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
		ATOM MyRegisterClass(HINSTANCE hInstance);
		VOID Paint(void);
		VOID CaculateHScroll(void);
		VOID CaculateVScroll(void);
		VOID GetRealClientRect(PRECT lprc);
		BOOL OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
		VOID KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);
		void SaveValue(void);
		void RestoreValue(void);

		LRESULT CALLBACK WndProcReal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	};
}
extern WINS::CWinMain clsWinMain;

