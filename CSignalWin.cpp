
#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "myDebug.h"
#include "CData.h"

#include "CAudioWin.h"
#include "CSignalWin.h"

using namespace WINS;

#define WAVE_RECT_BORDER_TOP		25
#define WAVE_RECT_BORDER_LEFT		20
#define WAVE_RECT_BORDER_RIGHT		60
#define WAVE_RECT_BORDER_BOTTON		25

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

#define FONT_HEIGHT	14

#define TIMEOUT		100

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define UP_TO_ZERO(x) (x = x < 0 ? 0 : x)

#define DIVLONG		10
#define DIVSHORT	5

CSignalWin::CSignalWin()
{
	OPENCONSOLE;

	//Init();
	RegisterWindowsClass();

	DrawInfo.iHZoom = 0;
	DrawInfo.iHOldZoom = 0;
	DrawInfo.iVZoom = 0;
	DrawInfo.iVOldZoom = 0;
	DrawInfo.dwHZoomedPos = 0;
}

CSignalWin::~CSignalWin()
{
	UnInit();
	//CLOSECONSOLE;
}

void CSignalWin::Init(void)
{
	DrawInfo.FullVotage = FULL_VOTAGE;
	DrawInfo.VotagePerDIV = DrawInfo.FullVotage / ((UINT64)1 << (((CData*)DataOrignal)->DataBits-1));
}

void CSignalWin::UnInit(void)
{

}

void CSignalWin::KeyAndScroll(UINT message, WPARAM wParam, LPARAM lParam)
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
			tbdn = GET_WM_VSCROLL_POS(wParam, lParam);
			break;
		default:
			dn = 0;
			break;
		}

		if (dn != 0)
		{
			DrawInfo.dwVZoomedPos = BOUND(DrawInfo.dwVZoomedPos + dn, 0, DrawInfo.dwVZoomedHeight);
			DrawInfo.wVSclPos = DrawInfo.dwVZoomedPos >> DrawInfo.iVFit;
		}
		if (tbdn != 0)
		{
			DrawInfo.wVSclPos = BOUND(tbdn, 0, DrawInfo.wVSclMax);
			DrawInfo.dwVZoomedPos = DrawInfo.wVSclPos << DrawInfo.iVFit;
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_VERT, DrawInfo.wVSclPos, TRUE);
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
			dn = (rc.right - 2 * DIVLONG) / 16 + 1;
			break;
		case SB_LINEUP:
			dn = -(rc.right - 2 * DIVLONG) / 16 + 1;
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
			DrawInfo.dwHZoomedPos = BOUND(DrawInfo.dwHZoomedPos + dn, 0, DrawInfo.dwHZoomedWidth);
			DrawInfo.wHSclPos = DrawInfo.dwHZoomedPos >> DrawInfo.iHFit;
		}
		if (tbdn != 0)
		{
			//DbgMsg(("scroll tumb"));
			DrawInfo.wHSclPos = BOUND(tbdn, 0, DrawInfo.wHSclMax);
			DrawInfo.dwHZoomedPos = DrawInfo.wHSclPos << DrawInfo.iHFit;
		}
		if (dn != 0 || tbdn != 0)
		{
			SetScrollPos(hWnd, SB_HORZ, DrawInfo.wHSclPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		break;
	}
}

