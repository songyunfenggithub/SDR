
#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "Debug.h"

#include "CSoundCard.h"
#include "CData.h"
#include "CFFT.h"
#include "CAnalyze.h"

#include "CWinFFT.h"
#include "CWinSDR.h"
#include "CWinTools.h"
#include "CScreenButton.h"

#include "CWinSpectrumScanFFTShow.h"
#include "CWinSpectrumScanSet.h"

#include <dwmapi.h>
#pragma comment(lib,"Dwmapi.lib")

using namespace std;
using namespace WINS;
using namespace WINS::SPECTRUM_SCAN;
//using namespace DEVICES;

#define GET_WM_VSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)      HIWORD(wp)
#define GET_WM_HSCROLL_CODE(wp, lp)     LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)      HIWORD(wp)

#define BRUSH_BACKGROUND		BLACK_BRUSH

#define COLOR_BORDER_THICK		RGB(64, 64, 64)
#define COLOR_BORDER_THIN		RGB(128, 128, 128)
#define COLOR_BACKGROUND		RGB(128, 128, 128)

#define COLOR_ORIGNAL_FFT		RGB(0, 255, 0)  
#define COLOR_ORIGNAL_FFT_LOG	RGB(0, 255, 255)  
#define COLOR_FILTTED_FFT		RGB(0, 0, 255)  
#define COLOR_FILTTED_FFT_LOG	RGB(255, 255, 0)  

#define COLOR_TEXT_BACKGOUND	RGB(0, 0, 0)
#define COLOR_TEXT				RGB(255, 255, 255)

#define DIVLONG		10
#define DIVSHORT	5

#define TIMEOUT		100

#define DIVLONG		10
#define DIVSHORT	5

#define FONT_SIZE	14

#define CONTROLS_TOP		10
#define CONTROLS_LEFT		100
#define CONTROLS_TITLE_LEFT	10
#define CONTROLS_HEIGHT		36

#define CONTROL_FFTSIZE_ID		(WM_USER + 1)

CWinSpectrumScanSet clsWinSpectrumScanSet;

CWinSpectrumScanSet::CWinSpectrumScanSet()
{
	OPENCONSOLE_SAVED;

	hMutex = CreateMutex(NULL, false, "CWinScanSethMutex");

	RegisterWindowsClass();

	ButtonRFStart = new CScreenButton(new CScreenButton::BUTTON{ CONTROLS_LEFT, CONTROLS_TOP + CONTROLS_HEIGHT * 0, 100, 100, -1, &ButtonRect, 13,  NULL, "0,000,000,000", RGB(0, 0, 0),
		CScreenButton::BUTTON_FONT_SIZE::Button_Font_Size_32, CScreenButton::Button_Mouse_Num1, CScreenButton::Button_Mouse_None,
		true, 0, 0, 0, 2000000000,
		CScreenButton::Button_Align_Left,  NULL, NULL, ButtonRFStartFunc });
	ButtonRFStop = new CScreenButton(new CScreenButton::BUTTON{ CONTROLS_LEFT, CONTROLS_TOP + CONTROLS_HEIGHT * 1, 100, 100, -1, &ButtonRect, 13,  NULL, "0,000,000,000", RGB(0, 0, 0),
		CScreenButton::BUTTON_FONT_SIZE::Button_Font_Size_32, CScreenButton::Button_Mouse_Num1, CScreenButton::Button_Mouse_None,
		true, 0, 0, 0, 2000000000,
		CScreenButton::Button_Align_Left,  NULL, NULL, ButtonRFStopFunc });

	ButtonAverageDeep = new CScreenButton(new CScreenButton::BUTTON{ CONTROLS_LEFT, CONTROLS_TOP + CONTROLS_HEIGHT * 4, 100, 100, -1, &ButtonRect, 9, NULL, "AvgFlt:", RGB(255, 255, 255),
	CScreenButton::BUTTON_FONT_SIZE::Button_Font_Size_16, CScreenButton::Button_Step_Num, CScreenButton::Button_Mouse_None,
	true, 1, 1, 2, FFT_DEEP,
	CScreenButton::Button_Align_Left,  NULL, NULL, ButtonAverageDeepFunc });
}

