// MyDebuger.cpp: implementation of the MyDebuger class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdio.h>

#include "Debug.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void myMsgBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
	char info[256];
	MessageBox(hWnd,lpText,lpCaption,uType);
	sprintf(info, "[title] %s [text]:%s", lpCaption, lpText);
	OutputDebugString(info);
}
 

void myDbgMsg (PSTR sz,...)
{
    CHAR ach[1024];
    va_list args;

    va_start(args, sz);

//    wvsprintf (ach, sz, args);   /* Format the string */
    vsprintf (ach, sz, args);   /* Format the string */
	OutputDebugString(ach);
	//printf(ach);
//    MessageBox (NULL, ach, NULL, MB_OK|MB_ICONEXCLAMATION|MB_APPLMODAL);
}

VOID mydebugpoint()
{
	static int i=0;
	int b;
	if(i++ == 100000)
		b=0;
}

VOID myDebug01(DWORD I,PTCHAR szInfo)
{
	TCHAR s[256];

	int i,n;
	for(n = i = 0; i < 32; i++,n++)
	{
		s[n] = (char)*(I&(1<<(32-i-1))?"1":"0");
		if(!((i+1)%4))s[++n] = (char)*",";
	}
	sprintf(&s[n]," %s",szInfo);
	OutputDebugString(s);
}