void CSignalWin::CaculateScrolls(void)
{
	typedef enum {
		notsure,
		nobar,
		havebar
	} barstate;
	barstate hbar, vbar;
	CData* Data = (CData*)DataOrignal;
	DrawInfo.dwHZoomedWidth = Data->Len;
	if (DrawInfo.iHZoom > 0) {
		DrawInfo.dwHZoomedWidth = DrawInfo.dwHZoomedWidth << DrawInfo.iHZoom;
		DrawInfo.dbHZoom = (double)(1 << DrawInfo.iHZoom);
	}
	else {
		DrawInfo.dwHZoomedWidth = DrawInfo.dwHZoomedWidth >> abs(DrawInfo.iHZoom);
		DrawInfo.dbHZoom = (double)1.0 / (1 << -DrawInfo.iHZoom);
	}

	if (DrawInfo.dwHZoomedWidth - DrawInfo.DrawWidth > 0) hbar = havebar;
	else if (DrawInfo.dwHZoomedWidth - (DrawInfo.DrawWidth - GetSystemMetrics(SM_CYHSCROLL)) < 0) hbar = nobar;
	else hbar = notsure;

	DrawInfo.dwVZoomedFullHeight = MoveBits(MoveBits(1, Data->DataBits), DrawInfo.iVZoom);
	DrawInfo.dwVZoomedHeight = DrawInfo.dwVZoomedFullHeight;
	DrawInfo.dbVZoom = DrawInfo.iVZoom > 0 ? (UINT64)1 << DrawInfo.iVZoom : (double)1.0 / ((UINT64)1 << -DrawInfo.iVZoom);

	if (DrawInfo.dwVZoomedHeight - DrawInfo.DrawHeight > 0) vbar = havebar;
	else if (DrawInfo.dwVZoomedHeight - (DrawInfo.DrawHeight - GetSystemMetrics(SM_CYVSCROLL)) < 0) vbar = nobar;
	else vbar = notsure;

	if (hbar == notsure && vbar == notsure) { hbar = nobar; vbar = nobar; }
	else if (hbar == havebar && vbar == notsure) vbar = havebar;
	else if (hbar == notsure && vbar == havebar) hbar = havebar;

	if (hbar == havebar) DrawInfo.DrawHeight = WinRect.bottom - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON - GetSystemMetrics(SM_CYHSCROLL);
	if (vbar == havebar) DrawInfo.DrawWidth = WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT - GetSystemMetrics(SM_CYVSCROLL);

	CaculateHScroll();
	CaculateVScroll();

	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);
}

void CSignalWin::CaculateHScroll(void)
{
	INT iRangeH, i;

	//DrawInfo.dwHZoomedWidth = DrawInfo.DataLength;
	//if (DrawInfo.iHZoom > 0) {
	//	DrawInfo.dwHZoomedWidth = DrawInfo.dwHZoomedWidth << DrawInfo.iHZoom;
	//	DrawInfo.dbHZoom = (double)(1 << DrawInfo.iHZoom);
	//}
	//else {
	//	DrawInfo.dwHZoomedWidth = DrawInfo.dwHZoomedWidth >> abs(DrawInfo.iHZoom);
	//	DrawInfo.dbHZoom = (double)1.0 / (1 << -DrawInfo.iHZoom);
	//}

	CData* oData = (CData*)DataOrignal;
	CData* fData = (CData*)DataFiltted;

	if (bAutoScroll) {
		DrawInfo.dwHZoomedPos = (bFollowByOrignal == true ? oData->Pos : fData->Pos) * DrawInfo.dbHZoom - DrawInfo.DrawWidth;
		if (DrawInfo.dwHZoomedPos < 0) DrawInfo.dwHZoomedPos = 0;
	}
	else {
		i = DrawInfo.iHOldZoom - DrawInfo.iHZoom;
		if (i > 0)
			DrawInfo.dwHZoomedPos = DrawInfo.dwHZoomedPos >> i;
		else
			DrawInfo.dwHZoomedPos = DrawInfo.dwHZoomedPos << -i;
	}
	DrawInfo.iHOldZoom = DrawInfo.iHZoom;

	// caculate scroll max width
	DrawInfo.dwHZoomedWidth = DrawInfo.dwHZoomedWidth - DrawInfo.DrawWidth;
	UP_TO_ZERO(DrawInfo.dwHZoomedWidth);
	for (DrawInfo.iHFit = 0; (DrawInfo.dwHZoomedWidth >> DrawInfo.iHFit) > 0xFFFF; DrawInfo.iHFit++);
	DrawInfo.wHSclMax = DrawInfo.dwHZoomedWidth >> DrawInfo.iHFit;
	DrawInfo.wHSclPos = DrawInfo.dwHZoomedPos >> DrawInfo.iHFit;
	DrawInfo.dwHZoomedPos = BOUND(DrawInfo.dwHZoomedPos, 0, DrawInfo.dwHZoomedWidth);
	DrawInfo.wHSclPos = BOUND(DrawInfo.wHSclPos, 0, DrawInfo.wHSclMax);

	iRangeH = DrawInfo.wHSclMax;

	if (iRangeH < 0) iRangeH = 0;

	SetScrollRange(hWnd, SB_HORZ, 0, iRangeH, TRUE);
	SetScrollPos(hWnd, SB_HORZ, DrawInfo.wHSclPos, TRUE);
	//InvalidateRect(hWnd, NULL, TRUE);
	//UpdateWindow(hWnd);
}

