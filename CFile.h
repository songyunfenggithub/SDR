
#pragma once

class CFile  
{
public:
	TCHAR			szFile[512];
	TCHAR			szFileAs[512];
	HWND			hWndGetPos;

private:	
	TCHAR			szFilter[512];
	OPENFILENAME	ofn;
	DWORD			dwSaveStartPos, dwSaveEndPos;

public:
	CFile();
	virtual ~CFile();

	BOOL NewFile(VOID);
	BOOL OpenWaveFile(VOID);
	BOOL SaveFile(VOID);
	BOOL SaveFileAs(VOID);
	
	BOOL SaveBuffToFile(VOID);

	BOOL GetSaveFile(HWND hWnd, const BOOL fSaveAs);
	BOOL GetOpenFile(HWND hWnd);
	VOID SetFilter(LPTSTR szFilter);

	static LRESULT CALLBACK DlgSaveLengthProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

};

extern CFile	clsFile;
