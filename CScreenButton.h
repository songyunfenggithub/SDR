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

    typedef enum BUTTON_ALIGN_MODE_ENUM {
        Button_Align_Left,
        Button_Align_Right,
        Button_Align_Center
    }BUTTON_ALIGN_MODE;

    typedef enum BUTTON_ACTION_MODE_ENUM {
        Button_Mouse_Num1,
        Button_Mouse_Num2,
        Button_Step_Num,
        Button_Confirm
    }BUTTON_ACTION_MODE;

    typedef enum BUTTON_MOUSE_ACTION_TYPE_ENUM {
        Button_Mouse_None,
        Button_Mouse_Left,
        Button_Mouse_Right,
        Button_Mouse_DOUBLE_Left,
        Button_Mouse_DOUBLE_Right
    }BUTTON_MOUSE_ACTION_TYPE;
    
    typedef enum BUTTON_FONT_SIZE_ENUM {
        Button_Font_Size_16 = 0,
        Button_Font_Size_32 = 1
    }BUTTON_FONT_SIZE;

    struct BUTTON {
        int     X, Y, W, H;
        int     MouseIndex = -1;
        RECT    *rt = NULL;
        int     txtNum;
        char    *tilte;
        char    txt[50];
        COLORREF color = RGB(255, 255, 255);
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
    bool RefreshMouseNumButton(INT64 value);
    void OnMouseMouseNumButton(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void DrawMouseNumButton(HDC hdc);
    void DrawButton(HDC hdc);
    void OnMouseButton(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    bool MouseInButton(HWND hWnd, UINT x, UINT y);
};

