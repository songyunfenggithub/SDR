// File.cpp: implementation of the CFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include <windows.h>
#include <winuser.h>
#include <commdlg.h>
#include <stdio.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <math.h>


#include "CFile.h"
#include "CWinMain.h"
#include "CSoundCard.h"
#include "MyDebug.h"

#include "CData.h"

CFile		clsFile;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFile::CFile()
{
	// Fill in the ofn structure to support a template and hook.
    ZeroMemory( &ofn, sizeof( OPENFILENAME ) );
	ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.lpstrFilter       = szFilter;
//    ofn.lpstrCustomFilter = NULL;
//    ofn.nMaxCustFilter    = 0;
//    ofn.nFilterIndex      = 0;
//    ofn.lpstrFileTitle    = NULL;
	ofn.nMaxFile          = MAX_PATH;
//    ofn.nMaxFileTitle     = 0;
//    ofn.lpstrInitialDir   = NULL;
//    ofn.nFileOffset       = 0;
//    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = "wvd";
//    ofn.lCustData         = NULL;//(LPARAM)&sMyData;
//	ofn.lpfnHook 		   = NULL;//ComDlg32DlgProc;
//	ofn.lpTemplateName    = NULL;//MAKEINTRESOURCE(IDD_COMDLG32);
//    ofn.Flags             = OFN_CREATEPROMPT | 
//									 OFN_READONLY | 
//									 OFN_FILEMUSTEXIST | 
//									 OFN_PATHMUSTEXIST | 
//									 OFN_SHOWHELP;//OFN_SHOWHELP | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
	SetFilter("Wave Data Files (*.sound)\0*.sound\0All Files (*.*)\0*.*\0\0");
	*szFile = '\0';
	*szFileAs = '\0';
	dwSaveEndPos = dwSaveStartPos = 0;
	hWndGetPos = NULL;
}

CFile::~CFile()
{

}

BOOL CFile::NewFile(VOID)
{
	clsSoundCard.outBufLength = 0;
	SetWindowText(clsWinMain.hWnd, clsWinMain.szTitle);
	clsWinMain.CaculateHScroll();
	InvalidateRect(clsWinMain.hWnd,NULL,TRUE);
	return TRUE;
}

BOOL CFile::OpenWaveFile(VOID)
{
	if(clsFile.GetOpenFile(clsWinMain.hWnd))
	{
		DWORD  NumberOfBytesReaded;
		HANDLE hFile = CreateFile ( szFile,
									GENERIC_READ,
									0,
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			return FALSE;
		}
		else
		{
			TCHAR info[2];
			ReadFile(hFile, info, 2, &NumberOfBytesReaded, NULL);
			ReadFile(hFile, &clsSoundCard.FormatEx, sizeof(WAVEFORMATEX), &NumberOfBytesReaded, NULL);
			ReadFile(hFile, &clsSoundCard.outBufLength, 4, &NumberOfBytesReaded, NULL);
			ReadFile(hFile, clsSoundCard.outBuffer, clsSoundCard.outBufLength, &NumberOfBytesReaded, NULL);
			CloseHandle(hFile);
			TCHAR sz[1024];
			int i;
			for(i = strlen(szFile); szFile[i] != '\\'; i--);
			sprintf(sz, "%s - %s", clsWinMain.szTitle, szFile + i + 1); 
			SetWindowText(clsWinMain.hWnd, sz);
		}
		clsWinMain.CaculateHScroll();
		InvalidateRect(clsWinMain.hWnd,NULL,TRUE);
		return TRUE;
	}
	return FALSE;
}

BOOL CFile::SaveFileAs(VOID)
{
	if(!clsFile.GetSaveFile(clsWinMain.hWnd,TRUE))return FALSE;
	strcpy(szFile, szFileAs);
	BOOL f;
	if(f = SaveFile())
	{
		TCHAR sz[1024];
		int i;
		for(i = strlen(szFile); szFile[i] != '\\'; i--);
		sprintf(sz, "%s - %s", clsWinMain.szTitle, szFile + i + 1); 
		SetWindowText(clsWinMain.hWnd, sz);
	}
	return f;
}


BOOL CFile::SaveFile(VOID)
{
	DWORD  NumberOfBytesWritten;
	
	if(clsFile.szFile[0] == '\0')if(!clsFile.GetSaveFile(clsWinMain.hWnd,FALSE))return FALSE;

	if(!clsFile.dwSaveEndPos)clsFile.dwSaveEndPos = clsSoundCard.outBufLength;

	HANDLE hFile = CreateFile ( szFile,
									GENERIC_WRITE,
									0,
									NULL,
									CREATE_ALWAYS,
									FILE_ATTRIBUTE_NORMAL,
									NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		DbgMsg("Failed to create file :%d", GetLastError());
		return FALSE;
	}
	else
	{
		DWORD	dwLen = dwSaveEndPos - dwSaveStartPos;
		WriteFile(hFile, "wa", 2, &NumberOfBytesWritten, NULL);
		WriteFile(hFile, &clsSoundCard.FormatEx, sizeof(WAVEFORMATEX), &NumberOfBytesWritten, NULL);
		WriteFile(hFile, &dwLen, 8, &NumberOfBytesWritten, NULL);
		WriteFile(hFile, clsSoundCard.outBuffer + dwSaveStartPos, 
			dwSaveEndPos - dwSaveStartPos, &NumberOfBytesWritten, NULL);
		CloseHandle( hFile );
	}
	return TRUE;
}

