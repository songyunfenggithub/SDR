
#pragma once

class CData;

namespace WINS {

#define MAX_LOADSTRING 100

	class CWinAudio;
	class CWinFiltted;
	class CWinFilter;

	class CWinMain
	{
	public:
		HWND hWnd = NULL;
		HMENU hMenu = NULL;
		TCHAR szTitle[MAX_LOADSTRING];								// The title bar text

		HWND hWndRebar = NULL;
		RECT RebarRect = { 0 };

		CWinAudio* m_audioWin = NULL;
		CWinFiltted* m_filttedWin = NULL;
		CWinFilter* m_FilterWin = NULL;

		typedef struct DRAWINFO_STRUCT {
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
		ATOM RegisterClass(HINSTANCE hInstance);
		VOID Paint(void);
		VOID CaculateHScroll(void);
		VOID CaculateVScroll(void);
		VOID GetRealClientRect(PRECT lprc);
		BOOL OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
		VOID KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam);
		void SaveValue(void);
		void RestoreValue(void);

		HWND MakeToolsBar(void);

		void DrawSignal_short(CData* cData, UINT pos, UINT pen, HDC hdc, UINT dwXOffSet, UINT bufStep, UINT dwWStep, RECT* rt);
		void DrawSignal_float(CData* cData, UINT pos, UINT pen, HDC hdc, UINT dwXOffSet, UINT bufStep, UINT dwWStep, RECT* rt);

		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	};
}
extern WINS::CWinMain clsWinMain;

