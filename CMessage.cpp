#include "CMessage.h"


const char* CMessage::MsgModeStrings[] = {
            "MSG_MODE_NORMAL",
            "MSG_MODE_NORMAL",
            "MSG_MODE_ERROR"
};

CMessage::MSG_COLOR CMessage::MsgColors[] = {
    MSG_COLOR_WHITE,
    MSG_COLOR_RED,
    MSG_COLOR_GREEN
};

CMessage::CMessage()
{
}
    
CMessage::~CMessage()
{
}


void CMessage::add_msg(char* msg, MSG_MODE Mode)
{
//    Log.d(MyApp.TAG, msg + " " + MsgModeStrings[Mode]);
    if (!show) return;
    int i = MsgCount;
    mMsgs[i].mode = Mode;
    mMsgs[i].Tick = MSGDELAY;
    mMsgs[i].msg = msg;
    MsgCount++;
    if (MsgCount == MSG_LENGTH) MsgCount = 0;

}

void CMessage::Msg_On(void)
{
    mMsgOnOff = true;
}

void CMessage::Msg_Off(void)
{
    mMsgOnOff = false;
}

void CMessage::Msg_Reset(void) 
{
        int i;
        for (i = 0; i < MSG_LENGTH; i++)mMsgs[i].Tick = MSGDELAY;
    }

void CMessage::Msg_Clear(void)
{
        int i;
        for (i = 0; i < MSG_LENGTH; i++)mMsgs[i].Tick = 0;
    }

void CMessage::Msg_Tick(void) 
{
        int i;
        for (i = 0; i < MSG_LENGTH; i++)
        {
            if (mMsgs[i].Tick > 0)
            {
                mMsgs[i].Tick--;
            }
        }
    }

int Msg_Line_Hieght = 27;
void CMessage::MsgDraw(HWND hWnd, int X, int Y)
{
    if (mMsgOnOff) { 
        HDC hDC = GetDC(hWnd);
        PAINTSTRUCT ps;
        RECT r, rt;
        SelectObject(hDC, CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, _T("Arial")));
        HPEN hPen = CreatePen(PS_DOT, 0, RGB(64, 64, 64));
        GetClientRect(hWnd, &rt);
        //SetBkColor(hDC, COLOR_TEXT_BACKGOUND);
        SetBkMode(hDC, TRANSPARENT);
        //	SetBkMode(hdc, OPAQUE); 
        //	SetBkColor(hdc,COLOR_TEXT_BACKGOUND);
        FillRect(hDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));

        for (int i = 0; i < MSG_LENGTH; i++) {
            if (mMsgs[i].Tick > 0) {
                mMsgs[i].Tick--;
                r.top = Y;
                r.left = X;
                r.right = rt.right - X;
                r.bottom = rt.top + 20;
                SetTextColor(hDC, MsgColors[mMsgs[i].mode]);
                DrawText(hDC, mMsgs[i].msg, strlen(mMsgs[i].msg), &r, NULL);
                if (i == MsgCount - 1)
                {
                    r.left -= 10;
                    DrawText(hDC, "O", 1, &r, NULL);
                    r.left += 10;
                }
            }
        }

        DeleteObject(SelectObject(hDC, GetStockObject(SYSTEM_FONT)));
        DeletePen(hPen);
        ReleaseDC(hWnd, hDC);

    }
}
