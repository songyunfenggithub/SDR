
#include "stdafx.h"
#include "resource.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "stdio.h"
#include <ctime>

#include "Public.h"
#include "Debug.h"

#include "CWinMsg.h"

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define FONT_SIZE 14
#define TIMEOUT 100

using namespace std;
using namespace WINS;

CWinMsg clsWinMsg;

FILE* logfile = NULL;

CWinMsg::CWinMsg()
{
	DeleteFile("debug.log");

	hMutex = CreateMutex(NULL, false, "CWinMsghMutex");
	RegisterWindowsClass();
}

CWinMsg::~CWinMsg()
{

}

void CWinMsg::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinMsg::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = WIN_MSG_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinMsg::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(WIN_MSG_CLASS, "调试输出窗口", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 600, 600, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

bool CWinMsg::DoNotify(UINT msg, WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

void CWinMsg::SetMsg(char* msg)
{
	int n = strlen(msg) + 1;
	char* str = new char[n];
	strcpy(str, msg);

	WaitForSingleObject(hMutex, INFINITE);

	if (bLogToFile) {
		logfile = fopen("debug.log", "a");
		//time_t now = time(0);
		//fprintf(logfile, ctime(&now));
		//fprintf(logfile, str);
		//fputs(ctime(&now), logfile);
		fputs(str, logfile);
		fclose(logfile);
	}

	Msgs[MsgPos++] = str;
	MsgPos &= MSG_MAX_MASK;

	VScrollRang = MsgPos - ScreenMaxLines;
	UP_TO_ZERO(VScrollRang);
	if (bAutoScroll) VScrollPos = BOUND((MsgPos - ScreenMaxLines) & MSG_MAX_MASK, 0, VScrollRang);

	ReleaseMutex(hMutex);

	SetScrollRange(hWnd, SB_VERT, 0, VScrollRang, false);
	SetScrollPos(hWnd, SB_VERT, VScrollPos, true);
}

LRESULT CALLBACK CWinMsg::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CWinMsg* me = (CWinMsg*)get_WinClass(hWnd);

	switch (message)
	{
	case WM_CREATE:
		me = (CWinMsg*)set_WinClass(hWnd, lParam);
		me->hWnd = hWnd;
		me->uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		break;
	case WM_TIMER: 
	{
		static UINT MsgPosSaved = 0;
		if (MsgPosSaved != me->MsgPos) {
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
			MsgPosSaved = me->MsgPos;
		}
	}
	break;
	case WM_SIZE:
	{
		GetClientRect(hWnd, &me->WinRect);
		me->ScreenMaxLines = me->WinRect.bottom / FONT_SIZE;
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
	}
	break;
	case WM_NOTIFY:
		me->DoNotify(message, wParam, lParam);
		break;
	case WM_COMMAND:
		me->OnCommand(message, wParam, lParam);
		break;
	case WM_RBUTTONUP:
		me->bAutoScroll = !me->bAutoScroll;
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;
	case WM_LBUTTONUP:
		break;
	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;
	case WM_PAINT:
		me->Paint();
	break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		me->KeyAndScroll(message, wParam, lParam);
		break;
	case WM_MOUSEWHEEL:
	{
		INT fwKeys = GET_KEYSTATE_WPARAM(wParam);
		INT zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		//DbgMsg("WM_MOUSEWHEEL fwKeys：%d, zDelta:%d", fwKeys, zDelta);
		INT delta = zDelta / 120 * 5;
		me->VScrollPos = BOUND(me->VScrollPos - delta, 0, me->VScrollRang);
		SetScrollPos(hWnd, SB_VERT, me->VScrollPos, true);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
	}
	break;
	case WM_DESTROY:
		me->SaveValue();
		WaitForSingleObject(me->hMutex, INFINITE);
		me->MsgPos = 0;
		for (int i = 0; i < MSG_MAX_LENGTH; i++) {
			if (me->Msgs[i] != NULL) {
				delete[] me->Msgs[i];
				me->Msgs[i] = NULL;
			}
			else break;
		}
		ReleaseMutex(me->hMutex);
		me->hWnd = NULL;
		DbgMsg("CWinMsg WM_DESTROY\r\n");
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CWinMsg::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDM_FFT_HORZ_ZOOM_INCREASE:
		break;
	case IDM_FFT_HORZ_ZOOM_DECREASE:
		break;
	case IDM_FFT_HORZ_ZOOM_HOME:
		break;
	case IDM_FFT_VERT_ZOOM_INCREASE:
		break;
	case IDM_FFT_VERT_ZOOM_DECREASE:
		break;
	case IDM_FFT_VERT_ZOOM_HOME:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

void CWinMsg::SaveValue(void)
{
#define VALUE_LENGTH	100
	char section[VALUE_LENGTH];
	sprintf(section, "CWinMsg");
	//WritePrivateProfileString(section, "HScrollPos", std::to_string(HScrollPos).c_str(), IniFilePath);
	//WritePrivateProfileString(section, "VScrollPos", std::to_string(VScrollPos).c_str(), IniFilePath);
	//WritePrivateProfileString(section, "HScrollZoom", std::to_string(HScrollZoom).c_str(), IniFilePath);
	//WritePrivateProfileString(section, "VScrollZoom", std::to_string(VScrollZoom).c_str(), IniFilePath);
}

void CWinMsg::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	char section[VALUE_LENGTH];
	sprintf(section, "CWinMsg");
	//GetPrivateProfileString(section, "HScrollPos", "0", value, VALUE_LENGTH, IniFilePath);
	//HScrollPos = atoi(value);
	//GetPrivateProfileString(section, "VScrollPos", "0", value, VALUE_LENGTH, IniFilePath);
	//VScrollPos = atoi(value);
	//GetPrivateProfileString(section, "HScrollZoom", "1.0", value, VALUE_LENGTH, IniFilePath);
	//HScrollZoom = atof(value);
	//GetPrivateProfileString(section, "VScrollZoom", "1.0", value, VALUE_LENGTH, IniFilePath);
	//VScrollZoom = atof(value);
}

void CWinMsg::Paint(void)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);
	RECT r = { 0 }, rt = { 0 };
	
	GetClientRect(hWnd, &rt);
	r = rt;

	HDC		hdc = CreateCompatibleDC(hDC);
	HBITMAP hbmp = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hdc, hbmp);
	FillRect(hdc, &rt, (HBRUSH)GetStockObject(WHITE_BRUSH));

	static HFONT hFont = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial"));
	SelectObject(hdc, hFont);
	SIZE sizeText;
	UINT pos = VScrollPos;
	pos &= MSG_MAX_MASK;
	SetBkMode(hdc, TRANSPARENT);
	//	SetBkMode(hdc, OPAQUE); 
	//	SetBkColor(hdc,RGB(0,0,0));
	char* str = NULL;
	r.left = rt.right - 100;
	r.top += 10;
	if (bAutoScroll) {
		SetTextColor(hdc, RGB(0, 255, 0));
		DrawText(hdc, "Auto Scroll", 11, &r, NULL);
	}
	r = rt;
	SetTextColor(hdc, RGB(0, 0, 0));
	while (Msgs[pos] != NULL && r.top < rt.bottom) {
		str = Msgs[pos];
		UINT n = strlen(str);
		GetTextExtentPoint32(hdc, str, n, &sizeText);
		DrawText(hdc, str, n, &r, NULL);
		r.top += sizeText.cy;
		pos++;
		pos &= MSG_MAX_MASK;
	}

	BitBlt(
		hDC, 0, 0, rt.right, rt.bottom,
		hdc, 0, 0,
		SRCCOPY
	);
	DeleteObject(hdc);
	DeleteObject(hbmp);

	EndPaint(hWnd, &ps);
}

