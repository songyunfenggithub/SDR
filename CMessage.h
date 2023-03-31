#pragma once

#include "stdafx.h"
#include "windows.h"
#include "Windowsx.h"

#define MSG_LENGTH          0x20
#define MSG_LENGTH_MASK     (MSG_LENGTH - 1)

#define MSGDELAY    0x80
#define MSGTXTX     200
#define MSGTXTY     50

class CMessage
{
public:
    bool show = true;

    typedef enum {
        MSG_MODE_NORMAL = 0,
        MSG_MODE_ERROR = 1,
        MSG_MODE_DEBUG = 2
    }MSG_MODE;

    typedef enum {
        MSG_COLOR_WHITE = RGB(255, 255, 255),
        MSG_COLOR_RED = RGB(255, 255, 255),
        MSG_COLOR_GREEN = RGB(255, 255, 255)
    }MSG_COLOR;

    typedef struct tagMSG {
        MSG_MODE mode;
        int Tick;
        char* msg;
    }MSG;
    static MSG_COLOR MsgColors[];
    static const char* MsgModeStrings[];


    MSG mMsgs[MSG_LENGTH];
    int MsgCount = 0;
    bool mMsgOnOff = true;

    CMessage();
    ~CMessage();

    void add_msg(char* msg, MSG_MODE Mode);
    void Msg_On(void);
    void Msg_Off(void);
    void Msg_Reset(void);
    void Msg_Clear(void);
    void Msg_Tick(void);
    void MsgDraw(HWND hWnd, int X, int Y);

};

