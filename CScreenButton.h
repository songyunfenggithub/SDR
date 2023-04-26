#pragma once
#include <windows.h>
#include <stdlib.h>

class CScreenButton
{
public:

    typedef struct BUTTON;
    typedef void (*ButtonDrawFunc)(BUTTON* me, HDC hdc, RECT* srcRt);
    typedef void (*ButtonOnMouseFunc)(BUTTON* me, UINT message, WPARAM wParam, LPARAM lParam);
    typedef void (*ButtonFunc)(CScreenButton* button);

    typedef enum {
        Button_Align_Left,
        Button_Align_Right
    }BUTTON_ALIGN_MODE;

    typedef enum {
        Button_Mouse_Num1,
        Button_Mouse_Num2,
        Button_Step_Num,
        Button_Confirm
    }BUTTON_ACTION_MODE;

    typedef enum {
        Button_Mouse_None,
        Button_Mouse_Left,
        Button_Mouse_Right,
        Button_Mouse_DOUBLE_Left,
        Button_Mouse_DOUBLE_Right
    }BUTTON_MOUSE_ACTION_TYPE;
    
    typedef enum {
        Button_Font_Size_16 = 0,
        Button_Font_Size_32 = 1
    }BUTTON_FONT_SIZE;

    struct BUTTON {
        int     X, Y, W, H;
        int     txtNum;
        char    *tilte;
        char    txt[50];
        BUTTON_FONT_SIZE fontsize;
        BUTTON_ACTION_MODE mode;
        BUTTON_MOUSE_ACTION_TYPE mouse_action;
        bool    status;
        INT64     value, step, min, max;
        BUTTON_ALIGN_MODE     alginMode;

        ButtonDrawFunc        Draw;
        ButtonOnMouseFunc     OnMouse;
        ButtonFunc            buttonFunc;
    };

    BUTTON *Button = NULL;

    static HFONT hFont_32, hFont_16;
    constexpr static const HFONT* hFonts[] = { &hFont_16, &hFont_32 };
    
public:
    CScreenButton();
    CScreenButton(BUTTON* pButton);
    ~CScreenButton();

    void Draw(HDC hdc, RECT* srcRt);
    void SetValue(INT64 value);
    void ButtonInit(HWND hWnd);
    void RefreshMouseNumButton(INT64 value);
    void OnMouseMouseNumButton(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void DrawMouseNumButton(HDC hdc, RECT* srcRt);
    void DrawButton(HDC hdc, RECT* srcRt);
    void OnMouseButton(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    bool MouseInButton(HWND hWnd, UINT x, UINT y);
};

