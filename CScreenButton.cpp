#include "stdafx.h"
#include <stdio.h>
#include <windowsx.h>
#include <wingdi.h>

#include "public.h"
#include "CAnalyze.h"
#include "CWinOneFFT.h"
#include "CWinSpectrum.h"
#include "CScreenButton.h"

const char ButtonFont[] = "courier";

HFONT CScreenButton::hFont_16 = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, ButtonFont);
HFONT CScreenButton::hFont_32 = CreateFont(32, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, ButtonFont);
//const HFONT* CScreenButton::hFonts = { &hFont_16, &hFont_32 };

CScreenButton::CScreenButton(BUTTON* Button)
{
	this->Button = Button;
}

CScreenButton::CScreenButton()
{

}

CScreenButton::~CScreenButton()
{

}

void CScreenButton::SetValue(INT64 value)
{
	Button->value = BOUND(value, Button->min, Button->max);
}

void CScreenButton::Draw(HDC hdc, RECT* srcRt)
{
	if (Button == NULL) return;
	CScreenButton::BUTTON* b;
	SIZE sizeText;
	HFONT hFontDefault;
	RECT rt;

	HPEN hPen = (HPEN)GetStockObject(WHITE_PEN);
	SetBkMode(hdc, TRANSPARENT); 

	b = Button;

	HFONT hFont = *CScreenButton::hFonts[b->fontsize];
	hFontDefault = (HFONT)SelectObject(hdc, hFont);

	GetTextExtentPoint32(hdc, b->txt, strlen(b->txt), &sizeText);
	b->W = sizeText.cx;
	b->H = sizeText.cy;

	rt.top = srcRt->top + b->Y;
	rt.left = b->alginMode == BUTTON_ALIGN_MODE::Button_Align_Right ? srcRt->right - b->X - b->W : srcRt->left + b->X;
	rt.right = rt.left + b->W;
	rt.bottom = rt.top + b->H;

	MoveToEx(hdc, rt.left, rt.top, NULL);
	LineTo(hdc, rt.right, rt.top);
	LineTo(hdc, rt.right, rt.bottom);
	LineTo(hdc, rt.left, rt.bottom);
	LineTo(hdc, rt.left, rt.top);

	DrawText(hdc, b->txt, strlen(b->txt), &rt, NULL);

	SelectObject(hdc, hFontDefault);
	SelectObject(hdc, hPen);
}

void CScreenButton::ButtonInit(HWND hWnd)
{
	char txt[50];
	HDC hdc = GetDC(hWnd);
	HFONT hFont = *CScreenButton::hFonts[Button->fontsize];
	HFONT hFontDefault = (HFONT)SelectObject(hdc, hFont);
	SIZE sizeText;
	memset(txt, 'T', 50);
	txt[Button->txtNum] = '\0';
	GetTextExtentPoint32(hdc, txt, strlen(txt), &sizeText);
	Button->W = sizeText.cx;
	Button->H = sizeText.cy;
	SelectObject(hdc, hFontDefault);
	ReleaseDC(hWnd, hdc);
}

void CScreenButton::RefreshMouseNumButton(INT64 value)
{
	if (value >= Button->min && value <= Button->max) {
		Button->value = value;
		char s[100];
		char* s1 = fomatKINT64Width(Button->value, 4, s);
		strcpy(Button->txt, s1 + 2);
	}
}

void CScreenButton::OnMouseMouseNumButton(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT MouseX = GET_X_LPARAM(lParam);
	UINT MouseY = GET_Y_LPARAM(lParam);
	RECT rt;

	GetClientRect(hWnd, &rt);
	rt.top = WAVE_RECT_BORDER_TOP + Button->Y;
	rt.left = Button->alginMode == CScreenButton::BUTTON_ALIGN_MODE::Button_Align_Right ?
		rt.right - WAVE_RECT_BORDER_RIGHT - Button->X - Button->W :
		WAVE_RECT_BORDER_LEFT + Button->X;
	rt.right = rt.left + Button->W;
	rt.bottom = rt.top + Button->H;

	if (MouseX > rt.left && MouseX < rt.right && MouseY > rt.top && MouseY < rt.bottom) {
		int W = Button->W / 13;
		int index = (rt.right - MouseX) / W;
		if ((index + 1) % 4 == 0) return;
		int index2;
		/*
		if (index < 3) index2 = index;
		else if (index == 3) return;
		else if (index < 7)index2 = index - 1;
		else if (index == 7)return;
		else if (index < 11) index2 = index - 2;
		else if (index == 11) return;
		else index2 = index - 3;
		*/
		index2 = index - (index + 1) / 4;

		double rfstep = pow(10, index2);
		INT64 savevalue = Button->value;
		switch (message)
		{
		case WM_LBUTTONUP:
			Button->value += rfstep;
			if (Button->value > Button->max) Button->value = Button->max;
			break;
		case WM_RBUTTONUP:
			Button->value -= rfstep;
			if (Button->value < Button->min) Button->value = Button->min;
			break;
		case WM_MOUSEMOVE:
			break;
		default:
			break;
		}
		if (savevalue != Button->value) {
			if (Button->mode == Button_Mouse_Num1) {
				clsAnalyze.set_SDR_rfHz(Button->value);
			}
			else if (Button->mode == Button_Mouse_Num2) {
				clsAnalyze.rfHz_Step = Button->value;
			}

			char s[100];
			char* s1 = fomatKINT64Width(Button->value, 4, s);
			strcpy(Button->txt, s1 + 2);

		}
	}
}