void CWinMsg::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
{
	INT     iMax, iMin, iPos;
	int		dn = 0, tbdn = 0;
	RECT    rc;

	switch (message)
	{
	case WM_KEYDOWN:
		/* Translate keyboard messages to scroll commands */
		switch (wParam)
		{
		case VK_UP:
			PostMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0L);
			break;
		case VK_DOWN:
			PostMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0L);
			break;
		case VK_PRIOR:
			PostMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0L);
			break;
		case VK_NEXT:
			PostMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
			break;
		case VK_HOME:
			PostMessage(hWnd, WM_HSCROLL, SB_PAGEUP, 0L);
			break;
		case VK_END:
			PostMessage(hWnd, WM_HSCROLL, SB_PAGEDOWN, 0L);
			break;
		case VK_LEFT:
			PostMessage(hWnd, WM_HSCROLL, SB_LINEUP, 0L);
			break;
		case VK_RIGHT:
			PostMessage(hWnd, WM_HSCROLL, SB_LINEDOWN, 0L);
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_UP:
		case VK_DOWN:
		case VK_PRIOR:
		case VK_NEXT:
			PostMessage(hWnd, WM_VSCROLL, SB_ENDSCROLL, 0L);
			break;

		case VK_HOME:
		case VK_END:
		case VK_LEFT:
		case VK_RIGHT:
			PostMessage(hWnd, WM_HSCROLL, SB_ENDSCROLL, 0L);
			break;
		}
		break;

	case WM_VSCROLL:
		//Calculate new vertical scroll position
		GetScrollRange(hWnd, SB_VERT, &iMin, &iMax);
		iPos = GetScrollPos(hWnd, SB_VERT);
		GetClientRect(hWnd, &rc);
		switch (GET_WM_VSCROLL_CODE(wParam, lParam))
		{
		case SB_LINEDOWN:
			dn = rc.bottom / FONT_SIZE / 16;
			break;
		case SB_LINEUP:
			dn = -rc.bottom / FONT_SIZE / 16;
			break;
		case SB_PAGEDOWN:
			dn = rc.bottom / FONT_SIZE / 2;
			break;
		case SB_PAGEUP:
			dn = -rc.bottom / FONT_SIZE / 2;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			tbdn = GET_WM_VSCROLL_POS(wParam, lParam);
			break;
		default:
			dn = 0;
			break;
		}

		if (dn != 0)
		{
			VScrollPos = BOUND(VScrollPos + dn, 0, MsgPos);
		}
		if (tbdn != 0)
		{
			VScrollPos = BOUND(tbdn, 0, MsgPos);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_VERT, VScrollPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		break;

	case WM_HSCROLL:
		//Calculate new horizontal scroll position
		GetScrollRange(hWnd, SB_HORZ, &iMin, &iMax);
		iPos = GetScrollPos(hWnd, SB_HORZ);
		GetClientRect(hWnd, &rc);

		switch (GET_WM_HSCROLL_CODE(wParam, lParam))
		{
		case SB_LINEDOWN:
			dn = rc.right / 16 + 1;
			break;
		case SB_LINEUP:
			dn = -rc.right / 16 + 1;
			break;
		case SB_PAGEDOWN:
			dn = rc.right / 2 + 1;
			break;
		case SB_PAGEUP:
			dn = -rc.right / 2 + 1;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			tbdn = GET_WM_HSCROLL_POS(wParam, lParam);
			break;
		default:
			dn = 0;
			break;
		}
		if (dn != 0)
		{
			HScrollPos = BOUND(HScrollPos + dn, 0, HScrollRang);
		}
		if (tbdn != 0)
		{
			HScrollPos = BOUND(tbdn, 0, HScrollRang);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		break;
	}
}
