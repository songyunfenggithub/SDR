// MyDebuger.h: interface for the MyDebuger class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MYDEBUGER_H__1A4E8B35_45DE_49DE_96B4_3DABF019A5D7__INCLUDED_)
#define AFX_MYDEBUGER_H__1A4E8B35_45DE_49DE_96B4_3DABF019A5D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#if defined(_DEBUG)

void myMsgBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);
void myDbgMsg (PSTR sz,...);
VOID mydebugpoint();
VOID myDebug01(DWORD I,PTCHAR szInfo);

#define MsgBox(hWnd,lpText,lpCaption,uType) myMsgBox(hWnd,lpText,lpCaption,uType)
#define DbgMsg(x) myDbgMsg(x)
#define debugpoint() mydebugpoint()
#define Dbg01(x,s) myDebug01(x,s)

#else

#define MsgBox(hWnd,lpText,lpCaption,uType)
#define DbgMsg(x)
#define Dbg01(x,s)
#define debugpoint()
/*
#define DbgMsg(a,b)
#define DbgMsg(a,b,c)
#define DbgMsg(a,b,,c,d)
#define debug(format, ...) fprintf(stderr, fmt, __VA_ARGS__)  
#define debug(format, args...) fprintf (stderr, format, args)  
#define debug(format, ...) fprintf (stderr, format, ## __VA_ARGS__)  
*/
#endif

#endif // !defined(AFX_MYDEBUGER_H__1A4E8B35_45DE_49DE_96B4_3DABF019A5D7__INCLUDED_)