void CSignalWin::CaculateVScroll(void)
{
	INT i;

	//DrawInfo.dwVZoomedFullHeight = MoveBits(MoveBits(1, DrawInfo.DataBit), DrawInfo.iVZoom);
	//DrawInfo.dwVZoomedHeight = DrawInfo.dwVZoomedFullHeight;
	//DrawInfo.dbVZoom = DrawInfo.iVZoom > 0 ? (UINT64)1 << DrawInfo.iVZoom : (double)1.0 / ((UINT64)1 << -DrawInfo.iVZoom);

	i = DrawInfo.iVOldZoom - DrawInfo.iVZoom;
	if (i > 0) {
		DrawInfo.dwVZoomedPos = ((DrawInfo.dwVZoomedPos + DrawInfo.DrawHeight / 2) >> i) - DrawInfo.DrawHeight / 2;
	}
	else {
		DrawInfo.dwVZoomedPos = ((DrawInfo.dwVZoomedPos + DrawInfo.DrawHeight / 2) << -i) - DrawInfo.DrawHeight / 2;
	}
	DrawInfo.iVOldZoom = DrawInfo.iVZoom;

	// caculate scroll max height

	DrawInfo.dwVZoomedHeight -= DrawInfo.DrawHeight;
	UP_TO_ZERO(DrawInfo.dwVZoomedHeight);
	for (DrawInfo.iVFit = 0; (DrawInfo.dwVZoomedHeight >> DrawInfo.iVFit) > 0xFFFF; DrawInfo.iVFit++);
	DrawInfo.wVSclMax = DrawInfo.dwVZoomedHeight >> DrawInfo.iVFit;
	DrawInfo.wVSclPos = DrawInfo.dwVZoomedPos >> DrawInfo.iVFit;
	DrawInfo.dwVZoomedPos = BOUND(DrawInfo.dwVZoomedPos, 0, DrawInfo.dwVZoomedHeight);
	DrawInfo.wVSclPos = BOUND(DrawInfo.wVSclPos, 0, DrawInfo.wVSclMax);

	SetScrollRange(hWnd, SB_VERT, 0, DrawInfo.wVSclMax, TRUE);
	SetScrollPos(hWnd, SB_VERT, DrawInfo.wVSclPos, TRUE);
	//InvalidateRect(hWnd, NULL, TRUE);
	//UpdateWindow(hWnd);
}