bool CScreenButton::MouseInButton(HWND hWnd, UINT x, UINT y)
{
	RECT rt;
	GetClientRect(hWnd, &rt);
	rt.top = WAVE_RECT_BORDER_TOP + Button->Y;
	rt.left = Button->alginMode == CScreenButton::BUTTON_ALIGN_MODE::Button_Align_Right ?
		rt.right - WAVE_RECT_BORDER_RIGHT - Button->X - Button->W :
		WAVE_RECT_BORDER_LEFT + Button->X;
	rt.right = rt.left + Button->W;
	rt.bottom = rt.top + Button->H;
	return (x > rt.left && x < rt.right && y > rt.top && y < rt.bottom);
}

void CScreenButton::OnMouseButton(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT MouseX = GET_X_LPARAM(lParam);
	UINT MouseY = GET_Y_LPARAM(lParam);
	if (MouseInButton(hWnd, MouseX, MouseY) == true) {
		INT64 savevalue = Button->value;
		INT64 v;
		switch (message)
		{
		case WM_LBUTTONUP:
			v = Button->value + Button->step;
			Button->value = BOUND(v, Button->min, Button->max);
			Button->mouse_action = Button_Mouse_Left;
			break;
		case WM_RBUTTONUP:
			v = Button->value - Button->step;
			Button->value = BOUND(v, Button->min, Button->max);
			Button->mouse_action = Button_Mouse_Right;
			break;
		case WM_MOUSEMOVE:
			Button->mouse_action = Button_Mouse_None;
			break;
		default:
			break;
		}
		if (Button->mode == Button_Step_Num) {
			if (savevalue != Button->value) {
				if (Button->buttonFunc != NULL) Button->buttonFunc(this);
			}
		}
		else {
			if (Button->mode == Button_Confirm && Button->buttonFunc != NULL) Button->buttonFunc(this);
		}
	}
}

void CScreenButton::DrawMouseNumButton(HDC hdc, RECT* srcRt)
{
	HFONT hFont = *CScreenButton::hFonts[Button->fontsize];
	HFONT hFont_old = (HFONT)SelectObject(hdc, hFont);

	HPEN hPen = (HPEN)GetStockObject(WHITE_PEN);
	HPEN hPen_old = (HPEN)SelectObject(hdc, hPen);
	SetBkMode(hdc, TRANSPARENT);

	RECT rt;
	rt.top = srcRt->top + Button->Y;
	rt.left = Button->alginMode == CScreenButton::BUTTON_ALIGN_MODE::Button_Align_Right ?
		srcRt->right - Button->X - Button->W : srcRt->left + Button->X;
	rt.right = rt.left + Button->W;
	rt.bottom = rt.top + Button->H;

	MoveToEx(hdc, rt.left, rt.top, NULL);
	LineTo(hdc, rt.right, rt.top);
	LineTo(hdc, rt.right, rt.bottom);
	LineTo(hdc, rt.left, rt.bottom);
	LineTo(hdc, rt.left, rt.top);

	DrawText(hdc, Button->txt, strlen(Button->txt), &rt, NULL);

	SelectObject(hdc, hFont_old);
	SelectObject(hdc, hPen_old);
}

void CScreenButton::DrawButton(HDC hdc, RECT* srcRt)
{

	HFONT hFont = *CScreenButton::hFonts[Button->fontsize];
	HFONT hFont_old = (HFONT)SelectObject(hdc, hFont);
	HPEN hPen = (HPEN)GetStockObject(WHITE_PEN);
	HPEN hPen_old = (HPEN)SelectObject(hdc, hPen);

	SetBkMode(hdc, TRANSPARENT);

	RECT rt;
	rt.top = srcRt->top + Button->Y;
	rt.left = Button->alginMode == CScreenButton::BUTTON_ALIGN_MODE::Button_Align_Right ?
		srcRt->right - Button->X - Button->W : srcRt->left + Button->X;
	rt.right = rt.left + Button->W;
	rt.bottom = rt.top + Button->H;

	MoveToEx(hdc, rt.left, rt.top, NULL);
	LineTo(hdc, rt.right, rt.top);
	LineTo(hdc, rt.right, rt.bottom);
	LineTo(hdc, rt.left, rt.bottom);
	LineTo(hdc, rt.left, rt.top);

	char txt[50];
	sprintf(txt, "%s%02d", Button->txt, (int)Button->value);
	DrawText(hdc, txt, strlen(txt), &rt, NULL);

	SelectObject(hdc, hFont_old);
	SelectObject(hdc, hPen_old);
}
