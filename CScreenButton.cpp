#include "stdafx.h"
#include <stdio.h>
#include <windowsx.h>
#include "CScreenButton.h"
#include <wingdi.h>

const char ButtonFont[] = "courier";

CScreenButton::CScreenButton()
{

}

CScreenButton::~CScreenButton()
{

}

void CScreenButton::Draw(HDC hdc, RECT* srcRt)
{
	if (Buttons == NULL) return;
	CScreenButton::BUTTON* b;
	SIZE sizeText;
	HFONT hFont;
	HFONT hFontDefault;
	RECT rt;

	HPEN hPen = (HPEN)GetStockObject(WHITE_PEN);
	SetBkMode(hdc, TRANSPARENT); 

	for (int i = 0; i < NumButton; i++)
	{	

		b = &Buttons[i];

		hFont = CreateFont(b->fontsize, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("courier"));
		hFontDefault = (HFONT)SelectObject(hdc, hFont);

		GetTextExtentPoint32(hdc, b->txt, strlen(b->txt), &sizeText);
		b->W = sizeText.cx;
		b->H = sizeText.cy;

		rt.top = srcRt->top + b->Y;
		rt.left = b->alginMode == BUTTON_ALIGN_MODE::Button_Align_Right ?  srcRt->right - b->X - b->W : srcRt->left + b->X;
		rt.right = rt.left + b->W;
		rt.bottom = rt.top + b->H;

		MoveToEx(hdc, rt.left, rt.top, NULL);
		LineTo(hdc, rt.right, rt.top);
		LineTo(hdc, rt.right, rt.bottom);
		LineTo(hdc, rt.left, rt.bottom);
		LineTo(hdc, rt.left, rt.top);

		DrawText(hdc, b->txt, strlen(b->txt), &rt, NULL);

		SelectObject(hdc, hFontDefault);
		DeleteObject(hFont);
	}

	SelectObject(hdc, hPen);
}