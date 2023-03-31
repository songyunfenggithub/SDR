#pragma once
#include <windows.h>
#include <stdlib.h>

class CScreenButton
{
public:

    typedef struct BUTTON;

    typedef void (*ButtonDraw)(BUTTON* me, HDC hdc, RECT* srcRt);
    typedef void (*ButtonOnMouse)(BUTTON* me, UINT message, WPARAM wParam, LPARAM lParam);

    typedef enum {
        Button_Align_Left,
        Button_Align_Right
    }BUTTON_ALIGN_MODE;

    struct BUTTON {
        int     X, Y, W, H;
        char    txt[50];
        int     fontsize;
        int     key;
        bool    status;
        INT64     value, min, max;
        BUTTON_ALIGN_MODE     alginMode;

        ButtonDraw        Draw;
        ButtonOnMouse     OnMouse;
    };

    BUTTON *Buttons = NULL;
    UINT NumButton = 0;

    CScreenButton();
    ~CScreenButton();

    void Draw(HDC hdc, RECT* srcRt);

};

