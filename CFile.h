// File.h: interface for the CFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE_H__0E827EB8_426F_4C0A_8A41_9A0ABF870FDA__INCLUDED_)
#define AFX_FILE_H__0E827EB8_426F_4C0A_8A41_9A0ABF870FDA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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
	
	BOOL GetSaveFile(HWND hWnd, const BOOL fSaveAs);
	BOOL GetOpenFile(HWND hWnd);
	VOID SetFilter(LPTSTR szFilter);

	static LRESULT CALLBACK DlgSaveLengthProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

};

extern CFile	clsFile;

#endif // !defined(AFX_FILE_H__0E827EB8_426F_4C0A_8A41_9A0ABF870FDA__INCLUDED_)
