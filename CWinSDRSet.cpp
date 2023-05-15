#include "stdafx.h"
#include "resource.h"

#include "stdio.h"

#include "Public.h"
#include "Debug.h"

#include "CSDR.h"

#include "CWinControls.h"
#include "CWinSDRSet.h"


#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)


using namespace DEVICES;
//using namespace WINS;
using namespace WINS::SDR_SET;

CWinSDRSet clsWinSDRSet;

CWinSDRSet::CWinSDRSet()
{
	RegisterWindowsClass();
}

CWinSDRSet::~CWinSDRSet()
{

}

void CWinSDRSet::RegisterWindowsClass(void)
{
	static bool first = true;
	if (first == false) return;
	first = false;

	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinSDRSet::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = WIN_SDR_SET_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinSDRSet::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(WIN_SDR_SET_CLASS, "SDR设置窗口", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 600, 600, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

bool CWinSDRSet::DoNotify(UINT msg, WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

LRESULT CALLBACK CWinSDRSet::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CWinSDRSet* me = (CWinSDRSet*)get_WinClass(hWnd);

	switch (message)
	{
	case WM_CREATE:
		me = (CWinSDRSet*)set_WinClass(hWnd, lParam);
		me->hWnd = hWnd;
		me->buildControls();

	break;
	case WM_SIZE:
	{
		GetClientRect(hWnd, &me->WinRect);

		me->VScrollRang = clsSDR.max_index * PARAMS_ITEM_HEIGHT - me->WinRect.bottom;
		SetScrollRange(hWnd, SB_VERT, 0, me->VScrollRang, false);
		SetScrollPos(hWnd, SB_VERT, me->VScrollPos, true);
		MoveWindow(me->m_WinControls->hWnd, 0, -me->VScrollPos, me->WinRect.right, PARAMS_ITEM_HEIGHT * clsSDR.max_index, true);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
	}
	break;
	case WM_LBUTTONUP:
		break;
	case WM_NOTIFY:
		me->DoNotify(message, wParam, lParam);
		break;
	case WM_COMMAND:
		me->OnCommand(message, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
	//me->Paint();
	break;
	case WM_DESTROY:
		me->SaveValue();
		DestroyWindow(me->m_WinControls->hWnd);
		me->index = 0;
		me->hWnd = NULL;
		DbgMsg("CWinSDRSet WM_DESTROY\r\n");
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
	me->KeyAndScroll(message, wParam, lParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CWinSDRSet::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
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

void CWinSDRSet::SaveValue(void)
{
#define VALUE_LENGTH	100
	char section[VALUE_LENGTH];
	sprintf(section, "SDR Setting");
	//WritePrivateProfileString(section, "HScrollPos", std::to_string(HScrollPos).c_str(), IniFilePath);
	//WritePrivateProfileString(section, "VScrollPos", std::to_string(VScrollPos).c_str(), IniFilePath);
	//WritePrivateProfileString(section, "HScrollZoom", std::to_string(HScrollZoom).c_str(), IniFilePath);
	//WritePrivateProfileString(section, "VScrollZoom", std::to_string(VScrollZoom).c_str(), IniFilePath);
}

void CWinSDRSet::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	char section[VALUE_LENGTH];
	sprintf(section, "SDR Setting");
	//GetPrivateProfileString(section, "HScrollPos", "0", value, VALUE_LENGTH, IniFilePath);
	//HScrollPos = atoi(value);
	//GetPrivateProfileString(section, "VScrollPos", "0", value, VALUE_LENGTH, IniFilePath);
	//VScrollPos = atoi(value);
	//GetPrivateProfileString(section, "HScrollZoom", "1.0", value, VALUE_LENGTH, IniFilePath);
	//HScrollZoom = atof(value);
	//GetPrivateProfileString(section, "VScrollZoom", "1.0", value, VALUE_LENGTH, IniFilePath);
	//VScrollZoom = atof(value);
}

CWinSDRSet* CWinSDRSet::buildSets(CWinSDRSet *winSet)
{
	static HFONT hf = CreateFont(FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Arial"));
	UINT self_index = index;
	UINT WinHeight = 0;

	CWinSDRSet* set = new CWinSDRSet();
	winSet->SDRSets[winSet->WinCount++] = set;
	set->hWnd = CreateWindow(WIN_SDR_SET_CLASS, SDR_params[index].txt,
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		0, 0, 100, 0, winSet->hWnd, NULL, hInst, set);
	ShowWindow(set->hWnd, SW_SHOW);
	UpdateWindow(set->hWnd);

	HWND htext = CreateWindow(TEXT("STATIC"), (LPCSTR)SDR_params[index].txt, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 500, 20, set->hWnd, (HMENU)NULL, NULL, NULL);
	SendMessage(htext, WM_SETFONT, (WPARAM)hf, TRUE);
	WinHeight += 20;

	index++;
	if (index < clsSDR.max_index) {
		while ((SDR_params[self_index].level + 1) == SDR_params[index].level) {
			if (SDR_params[index].valueType == SDR_PAMRAS_NONE) {
				CWinSDRSet* rset = buildSets(set);
				rset->WinSetRt.top = WinHeight;
				WinHeight += rset->WinSetRt.bottom;
				MoveWindow(rset->hWnd, rset->WinSetRt.left, rset->WinSetRt.top, rset->WinSetRt.right, rset->WinSetRt.bottom, true);
			}
			else {
				HWND htext = CreateWindow(TEXT("STATIC"), (LPCSTR)SDR_params[index].txt, WS_CHILD | WS_VISIBLE | SS_LEFT,
					10, WinHeight, 500, 20, set->hWnd, (HMENU)NULL, NULL, NULL);
				SendMessage(htext, WM_SETFONT, (WPARAM)hf, TRUE);
				WinHeight += 20;
				index++;
				if (index >= clsSDR.max_index) break;
			}
		}
	}
	set->WinSetRt = {
		10, 
		0, 
		500 - SDR_params[self_index].level * 10,
		(INT)WinHeight
	};
	DbgMsg("index%d, level%d, title:%s, height:%d", self_index, SDR_params[self_index].level, SDR_params[self_index].txt, set->WinSetRt.bottom);
	return set;
}

void CWinSDRSet::buildControls(void)
{
	m_WinControls = new CWinControls();
	m_WinControls->hWnd = CreateWindow(WIN_CONTROLS_CLASS, "",
		WS_CHILD | WS_VISIBLE,
		0, 0, 100, 0, hWnd, NULL, hInst, m_WinControls);
	ShowWindow(m_WinControls->hWnd, SW_SHOW);
	UpdateWindow(m_WinControls->hWnd);
	static HFONT hFont = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial"));
	UINT DrawTop = 0;
	UINT index = 0;
	UINT DrawedNum = 0;
	for (index = 0; index < clsSDR.max_index; index++) {
		HWND hwnd = NULL;
		switch (SDR_params[index].valueType)
		{
		case SDR_PAMRAS_TYPE::SDR_PAMRAS_NONE:
			break;
		case SDR_PAMRAS_TYPE::SDR_PAMRAS_ENUM:
		{
			hwnd = CreateWindowEx(
				0, TEXT("combobox"), NULL,
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST,
				PARAMS_ITEM_CONTROL_POSTION, index * PARAMS_ITEM_HEIGHT + 1, PARAMS_ITEM_CONTROL_WIDTH, PARAMS_ITEM_HEIGHT - 4,
				m_WinControls->hWnd, (HMENU)(WM_USER + index), hInst, NULL
			);
			static char s[1000];
			SDR_ENUM_MAP* pMap = SDR_params[index].pEnumMap;
			int n = pMap->size();
			int itemi = 0;
			SendMessage(hwnd, (UINT)CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
			for (SDR_ENUM_MAP::iterator it = pMap->begin(); it != pMap->end(); it++) {
				bool sel = false;
				SendMessage(hwnd, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)it->first);
				switch (SDR_params[index].enumValueType) {
					case SDR_PAMRAS_TYPE::SDR_PAMRAS_DOUBLE:
						if (*(double*)SDR_params[index].pValue == it->second) sel = true;
						break;
					case SDR_PAMRAS_TYPE::SDR_PAMRAS_UCHAR:
						if (*(UCHAR*)SDR_params[index].pValue == it->second) sel = true;
						break;
					case SDR_PAMRAS_TYPE::SDR_PAMRAS_ENUM:
						if (*(INT*)SDR_params[index].pValue == it->second) sel = true;
						break;
				}
				if (sel == true) {
					SendMessage(hwnd, (UINT)CB_SETCURSEL, (WPARAM)itemi, (LPARAM)0);
					SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)it->first);
				}
				itemi++;
			}
		}
		break;
		case SDR_PAMRAS_TYPE::SDR_PAMRAS_DOUBLE:
		case SDR_PAMRAS_TYPE::SDR_PAMRAS_FLOAT:
		case SDR_PAMRAS_TYPE::SDR_PAMRAS_INT:
		case SDR_PAMRAS_TYPE::SDR_PAMRAS_UINT:
		case SDR_PAMRAS_TYPE::SDR_PAMRAS_USHORT:
		case SDR_PAMRAS_TYPE::SDR_PAMRAS_UCHAR:
		{
			hwnd = CreateWindow(
				TEXT("EDIT"), NULL, 
				WS_CHILD | WS_VISIBLE | WS_BORDER | SS_LEFT | ES_NUMBER 
				, PARAMS_ITEM_CONTROL_POSTION, index * PARAMS_ITEM_HEIGHT + 1, PARAMS_ITEM_CONTROL_WIDTH, PARAMS_ITEM_HEIGHT - 1,
				m_WinControls->hWnd, (HMENU)(WM_USER + index), NULL, NULL);
			char str[1000];
			switch (SDR_params[index].valueType){
			case SDR_PAMRAS_TYPE::SDR_PAMRAS_DOUBLE:
				sprintf(str, "%.2lf", *(double*)SDR_params[index].pValue);
				break;
			case SDR_PAMRAS_TYPE::SDR_PAMRAS_FLOAT:
				sprintf(str, "%.2f", *(float*)SDR_params[index].pValue);
				break;
			case SDR_PAMRAS_TYPE::SDR_PAMRAS_INT:
				sprintf(str, "%d", *(INT*)SDR_params[index].pValue);
				break;
			case SDR_PAMRAS_TYPE::SDR_PAMRAS_UINT:
				sprintf(str, "%u", *(UINT*)SDR_params[index].pValue);
				break;
			case SDR_PAMRAS_TYPE::SDR_PAMRAS_USHORT:
				sprintf(str, "%hu", *(USHORT*)SDR_params[index].pValue);
				break;
			case SDR_PAMRAS_TYPE::SDR_PAMRAS_UCHAR:
				sprintf(str, "%hhu", *(UCHAR*)SDR_params[index].pValue);
				break;
			}
			SetWindowText(hwnd, str);
		}
		break;
		case SDR_PAMRAS_TYPE::SDR_PAMRAS_ENABLE_DISABLE:
			hwnd = CreateWindow(TEXT("button"), "Enable / Disable", WS_CHILD | WS_VISIBLE | SS_LEFT | BS_AUTOCHECKBOX | BS_NOTIFY,
				PARAMS_ITEM_CONTROL_POSTION, index * PARAMS_ITEM_HEIGHT + 1, PARAMS_ITEM_CONTROL_WIDTH, PARAMS_ITEM_HEIGHT - 4,
				m_WinControls->hWnd, (HMENU)(WM_USER + index), NULL, NULL);
			if(*(UCHAR*)SDR_params[index].pValue != 0)SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);
			break;
		}
		if (hwnd) {
			SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);

			// Create the tooltip. g_hInst is the global instance handle.
			static HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				hWnd, NULL,	hInst, NULL);
			// Associate the tooltip with the tool.
			TOOLINFO toolInfo = { 0 };
			toolInfo.cbSize = sizeof(toolInfo);
			toolInfo.hwnd = hWnd;
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
			toolInfo.uId = (UINT_PTR)hwnd;
			toolInfo.lpszText = (LPSTR)SDR_params[index].comment;
			SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
		}
		//if (hwnd) SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));
		m_WinControls->hWndControls[index] = hwnd;
	}
}


void CWinSDRSet::Paint(void)
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
	UINT DrawTop = 0;
	UINT index = 0;
	UINT DrawedNum = 0;
	SetBkMode(hdc, TRANSPARENT);
	//	SetBkMode(hdc, OPAQUE); 
	//	SetBkColor(hdc,RGB(0,0,0));
	SetTextColor(hdc, RGB(0, 0, 0));
	r = rt;
	for (index = VScrollPos; index < clsSDR.max_index; index++, DrawedNum++) {
		r.left = 10 * SDR_params[index].level;
		DrawText(hdc, SDR_params[index].txt, strlen(SDR_params[index].txt), &r, NULL);
		r.top += PARAMS_ITEM_HEIGHT;
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

void CWinSDRSet::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
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
			VScrollPos = BOUND(VScrollPos + dn, 0, VScrollRang);
		}
		if (tbdn != 0)
		{
			VScrollPos = BOUND(tbdn, 0, VScrollRang);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_VERT, VScrollPos, TRUE);
//			MoveWindow(m_WinControls->hWnd, 0, -VScrollPos, WinRect.right, clsSDR.max_index * PARAMS_ITEM_HEIGHT, true);
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