void CSignalWin::Paint(void)
{
	HDC		hDC;
	PAINTSTRUCT ps;
	RECT	rt, r;
	HPEN	hPen, hPenLighter;

	CData* oData = (CData*)DataOrignal;
	CData* fData = (CData*)DataFiltted;

	hDC = BeginPaint(hWnd, &ps);

	GetClientRect(hWnd, &rt);

	HDC		hdc = CreateCompatibleDC(hDC);
	HBITMAP hbmp = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	SelectObject(hdc, hbmp);

	SelectObject(hdc, CreateFont(FONT_HEIGHT, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));

	FillRect(hdc, &rt, (HBRUSH)GetStockObject(BLACK_BRUSH));

	int	x, y, pos, i;
	TCHAR	s[1024];
	int		zoomX, zoomY;
	zoomX = 1;
	zoomY = 1;
	pos = 0;
	s[0] = 0;

	hPen = CreatePen(PS_DOT, 0, RGB(64, 64, 64));
	hPenLighter = CreatePen(PS_DOT, 0, RGB(128, 128, 128));
	SetBkColor(hdc, RGB(0, 0, 0));
	r.top = WAVE_RECT_BORDER_TOP - DIVLONG - FONT_HEIGHT;
	r.right = rt.right;
	r.bottom = rt.bottom;
	SetTextColor(hdc, RGB(255, 255, 255));

#define WEB_H_STEP	8
	UINT web_hoffset = WEB_H_STEP - DrawInfo.dwHZoomedPos % WEB_H_STEP;
	if (web_hoffset == WEB_H_STEP)web_hoffset = 0;
	UINT long_step = WEB_H_STEP * 4;
	for (x = WAVE_RECT_BORDER_LEFT + web_hoffset; x <= rt.right - WAVE_RECT_BORDER_RIGHT; x += WEB_H_STEP)
	{
		UINT64 realPos = DrawInfo.dwHZoomedPos + x - WAVE_RECT_BORDER_LEFT;
		UINT LineOffset = realPos % long_step ? DIVSHORT : DIVLONG;
		UINT64 lineNum = realPos / WEB_H_STEP;
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP - LineOffset, NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP);

		if ((lineNum % 4) == 0)	{
			if ((lineNum % 16) == 0) {
				UINT64 pos = (UINT64)(((double)realPos) / DrawInfo.dbHZoom);
				sprintf(s, "%lu", pos);
				r.left = x;
				r.top = WAVE_RECT_BORDER_TOP - DIVLONG - FONT_HEIGHT;
				DrawText(hdc, s, strlen(s), &r, NULL);
				//sprintf(s, "%.6fs", (double)pos / *SampleRate);
				formatKKDouble((double)pos / oData->SampleRate, "s", s);
				r.top = WAVE_RECT_BORDER_TOP + DrawInfo.DrawHeight + DIVLONG;
				DrawText(hdc, s, strlen(s), &r, NULL);
			}
			SelectObject(hdc, lineNum % 16 ? hPen : hPenLighter);
			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + DrawInfo.DrawHeight);
		}
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + DrawInfo.DrawHeight, NULL);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + DrawInfo.DrawHeight + LineOffset);
	}
	r.top = WAVE_RECT_BORDER_TOP - FONT_HEIGHT / 2;
	r.left = rt.right - WAVE_RECT_BORDER_RIGHT + DIVLONG + 2;
	r.right = rt.right;
	r.bottom = rt.bottom;
	UINT web_voffset = WEB_H_STEP - DrawInfo.dwVZoomedPos % WEB_H_STEP;
	if (web_voffset == WEB_H_STEP)web_voffset = 0;
	for (y = WAVE_RECT_BORDER_TOP + web_voffset; y <= WAVE_RECT_BORDER_TOP + DrawInfo.DrawHeight; y += WEB_H_STEP)
	{
		UINT64 realPos = DrawInfo.dwVZoomedPos + y - WAVE_RECT_BORDER_TOP;
		UINT LineOffset = realPos % long_step ? DIVSHORT : DIVLONG;
		UINT64 lineNum = realPos / WEB_H_STEP;
		MoveToEx(hdc, WAVE_RECT_BORDER_LEFT - LineOffset, y, NULL);
		LineTo(hdc, WAVE_RECT_BORDER_LEFT, y);
		if ((lineNum % 4) == 0)	{
			if (lineNum % 16 == 0) {
				formatKDouble(DrawInfo.FullVotage - DrawInfo.VotagePerDIV * (realPos / DrawInfo.dbVZoom),
					DrawInfo.VotagePerDIV / DrawInfo.dbVZoom, "v", s);
				r.top = y - (FONT_HEIGHT >> 1);
				DrawText(hdc, s, strlen(s), &r, NULL);
			}
			SelectObject(hdc, lineNum % 16 ? hPen : hPenLighter);
			LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, y);
		}
		SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		MoveToEx(hdc, rt.right - WAVE_RECT_BORDER_RIGHT, y, NULL);
		LineTo(hdc, rt.right - WAVE_RECT_BORDER_RIGHT + LineOffset, y);
	}
	DeleteObject(hPen);
	DeleteObject(hPenLighter);

	if (bDrawOrignalSignal && oData != NULL) {
		if (oData->Buff != NULL) {
			if (oData->DataType == short_type) DrawSignal_short(hdc, &rt, oData, RGB(0, 255, 0));
			if (oData->DataType == float_type) DrawSignal_float(hdc, &rt, oData, RGB(0, 255, 0));
		}
	}
	if (bDrawFilttedSignal && fData != NULL) {
		if (fData->Buff != NULL) {
			if (fData->DataType == short_type) DrawSignal_short(hdc, &rt, fData, RGB(255, 255, 0));
			if (fData->DataType == float_type) DrawSignal_float(hdc, &rt, fData, RGB(255, 255, 0));
		}
	}

	double z = DrawInfo.iVZoom >= 0 ? (1.0 / ((UINT64)1 << DrawInfo.iVZoom)) : ((UINT64)1 << -DrawInfo.iVZoom);
	char tstr1[100], tstr2[100], tstradcpos[100], tstrfiltpos[100], tstrhbarpos[100], tstrfs[100];
	//AdcData->NumPerSec = 38400;
	double TimePreDiv = 32.0 / oData->SampleRate *
		(DrawInfo.iHZoom >= 0 ? 1.0 / ((UINT64)1 << DrawInfo.iHZoom) : ((UINT64)1 << -DrawInfo.iHZoom));
	sprintf(s, "32/DIV %sV/DIV %ss/DIV\r\n"\
		"Pos:%s\r\nFilttedPos:%s\r\n"\
		"hBarPos=%s\r\n"\
		"SampleRate=%s RealAdcSampleRate=%d\r\n"\
		"HZoom: %d, %d \t VZoom: %d, %d",
		formatKKDouble(32.0 * DrawInfo.VotagePerDIV * z, "", tstr1),
		formatKKDouble(TimePreDiv, "", tstr2),
		fomatKINT64(oData != NULL ? oData->Pos : 0, tstradcpos),
		fomatKINT64(fData != NULL ? fData->Pos : 0, tstrfiltpos),
		fomatKINT64(DrawInfo.dwHZoomedPos, tstrhbarpos),
		fomatKINT64(oData->SampleRate, tstrfs), oData->NumPerSec,
		DrawInfo.iHZoom, DrawInfo.iHZoom >= 0 ? 1 << DrawInfo.iHZoom : -(1 << -DrawInfo.iHZoom),
		DrawInfo.iVZoom, DrawInfo.iVZoom >= 0 ? 1 << DrawInfo.iVZoom : -(1 << -DrawInfo.iVZoom)
	);

	r.top = WAVE_RECT_BORDER_TOP + DIVLONG;
	r.left = WAVE_RECT_BORDER_LEFT + DIVLONG;
	r.right = rt.right;
	r.bottom = rt.bottom;
	SetBkMode(hdc, TRANSPARENT);
	//	SetBkMode(hdc, OPAQUE); 
	//	SetBkColor(hdc,RGB(0,0,0));
	SetTextColor(hdc, RGB(255, 255, 255));

	DrawText(hdc, s, strlen(s), &r, NULL);
	/*
	sprintf(s,
		"HZoom: %d, %d \t VZoom: %d, %d",
		DrawInfo.iHZoom, DrawInfo.iHZoom >= 0 ? 1 << DrawInfo.iHZoom : -(1 << -DrawInfo.iHZoom),
		DrawInfo.iVZoom, DrawInfo.iVZoom >= 0 ? 1 << DrawInfo.iVZoom : -(1 << -DrawInfo.iVZoom)
	);
	r.top += FONT_HEIGHT * 6;
	DrawText(hdc, s, strlen(s), &r, NULL);

	sprintf(s, "vzoom %.40f", DrawInfo.dbVZoom);
	r.top += FONT_HEIGHT;
	DrawText(hdc, s, strlen(s), &r, NULL);
	*/

	DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

	BitBlt(hDC,
		0, 0,
		rt.right, rt.bottom,
		hdc,
		0, 0,
		SRCCOPY);
	DeleteObject(hdc);
	DeleteObject(hbmp);

	EndPaint(hWnd, &ps);
}

