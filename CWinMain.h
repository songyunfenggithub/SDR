// CWinMain.h: interface for the CWinMain class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WINMAIN_H__67392A91_423D_41F4_96FA_D01F57838552__INCLUDED_)
#define AFX_WINMAIN_H__67392A91_423D_41F4_96FA_D01F57838552__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_LOADSTRING 100

class CAudioWin;

class CWinMain  
{
public:
	HWND		hWnd;
	HMENU		hMyMenu;
	TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
	TCHAR szWindowClass[MAX_LOADSTRING];						// The title bar text

	CAudioWin* m_audioWin = NULL;

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

} DRAWINFO, *PDRAWINFO;

	DRAWINFO	DrawInfo;

public:
	CWinMain();
	~CWinMain();

	BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
	ATOM MyRegisterClass(HINSTANCE hInstance);
	VOID Paint(HWND hWnd);
	VOID CaculateHScroll(void);
	VOID CaculateVScroll(void);
	VOID GetRealClientRect (HWND hwnd, PRECT lprc);
	VOID SetScrollRanges(HWND hwnd);
	BOOL OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	VOID KeyAndScroll(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProcReal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void SaveDefaultValue(void);
	void RestoreDefaultValue(void);

	static LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

};

extern CWinMain	clsWinMain;
#endif // !defined(AFX_WINMAIN_H__67392A91_423D_41F4_96FA_D01F57838552__INCLUDED_)