BOOL CFile::SaveBuffToFile(VOID)
{
	DWORD  NumberOfBytesWritten;

	if (clsFile.szFile[0] == '\0')if (!clsFile.GetSaveFile(clsWinMain.hWnd, FALSE))return FALSE;

	if (clsFile.dwSaveEndPos == 0) clsFile.dwSaveEndPos = 50000000;

	HANDLE hFile = CreateFile(szFile,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		DbgMsg("Failed to create file :%d", GetLastError());
		return FALSE;
	}
	else
	{
		DWORD dwLen = (dwSaveEndPos - dwSaveStartPos) * sizeof(ADCDATATYPE);
		//WriteFile(hFile, "wa", 2, &NumberOfBytesWritten, NULL);
		//WriteFile(hFile, &clsSoundCard.FormatEx, sizeof(WAVEFORMATEX), &NumberOfBytesWritten, NULL);
		//WriteFile(hFile, &dwLen, 8, &NumberOfBytesWritten, NULL);
		WriteFile(hFile, (char*)(clsData.AdcBuff + dwSaveStartPos), dwLen, &NumberOfBytesWritten, NULL);
		CloseHandle(hFile);
	}
	return TRUE;
}

LRESULT CALLBACK CFile::DlgSaveLengthProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemInt(hDlg, IDC_EDITSAVESTART, 
				clsFile.dwSaveStartPos > clsSoundCard.outBufLength? 0 : clsFile.dwSaveStartPos, FALSE);
			SetDlgItemInt(hDlg, IDC_EDITSAVEEND, 
				clsFile.dwSaveEndPos > clsSoundCard.outBufLength? clsSoundCard.outBufLength : clsFile.dwSaveEndPos, FALSE);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDC_CHECK_USE_BUF_LEN:
				if (HIWORD(wParam) == BN_CLICKED) 
                {
					if(IsDlgButtonChecked(hDlg,IDC_CHECK_USE_BUF_LEN))
					{
						 EnableWindow(GetDlgItem(hDlg,IDC_EDITSAVESTART),FALSE);
						 EnableWindow(GetDlgItem(hDlg,IDC_EDITSAVEEND),FALSE);
					} else {
						 EnableWindow(GetDlgItem(hDlg,IDC_EDITSAVESTART),TRUE);
						 EnableWindow(GetDlgItem(hDlg,IDC_EDITSAVEEND),TRUE);
					}
				}
				break;
			case IDC_STATIC_START_POS:
				if (HIWORD(wParam) == STN_CLICKED) 
                {
					SetDlgItemInt(hDlg,	IDC_EDITSAVESTART, 
						clsWinMain.DrawInfo.iHZoom > 0 ? 
						clsWinMain.DrawInfo.dwHZoomedPos >> clsWinMain.DrawInfo.iHZoom
						: 
						clsWinMain.DrawInfo.dwHZoomedPos << -clsWinMain.DrawInfo.iHZoom
						, TRUE);
				}
				break;
			case IDC_STATIC_END_POS:
				if (HIWORD(wParam) == STN_CLICKED) 
                {
					SetDlgItemInt(hDlg,	IDC_EDITSAVEEND, 
						clsWinMain.DrawInfo.iHZoom > 0 ? 
						clsWinMain.DrawInfo.dwHZoomedPos >> clsWinMain.DrawInfo.iHZoom
						: 
						clsWinMain.DrawInfo.dwHZoomedPos << -clsWinMain.DrawInfo.iHZoom
						, TRUE);
				}
				break;
			case IDOK:
				if(IsDlgButtonChecked(hDlg,IDC_CHECK_USE_BUF_LEN))
				{
					clsFile.dwSaveStartPos = 0;
					clsFile.dwSaveEndPos = clsSoundCard.outBufLength;
				} else {
					clsFile.dwSaveStartPos	= GetDlgItemInt(hDlg, IDC_EDITSAVESTART, 0, 0);
					clsFile.dwSaveEndPos	= GetDlgItemInt(hDlg, IDC_EDITSAVEEND, 0, 0);
					if(clsFile.dwSaveEndPos > clsSoundCard.outBufLength)
						clsFile.dwSaveEndPos = clsSoundCard.outBufLength;
					if(clsFile.dwSaveStartPos > clsSoundCard.outBufLength ||
						clsFile.dwSaveStartPos > clsFile.dwSaveEndPos)
					{
						MessageBox(hDlg,"Cut Postion Error","Cut Position Error",IDOK);
						break;
					}
				}
			case IDCANCEL:
				EndDialog(hDlg, LOWORD(wParam));
				clsFile.hWndGetPos = NULL;
				return TRUE;
			}
			break;
	}
    return FALSE;
}

VOID CFile::SetFilter(LPTSTR szFilter)
{
	ofn.lpstrFilter = szFilter;
}

BOOL CFile::GetOpenFile(HWND hWnd)
{

	ofn.hwndOwner   = hWnd;
    ofn.hInstance   = (HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE);
 	ofn.lpstrTitle  = "Open";
 	ofn.lpstrFile   = szFile;
    ofn.Flags       = OFN_CREATEPROMPT
					| OFN_READONLY
					| OFN_FILEMUSTEXIST
					| OFN_PATHMUSTEXIST
					| OFN_SHOWHELP
					;
	// OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;

	szFile[0]='\0';

    if (!GetOpenFileName(&ofn))
    {
		return false;
	}
	return true;
	
}

BOOL CFile::GetSaveFile(HWND hWnd, const BOOL fSaveAs)
{
	ofn.hwndOwner	= hWnd;
    ofn.hInstance	= (HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE);
	ofn.lpstrTitle	= fSaveAs ? "Save as" : "Save";
	ofn.lpstrFile	= fSaveAs ? szFileAs : szFile;
	ofn.Flags		= OFN_OVERWRITEPROMPT 
					| OFN_CREATEPROMPT
					| OFN_PATHMUSTEXIST
					| OFN_SHOWHELP
					;
    *szFile = 0;
    if (!GetSaveFileName(&ofn))
    {
		return false;
	}
	return true;
}

