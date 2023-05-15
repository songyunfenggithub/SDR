

#pragma comment(linker,"\"/manifestdependency:type='win32' "\
						"name='Microsoft.Windows.Common-Controls' "\
						"version='6.0.0.0' processorArchitecture='*' "\
						"publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "stdafx.h"
#include "resource.h"
#include "stdio.h"

#include "Public.h"
#include "Debug.h"

#include "SDRPlay_API.3.09/API/inc/sdrplay_api.h"
#pragma comment(lib,"SDRPlay_API.3.09/API/x64/sdrplay_api.lib")

#include "CSDR.h"
#include "CDataFromSDR.h"

#include "CWinSDRSet.h"
#include "CWinControls.h"

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

using namespace DEVICES; 
using namespace WINS;
using namespace WINS::SDR_SET;

CWinControls::CWinControls()
{
	RegisterWindowsClass();
}

CWinControls::~CWinControls()
{

}

void CWinControls::RegisterWindowsClass(void)
{
	static bool first = true;
	if (first == false) return;
	first = false;

	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinControls::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = WIN_CONTROLS_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinControls::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(WIN_CONTROLS_CLASS, "", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 600, 600, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

bool CWinControls::DoNotify(UINT msg, WPARAM wParam, LPARAM lParam)
{
	UINT index = wParam - WM_USER;
	HWND hwnd = hWndControls[index];
	LPNMHDR pnmh = (LPNMHDR)lParam;

	switch (SDR_params[index].valueType) {
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_NONE:
		break;
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_ENUM:
	{

	}
	break;
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_DOUBLE:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_FLOAT:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_INT:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_UINT:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_USHORT:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_UCHAR:
		break;
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_ENABLE_DISABLE:
		switch (pnmh->code) {
	
		}
		break;
	}
	return TRUE;
}

bool CWinControls::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);
	int index = wmId - WM_USER;
	char str[100];

	switch (SDR_params[index].valueType) {
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_NONE:
		break;
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_ENUM:
		if (wmEvent == CBN_SELCHANGE) {
			int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			(TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)str);
			DbgMsg("SDR_PAMRAS_ENUM CBN_SELCHANGE %s\r\n", str);
			CheckApply(index);
		}
	break;
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_DOUBLE:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_FLOAT:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_INT:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_UINT:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_USHORT:
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_UCHAR:
		if (wmEvent == EN_KILLFOCUS) {
			GetWindowText((HWND)lParam, str, 99);
			DbgMsg("SDR_PAMRAS_ EN_KILLFOCUS %s\r\n", str);
			CheckApply(index);
		}
		break;
	case SDR_PAMRAS_TYPE::SDR_PAMRAS_ENABLE_DISABLE:
		if (wmEvent == BN_CLICKED) {
			DbgMsg("SDR_PAMRAS_ENABLE_DISABLE BN_CLICKED %d\r\n", SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0);
			CheckApply(index);
		}
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinControls::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CWinControls* me = (CWinControls*)get_WinClass(hWnd);

	switch (message)
	{
	case WM_CREATE:
		me = (CWinControls*)set_WinClass(hWnd, lParam);
		me->hWnd = hWnd;
		break;
	case WM_SIZE:
	{
		GetClientRect(hWnd, &me->WinRect);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
	}
	break;
	case WM_RBUTTONUP: 
	{
		UINT xPos = GET_X_LPARAM(lParam);
		UINT yPos = GET_Y_LPARAM(lParam);
		if (xPos < 300) {
			int i = 0;
			static char s[1000];
			int index = yPos / PARAMS_ITEM_HEIGHT;
			if (SDR_params[index].comment != NULL) {
				DbgMsg((PSTR)SDR_params[index].txt);
				DbgMsg("--- help start ---");

				switch (SDR_params[index].valueType)
				{
				case SDR_PAMRAS_DOUBLE:
					i += sprintf(s + i, "default = %.2lf, min = %.2lf, max = %.2lf\r\n\r\n",
						(double)SDR_params[index].default, (double)SDR_params[index].min, (double)SDR_params[index].max);
					break;
				case SDR_PAMRAS_FLOAT:
					i += sprintf(s + i, "default = %.2f, min = %.2f, max = %.2f\r\n\r\n",
						(float)SDR_params[index].default, (float)SDR_params[index].min, (float)SDR_params[index].max);
					break;
				case SDR_PAMRAS_INT:
					i += sprintf(s + i, "default = %d, min = %d, max = %d\r\n\r\n",
						(int)SDR_params[index].default, (int)SDR_params[index].min, (int)SDR_params[index].max);
					break;
				case SDR_PAMRAS_UINT:
					i += sprintf(s + i, "default = %u, min = %u, max = %u\r\n\r\n",
						(unsigned int)SDR_params[index].default, (unsigned int)SDR_params[index].min, (unsigned int)SDR_params[index].max);
					break;
				case SDR_PAMRAS_USHORT:
					i += sprintf(s + i, "default = %hu, min = %hu, max = %hu\r\n\r\n",
						(unsigned short)SDR_params[index].default, (unsigned short)SDR_params[index].min, (unsigned short)SDR_params[index].max);
					break;
				case SDR_PAMRAS_UCHAR:
				case SDR_PAMRAS_ENABLE_DISABLE:
					i += sprintf(s + i, "default = %hhu, min = %hhu, max = %hhu\r\n\r\n",
						(unsigned char)SDR_params[index].default, (unsigned char)SDR_params[index].min, (unsigned char)SDR_params[index].max);
					break;
				case SDR_PAMRAS_ENUM:
				{
					SDR_ENUM_MAP* pMap = SDR_params[index].pEnumMap;
					int n = pMap->size();
					int i = 0;
					int itemi = 0;
					i += sprintf(s + i, "default : %s = %d\r\n\r\n", clsSDR.map_value_to_key(pMap, (int)SDR_params[index].default), (int)SDR_params[index].default);
					for (SDR_ENUM_MAP::iterator it = pMap->begin(); it != pMap->end(); it++) {
						i += sprintf(s + i, "%s = %d\r\n", it->first, it->second);
					}
				}
				break;
				case SDR_PAMRAS_NONE:
				defult:
					break;
				}
				DbgMultiMsg(s);

				DbgMultiMsg((PSTR)SDR_params[index].comment);
				DbgMsg("--- help end -----");
			}
		}
	}
	break;
	case WM_MOUSEWHEEL:
	{
		INT fwKeys = GET_KEYSTATE_WPARAM(wParam);
		INT zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		//DbgMsg("WM_MOUSEWHEEL fwKeys：%d, zDelta:%d", fwKeys, zDelta);
		if ((clsWinSDRSet.VScrollPos == 0 && zDelta > 0) || (clsWinSDRSet.VScrollPos == clsWinSDRSet.VScrollRang && zDelta < 0)) break;
		INT delta = zDelta / 120 * PARAMS_ITEM_HEIGHT * 5;
		clsWinSDRSet.VScrollPos = BOUND(clsWinSDRSet.VScrollPos - delta, 0, clsWinSDRSet.VScrollRang);
		SetScrollPos(clsWinSDRSet.hWnd, SB_VERT, clsWinSDRSet.VScrollPos, true);
		//MoveWindow(clsWinSDRSet.m_WinControls->hWnd, 0, -clsWinSDRSet.VScrollPos, clsWinSDRSet.WinRect.right, clsSDR.max_index * PARAMS_ITEM_HEIGHT, true);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
	}
	break;
	case WM_COMMAND:
		me->OnCommand(message, wParam, lParam);
		break;
	case WM_NOTIFY:
		me->DoNotify(message, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;
	case WM_PAINT:
	{
		//PAINTSTRUCT ps;
		//HDC hDC = BeginPaint(hWnd, &ps);
		//EndPaint(hWnd, &ps);
		me->Paint();
	}
	break;
	case WM_DESTROY:
	{
		for (int index = 0; index < clsSDR.max_index; index++) {
			if (me->hWndControls[index] != NULL) DestroyWindow(me->hWndControls[index]);
			me->hWndControls[index] = NULL;
		}
		me->hWnd = NULL;
		DbgMsg("CWinControls WM_DESTROY\r\n");
	}
	break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void CWinControls::Paint(void)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);
	RECT r = { 0 }, rt = clsWinSDRSet.WinRect;

	//GetClientRect(hWnd, &rt);
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
	static HPEN hPen = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
	SelectObject(hdc, hPen);
	r = rt;
	UINT topPos = clsWinSDRSet.VScrollPos;
	//static HWND show_hwnds[100] = { 0 };
	//UINT show_indexs[100] = { 0 };
	//UINT show = 0;
	//for (int i = 0; show_hwnds[i] != NULL; i++) {
	//	MoveWindow(show_hwnds[i], PARAMS_ITEM_CONTROL_POSTION, -100, PARAMS_ITEM_CONTROL_WIDTH, PARAMS_ITEM_HEIGHT - 4, true);
	//	show_hwnds[i] = NULL;
	//}
	for (index = topPos / PARAMS_ITEM_HEIGHT; index < clsSDR.max_index; index++) {
		if (SDR_params[index].valueType == SDR_PAMRAS_TYPE::SDR_PAMRAS_NONE) {
			MoveToEx(hdc, 0, index * PARAMS_ITEM_HEIGHT - topPos, NULL);
			LineTo(hdc, WinRect.right, index * PARAMS_ITEM_HEIGHT - topPos);
			MoveToEx(hdc, 0, (index + 1) * PARAMS_ITEM_HEIGHT - topPos, NULL);
			LineTo(hdc, WinRect.right, (index + 1) * PARAMS_ITEM_HEIGHT - topPos);
		}
		HWND hwnd = hWndControls[index];
		if (hwnd != NULL) {
			//show_indexs[show] = index;
			//show_hwnds[show++] = hwnd;
			BitBlt(
				hdc, PARAMS_ITEM_CONTROL_POSTION, index * PARAMS_ITEM_HEIGHT + 1 - topPos, PARAMS_ITEM_CONTROL_WIDTH, PARAMS_ITEM_HEIGHT - 4,
				GetWindowDC(hwnd), 0, 0,
				SRCCOPY
			);
		}
		r.left = 10 * SDR_params[index].level;
		r.top = index * PARAMS_ITEM_HEIGHT - topPos + (PARAMS_ITEM_HEIGHT - FONT_SIZE) / 2;
		DrawText(hdc, SDR_params[index].txt, strlen(SDR_params[index].txt), &r, NULL);
		r.top += PARAMS_ITEM_HEIGHT;
		if (r.top > rt.bottom)break;
	}

	BitBlt(
		hDC, 0, topPos, rt.right, rt.bottom,
		hdc, 0, 0,
		SRCCOPY
	);

	//for (int i = 0; show_hwnds[i] != NULL; i++)
	//	MoveWindow(show_hwnds[i], PARAMS_ITEM_CONTROL_POSTION, show_indexs[i] * PARAMS_ITEM_HEIGHT + 1 - topPos, PARAMS_ITEM_CONTROL_WIDTH, PARAMS_ITEM_HEIGHT - 4, true);

	MoveWindow(hWnd, 0, -topPos, WinRect.right, clsSDR.max_index * PARAMS_ITEM_HEIGHT, true);
	DeleteObject(hdc);
	DeleteObject(hbmp);

	EndPaint(hWnd, &ps);
}

void CWinControls::CheckApply(UINT index)
{
	static char str[100];
	HWND hwnd = hWndControls[index];
	GetWindowText(hwnd, str, 99);
	bool Changed = false;

	switch (SDR_params[index].valueType) {
	case SDR_PAMRAS_DOUBLE:
	{
		double dd = std::stold(str);
		if (SDR_params[index].max != 0) dd = dd < SDR_params[index].min ? SDR_params[index].min : (dd > SDR_params[index].max ? SDR_params[index].max : dd);
		double d = *(double*)SDR_params[index].pValue;
		if (d != dd) {
			sprintf(str, "%.2lf", dd);
			SetWindowText(hwnd, str);
			*(double*)(SDR_params[index].pValue) = dd;
			Changed = true;
		}
	}
	break;
	case SDR_PAMRAS_FLOAT:
	{
		float dd = std::stof(str);
		if (SDR_params[index].max != 0) dd = dd < (float)SDR_params[index].min ? (float)SDR_params[index].min :
			(dd > (float)SDR_params[index].max ? (float)SDR_params[index].max : dd);
		float d = *(float*)SDR_params[index].pValue;
		if (d != dd) {
			sprintf(str, "%.2f", dd);
			SetWindowText(hwnd, str);
			*(float*)(SDR_params[index].pValue) = dd;
			Changed = true;
		}
	}
	break;
	case SDR_PAMRAS_INT:
	{
		int dd = std::stoi(str);
		if (SDR_params[index].max != 0) dd = dd < (int)SDR_params[index].min ? (int)SDR_params[index].min :
			(dd > (int)SDR_params[index].max ? (int)SDR_params[index].max : dd);
		int d = *(int*)SDR_params[index].pValue;
		if (d != dd) {
			sprintf(str, "%d", dd);
			SetWindowText(hwnd, str);
			*(int*)(SDR_params[index].pValue) = dd;
			Changed = true;
		}
	}
	break;
	case SDR_PAMRAS_UINT:
	{
		unsigned int dd = (unsigned int)std::stoul(str);
		if (SDR_params[index].max != 0) dd = dd < (unsigned int)SDR_params[index].min ? (unsigned int)SDR_params[index].min :
			(dd > (unsigned int)SDR_params[index].max ? (unsigned int)SDR_params[index].max : dd);
		unsigned int d = *(unsigned int*)SDR_params[index].pValue;
		if (d != dd) {
			sprintf(str, "%u", dd);
			SetWindowText(hwnd, str);
			*(unsigned int*)(SDR_params[index].pValue) = dd;
			Changed = true;
		}
	}
	break;
	case SDR_PAMRAS_USHORT:
	{
		unsigned short dd = (unsigned short)std::stoul(str);
		if (SDR_params[index].max != 0) dd = dd < (unsigned short)SDR_params[index].min ? (unsigned short)SDR_params[index].min :
			(dd > (unsigned short)SDR_params[index].max ? (unsigned short)SDR_params[index].max : dd);
		unsigned short d = *(unsigned short*)SDR_params[index].pValue;
		if (d != dd) {
			sprintf(str, "%hu", dd);
			SetWindowText(hwnd, str);
			*(unsigned short*)(SDR_params[index].pValue) = dd;
			Changed = true;
		}
	}
	break;
	case SDR_PAMRAS_UCHAR:
	{
		unsigned char dd = (unsigned char)std::stoul(str);
		if (SDR_params[index].max != 0) dd = dd < (unsigned char)SDR_params[index].min ? (unsigned char)SDR_params[index].min :
			(dd > (unsigned char)SDR_params[index].max ? (unsigned char)SDR_params[index].max : dd);
		unsigned char d = *(unsigned char*)SDR_params[index].pValue;
		if (d != dd) {
			sprintf(str, "%hhu", dd);
			SetWindowText(hwnd, str);
			*(unsigned char*)(SDR_params[index].pValue) = dd;
			Changed = true;
		}
	}
	break;
	case SDR_PAMRAS_ENUM:
	{
		int ItemIndex = SendMessage(hwnd, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		(TCHAR)SendMessage(hwnd, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)str);
		SDR_ENUM_MAP* pmap = SDR_params[index].pEnumMap;
		SDR_ENUM_MAP::iterator it;
		for (it = pmap->begin(); it != pmap->end(); it++) {
			if (strcmp(it->first, str) == 0)break;
		}
		if (it != pmap->end()) {
			switch (SDR_params[index].enumValueType) {
			case SDR_PAMRAS_TYPE::SDR_PAMRAS_DOUBLE:
			{
				double dd = (double)it->second;
				if (*(double*)SDR_params[index].pValue != dd) {
					*(double*)SDR_params[index].pValue = dd;
					Changed = true;
				}

			}
			break;
			case SDR_PAMRAS_TYPE::SDR_PAMRAS_UCHAR:
			{
				UCHAR dd = (UCHAR)it->second;
				if (*(UCHAR*)SDR_params[index].pValue != dd) {
					*(UCHAR*)SDR_params[index].pValue = dd;
					Changed = true;
				}
			}
			break;
			case SDR_PAMRAS_TYPE::SDR_PAMRAS_ENUM:
			{
				int dd = (int)it->second;
				if (*(int*)SDR_params[index].pValue != dd) {
					*(int*)SDR_params[index].pValue = dd;
					Changed = true;
				}

			}
			break;
			}
		}
	}
	break;
	case SDR_PAMRAS_ENABLE_DISABLE:
	{
		UCHAR dd = SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
		if (*(UCHAR*)(SDR_params[index].pValue) != dd) {
			*(UCHAR*)(SDR_params[index].pValue) = dd;
			Changed = true;
		}
	}
	break;
	case SDR_PAMRAS_NONE:
	defalut:
		break;
	}

	if (Changed) SDR_params_apply(index);
}


void CWinControls::SDR_params_apply(UINT index)
{
	DbgMsg("SDR_params_apply\r\n");

	if (SDR_params[index].paramUpdateReason == sdrplay_api_Update_None) return;

	DbgMsg("sdrplay_api_Update paramUpdateReason %d.\r\n", SDR_params[index].paramUpdateReason);

	sdrplay_api_ErrT err;
	if ((err = sdrplay_api_Update(clsGetDataSDR.chosenDevice->dev, clsGetDataSDR.chosenDevice->tuner,
		(sdrplay_api_ReasonForUpdateT)SDR_params[index].paramUpdateReason, sdrplay_api_Update_Ext1_None)) !=
		sdrplay_api_Success)
	{
		DbgMsg("sdrplay_api_Update %d failed %s\n",
			SDR_params[index].paramUpdateReason,
			sdrplay_api_GetErrorString(err));
	}
	else
	{
		if (SDR_params[index].p_set_params_func != NULL)SDR_params[index].p_set_params_func(index);
	}

}
