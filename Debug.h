
#pragma once

#if defined(_DEBUG)

void myMsgBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);
void myDbgMsg (PSTR sz,...);
void myDbgMultiMsg(PSTR sz, ...);
VOID mydebugpoint();
VOID myDebug01(DWORD I,PTCHAR szInfo);

#define MsgBox(hWnd,lpText,lpCaption,uType) myMsgBox(hWnd,lpText,lpCaption,uType)
//#define DbgMsg(x) myDbgMsg(x)
#define DbgMsg(...)			myDbgMsg(__VA_ARGS__)
#define DbgMultiMsg(...)	myDbgMultiMsg(__VA_ARGS__)
//#define DbgMsg(__x__) myDbgMsg __x__
#define debugpoint() mydebugpoint()
#define Dbg01(x,s) myDebug01(x,s)

#else

#define MsgBox(hWnd,lpText,lpCaption,uType)
#define DbgMsg(x)
#define DbgMultiMsg(...)
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