CWinSpectrumScanSet::~CWinSpectrumScanSet()
{
	//CLOSECONSOLE;
}

void CWinSpectrumScanSet::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CWinSpectrumScanSet::StaticWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)IDC_MENU_SPECTRUM_SCAN;
	wcex.lpszClassName = WIN_SPECTRUM_SCAN_SET_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

void CWinSpectrumScanSet::OpenWindow(void)
{
	if (hWnd == NULL) {
		hWnd = CreateWindow(WIN_SPECTRUM_SCAN_SET_CLASS, "频谱扫描设置窗口", WS_OVERLAPPEDWINDOW,// & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, 0, 600, 600, NULL, NULL, hInst, this);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

LRESULT CALLBACK CWinSpectrumScanSet::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return clsWinSpectrumScanSet.WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CWinSpectrumScanSet::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		this->hWnd = hWnd;

		ButtonRFStart->ButtonInit(hWnd);
		ButtonRFStop->ButtonInit(hWnd);
		ButtonAverageDeep->ButtonInit(hWnd);
		RestoreValue();
		ButtonRFStart->RefreshMouseNumButton(ButtonRFStart->Button->value);
		ButtonRFStop->RefreshMouseNumButton(ButtonRFStop->Button->value);
		{
			hWndFFTSize = CreateWindowEx(
				0, TEXT("combobox"), NULL,
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST,
				CONTROLS_LEFT, CONTROLS_TOP + CONTROLS_HEIGHT * 3, 200, 14,
				hWnd, (HMENU)CONTROL_FFTSIZE_ID, hInst, NULL
			);
			char s[100];
			SendMessage(hWndFFTSize, (UINT)CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
			for (int i = 12; i < 23; i++) {
				sprintf(s, "%u", (UINT)1 << i);
				SendMessage(hWndFFTSize, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)s);
				if (FFTSize == 1 << i) SendMessage(hWndFFTSize, (UINT)CB_SETCURSEL, (WPARAM)i - 12, (LPARAM)0);
			}
			//			SendMessage(hWndrfRange, WM_SETTEXT, 0, (LPARAM)s);
			static HFONT hFont = CreateFont(FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Arial"));
			SendMessage(hWndFFTSize, WM_SETFONT, (WPARAM)hFont, TRUE);
		}
	}
	break;
	case WM_CHAR:
		DbgMsg("CWinOneSDRScanSet WM_CHAR\r\n");
		//PostMessage(clsWinSpect.hWnd, message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		//clsWinSpect.ActivehWnd = NULL;
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	{
		MouseX = GET_X_LPARAM(lParam);
		MouseY = GET_Y_LPARAM(lParam);
		OnMouse();
		ButtonRFStart->OnMouseMouseNumButton(hWnd, message, wParam, lParam);
		ButtonRFStop->OnMouseMouseNumButton(hWnd, message, wParam, lParam);
		ButtonAverageDeep->OnMouseButton(hWnd, message, wParam, lParam);
		static INT ButtonRFStartMouseIndex = 0;
		static INT ButtonRFStopMouseIndex = 0;
		if (
			ButtonRFStartMouseIndex != ButtonRFStart->Button->MouseIndex ||
			ButtonRFStopMouseIndex != ButtonRFStop->Button->MouseIndex
			) {
			ButtonRFStartMouseIndex = ButtonRFStart->Button->MouseIndex;
			ButtonRFStopMouseIndex = ButtonRFStop->Button->MouseIndex;
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
	}
	break;
	case WM_TIMER:
		break;
	case WM_SIZE:
		GetClientRect(hWnd, &WinRect);
		GetClientRect(hWnd, &ButtonRect);
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;
	case WM_COMMAND:
		return OnCommand(message, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		//不加这条消息屏幕刷新会闪烁
		break;
	case WM_PAINT:
		Paint();
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_VSCROLL:
	case WM_HSCROLL:
		KeyAndScroll(message, wParam, lParam);
		break;
	case WM_DESTROY:
		SaveValue();
		this->hWnd = NULL;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

BOOL CWinSpectrumScanSet::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case CONTROL_FFTSIZE_ID:
		if (wmEvent == CBN_SELCHANGE) {
			char str[100];
			int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			(TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)str);
			FFTSize = atoi(str);
			clsWinSpectrumScanFFTShow.fft->Init(FFTSize, FFTSize, clsWinSpectrumScanFFTShow.fft->FFTInfo->AverageDeep);
			//DbgMsg("FFTSize CBN_SELCHANGE %s\r\n", str);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

void CWinSpectrumScanSet::OnMouse(void)
{
}

VOID CWinSpectrumScanSet::Paint(void)
{

	PAINTSTRUCT ps = { 0 };
	HDC hDC = BeginPaint(hWnd, &ps);

	RECT	rt = { 0 }, r = { 0 };
	GetClientRect(hWnd, &rt);

	HDC		hdc = CreateCompatibleDC(hDC);
	HBITMAP hbmp = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hdc, hbmp);

	static HFONT hFont = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial"));
	SelectObject(hdc, hFont);

	FillRect(hdc, &rt, (HBRUSH)GetStockObject(WHITE_BRUSH));

	HPEN hPen = CreatePen(PS_DOT, 0, COLOR_BORDER_THICK);
	HPEN hPenLighter = CreatePen(PS_DOT, 0, COLOR_BORDER_THIN);

	SetBkColor(hdc, COLOR_BACKGROUND);

	SetBkMode(hdc, TRANSPARENT);
	//	SetBkMode(hdc, OPAQUE); 
	//	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);

	char str[100];
	int n = 0;

	r.top = ButtonRFStart->Button->Y + (ButtonRFStart->Button->H - FONT_SIZE) / 2;
	r.left = ButtonRFStart->Button->X - CONTROLS_LEFT + CONTROLS_TITLE_LEFT;
	r.right = r.left + CONTROLS_LEFT;
	r.bottom = r.top + ButtonRFStart->Button->H;
	n = sprintf(str, "开始频率");
	DrawText(hdc, str, n, &r, NULL);
	ButtonRFStart->DrawMouseNumButton(hdc);

	r.top = ButtonRFStop->Button->Y + (ButtonRFStop->Button->H - FONT_SIZE) / 2;
	r.left = ButtonRFStop->Button->X - CONTROLS_LEFT + CONTROLS_TITLE_LEFT;
	r.right = r.left + CONTROLS_LEFT;
	r.bottom = r.top + ButtonRFStop->Button->H;
	n = sprintf(str, "截止频率");
	DrawText(hdc, str, n, &r, NULL);
	ButtonRFStop->DrawMouseNumButton(hdc);

	r.top += 36;
	r.left = CONTROLS_LEFT;
	r.right = WinRect.right;
	r.bottom += 36;
	char t1[100];
	n = sprintf(str, "扫描频率带宽: %s Hz", fomatKINT64(ButtonRFStop->Button->value - ButtonRFStart->Button->value, t1));
	DrawText(hdc, str, n, &r, NULL);

	RECT rw = { 0 };
	GetWindowRect(hWndFFTSize, &rw);
	POINT p = { rw.left, rw.top };
	ScreenToClient(hWnd,&p);
	r.top = p.y + (rw.bottom - rw.top - FONT_SIZE) / 2;
	r.left = CONTROLS_TITLE_LEFT;
	r.right = CONTROLS_LEFT;
	r.bottom = r.top + FONT_SIZE;
	n = sprintf(str, "FFTSize");
	DrawText(hdc, str, n, &r, NULL);


	ButtonAverageDeep->DrawButton(hdc);

	BitBlt(hDC,
		0, 0,
		rt.right, rt.bottom,
		hdc,
		0, 0,
		SRCCOPY);
	DeleteObject(hdc);
	DeleteObject(hbmp);

	ReleaseMutex(hMutex);

	EndPaint(hWnd, &ps);
}

void CWinSpectrumScanSet::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
{
	INT     iMax, iMin, iPos;
	int		dn = 0, tbdn = 0;
	RECT    rc;

	switch (message)
	{
	case WM_KEYDOWN:
		DbgMsg("win fft WM_KEYDOWN\r\n");
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
			dn = rc.bottom / 16;
			break;
		case SB_LINEUP:
			dn = -rc.bottom / 16;
			break;
		case SB_PAGEDOWN:
			dn = rc.bottom / 2;
			break;
		case SB_PAGEUP:
			dn = -rc.bottom / 2;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			break;
		}

	case WM_HSCROLL:
		//Calculate new horizontal scroll position
		GetScrollRange(hWnd, SB_HORZ, &iMin, &iMax);
		iPos = GetScrollPos(hWnd, SB_HORZ);
		GetClientRect(hWnd, &rc);

		switch (GET_WM_HSCROLL_CODE(wParam, lParam))
		{
		case SB_LINEDOWN:
			dn = 1;// (rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / 16;
			break;
		case SB_LINEUP:
			dn = -1;// -(rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / 16;
			break;
		case SB_PAGEDOWN:
			dn = (rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / 2;
			break;
		case SB_PAGEUP:
			dn = -(rc.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT) / 2;
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
			HScrollPos = BOUND(HScrollPos + dn, 0, HScrollRange);
		}
		if (tbdn != 0)
		{
			HScrollPos = BOUND(tbdn, 0, HScrollRange);
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_HORZ, HScrollPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
			DbgMsg("CWinSDRScan HScrollPos: %d, HScrollRange: %d.\r\n", HScrollPos, HScrollRange);
		}
		break;
	}
}

void CWinSpectrumScanSet::SaveValue(void)
{
#define VALUE_LENGTH	100
	char section[VALUE_LENGTH];
	sprintf(section, "CWinSpectrumScanSet");
	char value[VALUE_LENGTH];
	WritePrivateProfileString(section, "RF_Start", std::to_string(ButtonRFStart->Button->value).c_str(), IniFilePath);
	WritePrivateProfileString(section, "RF_Stop", std::to_string(ButtonRFStop->Button->value).c_str(), IniFilePath);
	WritePrivateProfileString(section, "FFT_Size", std::to_string(FFTSize).c_str(), IniFilePath);
	WritePrivateProfileString(section, "AverageDeep", std::to_string(ButtonAverageDeep->Button->value).c_str(), IniFilePath);
}

void CWinSpectrumScanSet::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	char section[VALUE_LENGTH];
	sprintf(section, "CWinSpectrumScanSet");
	GetPrivateProfileString(section, "RF_Start", "0", value, VALUE_LENGTH, IniFilePath);
	ButtonRFStart->Button->value = atoi(value);
	GetPrivateProfileString(section, "RF_Stop", "0", value, VALUE_LENGTH, IniFilePath);
	ButtonRFStop->Button->value = atoi(value);
	GetPrivateProfileString(section, "FFT_Size", "16384", value, VALUE_LENGTH, IniFilePath);
	FFTSize = atoi(value);
	GetPrivateProfileString(section, "AverageDeep", "2", value, VALUE_LENGTH, IniFilePath);
	ButtonAverageDeep->Button->value = atoi(value);
}

void CWinSpectrumScanSet::ButtonRFStartFunc(CScreenButton* button)
{
	clsAnalyze.set_SDR_rfHz(button->Button->value);

	InvalidateRect(clsWinSpectrumScanSet.hWnd, NULL, TRUE);
	UpdateWindow(clsWinSpectrumScanSet.hWnd);
}

void CWinSpectrumScanSet::ButtonRFStopFunc(CScreenButton* button)
{
	InvalidateRect(clsWinSpectrumScanSet.hWnd, NULL, TRUE);
	UpdateWindow(clsWinSpectrumScanSet.hWnd);
}

void CWinSpectrumScanSet::ButtonAverageDeepFunc(CScreenButton* button)
{
	clsWinSpectrumScanFFTShow.fft->Init(clsWinSpectrumScanSet.FFTSize, clsWinSpectrumScanSet.FFTSize, button->Button->value);
	InvalidateRect(clsWinSpectrumScanSet.hWnd, NULL, TRUE);
	UpdateWindow(clsWinSpectrumScanSet.hWnd);
}
