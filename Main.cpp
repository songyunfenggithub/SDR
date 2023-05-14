// myWave.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"

//#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <commdlg.h>

#include "public.h"
#include "CWinMain.h"

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	hInst = hInstance;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, clsWinMain.szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_MYWAVE, clsWinMain.szWindowClass, MAX_LOADSTRING);
	clsWinMain.RegisterClass(hInstance);

	// Perform application initialization:
	if (!clsWinMain.InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_MYWAVE);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

