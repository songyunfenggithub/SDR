
#pragma once

class CFile  
{
public:
	TCHAR			szFile[512] = { 0 };
	TCHAR			szFileAs[512] = { 0 };
	HWND			hWndGetPos = NULL;

private:	
	TCHAR			szFilter[512] = { 0 };
	OPENFILENAME	ofn = { 0 };
	DWORD			dwSaveStartPos = 0, dwSaveEndPos = 0;

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

	static LRESULT CALLBACK DlgSaveLengthProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

};

extern CFile clsFile;