void CSignalWin::DrawSignal_short(HDC hdc, RECT *rt, void* Data, COLORREF Color)
{
	HPEN hPen;
	CData* cData = (CData*)Data;

	UINT dwWStep, dwXOffSet, bufStep, dwMark;
	if (DrawInfo.iHZoom > 0)
	{
		dwMark = (0xFFFFFFFF >> (32 - DrawInfo.iHZoom));
		dwWStep = 1 + dwMark;
		dwXOffSet = DrawInfo.dwHZoomedPos & dwMark;
		bufStep = 1;

	}
	else if (DrawInfo.iHZoom < 0)
	{
		dwWStep = 1;
		dwXOffSet = 0;
		bufStep = 1 + (0xFFFFFFFF >> (32 + DrawInfo.iHZoom));
	}
	else
	{
		dwWStep = 1;
		dwXOffSet = 0;
		bufStep = 1;
	}

	INT64 x, y;
	hPen = CreatePen(PS_SOLID, 1, Color);
	//SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
	SelectObject(hdc, hPen);
	x = WAVE_RECT_BORDER_LEFT - dwXOffSet;
	short* pData = (short*)cData->Buff +
		(
			DrawInfo.iHZoom > 0
			? DrawInfo.dwHZoomedPos >> DrawInfo.iHZoom
			: DrawInfo.dwHZoomedPos << (-DrawInfo.iHZoom)
			);
	y = DrawInfo.dwVZoomedFullHeight / 2 - (*pData * DrawInfo.dbVZoom) - DrawInfo.dwVZoomedPos;
	y = BOUND(y, 0, DrawInfo.DrawHeight);
	MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
	do
	{
		pData += bufStep;
		x += dwWStep;
		y = DrawInfo.dwVZoomedFullHeight / 2 - (*pData * DrawInfo.dbVZoom) - DrawInfo.dwVZoomedPos;
		y = BOUND(y, 0, DrawInfo.DrawHeight);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + y);

		if (bufStep == 1) {
			if ((pData == ((short*)cData->Buff + (cData->Pos & cData->Mask)))) {
				MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
				LineTo(hdc, x, WAVE_RECT_BORDER_TOP + DrawInfo.DrawHeight);
				MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
			}
		}
		else if (pData >= ((short*)cData->Buff + (cData->Pos & cData->Mask)) && (pData - bufStep) <= ((short*)cData->Buff + (cData->Pos & cData->Mask))) {
			MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + DrawInfo.DrawHeight);
			MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
		}

	} while (x < rt->right - WAVE_RECT_BORDER_RIGHT);
	DeleteObject(hPen);
}

void CSignalWin::DrawSignal_float(HDC hdc, RECT* rt, void* Data, COLORREF Color)
{
	HPEN hPen;
	CData* cData = (CData*)Data;

	UINT dwWStep, dwXOffSet, bufStep, dwMark;
	if (DrawInfo.iHZoom > 0)
	{
		dwMark = (0xFFFFFFFF >> (32 - DrawInfo.iHZoom));
		dwWStep = 1 + dwMark;
		dwXOffSet = DrawInfo.dwHZoomedPos & dwMark;
		bufStep = 1;

	}
	else if (DrawInfo.iHZoom < 0)
	{
		dwWStep = 1;
		dwXOffSet = 0;
		bufStep = 1 + (0xFFFFFFFF >> (32 + DrawInfo.iHZoom));
	}
	else
	{
		dwWStep = 1;
		dwXOffSet = 0;
		bufStep = 1;
	}
	INT64 x, y;
	hPen = CreatePen(PS_SOLID, 1, Color);
	//SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
	SelectObject(hdc, hPen);
	x = WAVE_RECT_BORDER_LEFT - dwXOffSet;
	float* pData = (float*)cData->Buff +
		( DrawInfo.iHZoom > 0
			? DrawInfo.dwHZoomedPos >> DrawInfo.iHZoom
			: DrawInfo.dwHZoomedPos << (-DrawInfo.iHZoom)
			);
	y = DrawInfo.dwVZoomedFullHeight / 2 - (*pData * DrawInfo.dbVZoom) - DrawInfo.dwVZoomedPos;
	y = BOUND(y, 0, DrawInfo.DrawHeight);
	MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
	do
	{
		pData += bufStep;
		x += dwWStep;
		y = DrawInfo.dwVZoomedFullHeight / 2 - (*pData * DrawInfo.dbVZoom) - DrawInfo.dwVZoomedPos;
		y = BOUND(y, 0, DrawInfo.DrawHeight);
		LineTo(hdc, x, WAVE_RECT_BORDER_TOP + y);
		if (bufStep == 1) {
			if ((pData == ((float*)cData->Buff + (cData->Pos & cData->Mask)))) {
				MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
				LineTo(hdc, x, WAVE_RECT_BORDER_TOP + DrawInfo.DrawHeight);
				MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
			}
		}
		else if (pData >= ((float*)cData->Buff + (cData->Pos & cData->Mask)) && (pData - bufStep) <= ((float*)cData->Buff + (cData->Pos & cData->Mask))) {
			MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP, NULL);
			LineTo(hdc, x, WAVE_RECT_BORDER_TOP + DrawInfo.DrawHeight);
			MoveToEx(hdc, x, WAVE_RECT_BORDER_TOP + y, NULL);
		}

	} while (x < rt->right - WAVE_RECT_BORDER_RIGHT);
	DeleteObject(hPen);
}

void CSignalWin::RegisterWindowsClass(void)
{
	static bool registted = false;
	if (registted == true) return;
	registted = true;

	DbgMsg("CSignalWin::RegisterWindowsClass\r\n");

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)CSignalWin::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_MYWAVE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = (LPCSTR)NULL;
	wcex.lpszClassName = SIGNAL_WIN_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);
}

LRESULT CALLBACK CSignalWin::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CSignalWin* me = (CSignalWin*)get_WinClass(hWnd);

	switch (message)
	{
	case WM_CREATE:
	{
		OPENCONSOLE;

		me = (CSignalWin*)set_WinClass(hWnd, lParam);

		me->hWnd = hWnd;
		me->RestoreValue();
		me->uTimerId = SetTimer(hWnd, 0, TIMEOUT, NULL);
		//KillTimer(hWnd, uTimerId);

		CheckMenuItem(me->hMenu, IDM_WAVEAUTOSCROLL,
			(me->bAutoScroll ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		CheckMenuItem(me->hMenu, IDM_WAVE_FOLLOW_ORIGNAL,
			(me->bFollowByOrignal ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
	}
	break;
	case WM_CHAR:
		DbgMsg("CSignalWin WM_CHAR\r\n");
		PostMessage(GetParent(me->hWnd), message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		//clsWinSpect.ActivehWnd = NULL;
		break;
	case WM_LBUTTONUP:
		break;
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		me->MouseX = GET_X_LPARAM(lParam);
		me->MouseY = GET_Y_LPARAM(lParam);
		me->OnMouse();
		break;
	case WM_TIMER:
		me->CaculateScrolls();
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);
		break;
	case WM_SIZE:
		printf("WM_SIZE\r\n");
		me->GetRealClientRect(&me->WinRect);
		me->DrawInfo.DrawHeight = me->WinRect.bottom - WAVE_RECT_BORDER_TOP - WAVE_RECT_BORDER_BOTTON;
		UP_TO_ZERO(me->DrawInfo.DrawHeight);
		me->DrawInfo.DrawWidth = me->WinRect.right - WAVE_RECT_BORDER_LEFT - WAVE_RECT_BORDER_RIGHT;
		UP_TO_ZERO(me->DrawInfo.DrawWidth);
		me->CaculateScrolls();
		break;
	case WM_COMMAND:
		return me->OnCommand(message, wParam, lParam);
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
	case WM_CLOSE:
		DbgMsg("%s CSignalWin WM_CLOSE\r\n", me->Tag);
		break;
	case WM_DESTROY:
		DbgMsg("%s CSignalWin WM_DESTROY\r\n", me->Tag);
		me->hWnd = NULL;
		me->SaveValue();
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

bool CSignalWin::OnCommand(UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);

	switch (wmId)
	{
	case IDM_WAVEHZOOMRESET:
		DrawInfo.iHZoom = 0;
		CaculateScrolls();
		break;
	case IDM_WAVEHZOOMINCREASE:
		if (DrawInfo.iHZoom < SIGNAL_DATA_MAX_ZOOM_BIT) {
			DrawInfo.iHZoom++;
			CaculateScrolls();
		}
		break;
	case IDM_WAVEHZOOMDECREASE:
	{
		INT z = 0;
		UINT wd = ((CData*)DataOrignal)->Len;
		while ((wd - 1) > DrawInfo.DrawWidth) { z--; wd = wd >> 1; }
		if (DrawInfo.iHZoom > z) {
			DrawInfo.iHZoom--;
			CaculateScrolls();
		}
	}
	break;
	case IDM_WAVEVZOOMRESET:
		DrawInfo.iVZoom = 0;
		CaculateScrolls();
		break;
	case IDM_WAVEVZOOMINCREASE:
		if (DrawInfo.iVZoom < SIGNAL_DATA_MAX_ZOOM_BIT) {
			DrawInfo.iVZoom++;
			CaculateScrolls();
		}
		break;
	case IDM_WAVEVZOOMDECREASE:
	{
		INT z = 0;
		UINT64 hd = (UINT64)1 << ((CData*)DataOrignal)->DataBits;
		while ((hd - 1) > DrawInfo.DrawHeight) { z--; hd = hd >> 1; }
		if (DrawInfo.iVZoom > z) {
			DrawInfo.iVZoom--;
			CaculateScrolls();
		}
	}
	break;
	case IDM_WAVEAUTOSCROLL:
		bAutoScroll = !bAutoScroll;
		CheckMenuItem(hMenu, IDM_WAVEAUTOSCROLL,
			(bAutoScroll ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
	case IDM_WAVE_FOLLOW_ORIGNAL:
		bFollowByOrignal = !bFollowByOrignal;
		CheckMenuItem(hMenu, IDM_WAVE_FOLLOW_ORIGNAL,
			(bFollowByOrignal ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		break;
		//Setting COMMANDS-----------------------
	case IDM_SPECTRUM_PAUSE_BREAK:
		break;
	case IDM_EXIT:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

void CSignalWin::OnMouse(void)
{
	RECT rt;
	GetClientRect(hWnd, &rt);

	double X = 0, Y = 0;
	int n = 0;
	n += sprintf(strMouse + n, "X: %d, core V: %lf", X, Y);
}

void CSignalWin::GetRealClientRect(PRECT lprc)
{
	DWORD dwStyle;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	GetClientRect(hWnd, lprc);
	if (dwStyle & WS_HSCROLL)
		lprc->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (dwStyle & WS_VSCROLL)
		lprc->right += GetSystemMetrics(SM_CXVSCROLL);
}

void CSignalWin::SaveValue(void)
{
#define VALUE_LENGTH	100
	char section[VALUE_LENGTH];
	sprintf(section, "%s_CSignalWin", Tag);
	WritePrivateProfileString(section, "iHZoom", std::to_string(DrawInfo.iHZoom).c_str(), IniFilePath);
	WritePrivateProfileString(section, "iVZoom", std::to_string(DrawInfo.iVZoom).c_str(), IniFilePath);
	WritePrivateProfileString(section, "iVOldZoom", std::to_string(DrawInfo.iVOldZoom).c_str(), IniFilePath);
	WritePrivateProfileString(section, "dwVZoomedPos", std::to_string(DrawInfo.dwVZoomedPos).c_str(), IniFilePath);
}

void CSignalWin::RestoreValue(void)
{
#define VALUE_LENGTH	100
	char value[VALUE_LENGTH];
	char section[VALUE_LENGTH];
	sprintf(section, "%s_CSignalWin", Tag);
	GetPrivateProfileString(section, "iHZoom", "0", value, VALUE_LENGTH, IniFilePath);
	DrawInfo.iHZoom = atoi(value);
	GetPrivateProfileString(section, "iVZoom", "0", value, VALUE_LENGTH, IniFilePath);
	DrawInfo.iVZoom = atoi(value);
	GetPrivateProfileString(section, "iVOldZoom", "0", value, VALUE_LENGTH, IniFilePath);
	DrawInfo.iVOldZoom = atoi(value);
	GetPrivateProfileString(section, "dwVZoomedPos", "0", value, VALUE_LENGTH, IniFilePath);
	DrawInfo.dwVZoomedPos = std::strtoll(value, NULL, 10);
}