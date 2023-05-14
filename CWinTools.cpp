


#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


#include "stdafx.h"
#include "Windows.h"
#include "Winuser.h"
#include "stdio.h"
#include "resource.h"
#include <limits>
#include <iostream>

#include "public.h"
#include "Debug.h"
#include "CData.h"
#include "CFFT.h"

#include "CWinMain.h"
#include "CDemodulatorAM.h"
#include "CAudio.h"
#include "CWinFFT.h"
#include "CWinSignal.h"
#include "CWinTools.h"

#pragma comment(lib, "comctl32.lib")

using namespace WINS;
using namespace METHOD;



#define ID_TOOLBAR 10
#define BUTTONWIDTH     16       // Width of the button in pixels
#define BUTTONHEIGHT    20      // Height of the button in pixels
#define IMAGEWIDTH      0     // Button image width in pixels
#define IMAGEHEIGHT     0       // Button image height in pixels
#define NUMIMAGES 0


CWinTools clsWinTools;

CWinTools::CWinTools()
{
    OPENCONSOLE_SAVED;
    Init();
}

CWinTools::~CWinTools()
{
    UnInit();
    //CLOSECONSOLE;
}

const char TAG[] = "CToolsWin";

void CWinTools::Init(void)
{
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    InitCommonControls();
}

void CWinTools::UnInit(void)
{
}

HWND CWinTools::CreateToolbar(HWND hWnd, LPTBBUTTON tbb, UINT numButtons, TOOL_TIPS* tips, UINT numTips)
{
    int iCBHeight;              // Height of the command bar 
    DWORD dwStyle;              // Style of the toolbar
    HWND hwndTB = NULL;         // Handle to the command bar control 
    //TBBUTTON tbb[4];
    //ZeroMemory(tbb, sizeof(tbb));
    //tbb[0].iBitmap = 100;
    //tbb[0].idCommand = 0;
    //tbb[0].fsState = TBSTATE_ENABLED;
    //tbb[0].fsStyle = TBSTYLE_SEP;
    //tbb[0].iString = (INT_PTR)TEXT("");
    //tbb[1].iBitmap = MAKELONG(0, 0);//MAKELONG(index,0);
    //tbb[1].idCommand = 0;
    //tbb[1].fsState = TBSTATE_ENABLED;
    //tbb[1].fsStyle = TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE;
    //tbb[1].iString = (INT_PTR)TEXT("A");
    //tbb[2].iBitmap = MAKELONG(0, 0);//MAKELONG(index,0);
    //tbb[2].idCommand = 1;
    //tbb[2].fsState = TBSTATE_ENABLED;
    //tbb[2].fsStyle = TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE;
    //tbb[2].iString = (INT_PTR)TEXT("B");
    //tbb[3].iBitmap = 100;
    //tbb[3].idCommand = 0;
    //tbb[3].fsState = TBSTATE_ENABLED;
    //tbb[3].fsStyle = TBSTYLE_SEP;
    //tbb[3].iString = NULL;// (INT_PTR)TEXT("");

    // Create the toolbar control.
    dwStyle = WS_VISIBLE | WS_CHILD | TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_ALTDRAG
        | CCS_ADJUSTABLE | CCS_NODIVIDER | CCS_NOPARENTALIGN;

    if (!(hwndTB = CreateToolbarEx(
        hWnd,               // Parent window handle
        dwStyle,            // Toolbar window styles
        (UINT)ID_TOOLBAR,  // Toolbar control identifier
        NUMIMAGES,          // Number of button images
        hInst,              // Module instance 
        //        (UINT_PTR)IDB_BITMAP1,        // Bitmap resource identifier
        (UINT_PTR)NULL,        // Bitmap resource identifier
        tbb,           // Array of TBBUTTON structure 
        // contains button data
        numButtons,
        // Number of buttons in toolbar
        BUTTONWIDTH,        // Width of the button in pixels
        BUTTONHEIGHT,       // Height of the button in pixels
        IMAGEWIDTH,         // Button image width in pixels
        IMAGEHEIGHT,        // Button image height in pixels
        sizeof(TBBUTTON))))// Size of a TBBUTTON structure
    {
        return NULL;
    }

    //    SendMessage(hwndTB, TB_SETSTYLE, 0, (LPARAM)TBSTYLE_FLAT | CCS_TOP | TBSTYLE_LIST | TBSTYLE_ALTDRAG);
    SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_DRAWDDARROWS);

    SendMessage(hwndTB, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));

    //Add ToolTips to the toolbar.
    if (tips != NULL)
    {
        HWND hTooltips;
        TOOLINFO ti = { 0 };
        ti.cbSize = sizeof(TOOLINFO);
        ti.uFlags = TTF_SUBCLASS;
        ti.hwnd = hwndTB;  //¹¤¾ßÀ¸µÄ¾ä±ú   
        ti.hinst = hInst;
        ti.rect.left = 0;
        ti.rect.top = 0;
        ti.rect.bottom = 100;
        ti.rect.right = 100;
        hTooltips = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            hWnd, NULL, hInst,
            NULL);
        SetWindowPos(hTooltips, HWND_TOPMOST, 0, 0, 100, 100,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SendMessage(hTooltips, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));
        for (int i = 0; i < numTips; i++) {
            if (tips[i].text != NULL) {

                ti.uId = tips[i].id;
                ti.lpszText = tips[i].text;
                SendMessage(hTooltips, TTM_ADDTOOL, 0, (LPARAM)&ti);
            }
        }
        SendMessage(hwndTB, TB_SETTOOLTIPS, (WPARAM)hTooltips, 0);
    }

    return hwndTB;
}

// CreateComboEx - Registers the ComboBoxEx window class and creates
// a ComboBoxEx control in the client area of the main window.
//
// g_hwndMain - A handle to the main window.
// g_hinst    - A handle to the program instance.

HWND CWinTools::CreateComboBox(HWND hWnd)
{
    HWND hwnd;
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_USEREX_CLASSES;

    InitCommonControlsEx(&icex);

    //hwnd = CreateWindowEx(0, WC_COMBOBOXEX, NULL,
    //    WS_BORDER | WS_VISIBLE |
    //    WS_CHILD | CBS_DROPDOWN,
    //    // No size yet--resize after setting image list.
    //    0,      // Vertical position of Combobox
    //    0,      // Horizontal position of Combobox
    //    0,      // Sets the width of Combobox
    //    100,    // Sets the height of Combobox
    //    hWnd,
    //    NULL,
    //    hInst,
    //    NULL);

    hwnd = CreateWindowEx(0,
        TEXT("combobox"),
        NULL,
        WS_VISIBLE |
        WS_CHILD |
        WS_TABSTOP |
        WS_VSCROLL |
        WS_CLIPCHILDREN |
        WS_CLIPSIBLINGS |
        CBS_AUTOHSCROLL |
        CBS_DROPDOWN |
        0,
        0,
        0,
        100,
        200,
        hWnd,
        NULL,
        hInst,
        NULL);

    //add some stuff to the combobox
    {
        int   i;
        char szString[1000];

        SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));

        for (i = 0; i < 25; i++)
        {
            sprintf(szString, TEXT("Item %d"), i + 1);
            SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)szString);
        }
    }

    return (hwnd);
}

void CWinTools::CreateRebarBand(HWND hWndReBar, LPSTR title, UINT ID, UINT cx, UINT iImage, HWND hWndChild)
{
    LPREBARBANDINFO rbBand = new REBARBANDINFO;
    ZeroMemory(rbBand, sizeof(REBARBANDINFO));
    // Initialize structure members that both bands will share.
    rbBand->cbSize = sizeof(REBARBANDINFO);  // Required
    rbBand->fMask = RBBIM_COLORS | RBBIM_TEXT | RBBIM_BACKGROUND |
        RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE |
        RBBIM_SIZE | RBBIM_IMAGE | RBBIM_ID;
    rbBand->fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_GRIPPERALWAYS;
    rbBand->hbmBack = NULL;
    //rbBand->hbmBack = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACKGROUND));
    RECT rc;
    GetWindowRect(hWndChild, &rc);
    rbBand->lpText = title;
    rbBand->hwndChild = hWndChild;
    rbBand->cxMinChild = 0;
    rbBand->cyMinChild = 29;// rc.bottom - rc.top;
    rbBand->cx = cx;
    rbBand->wID = ID;
    rbBand->cch = 2;
    rbBand->iImage = iImage;

    // Add the band that has the combo box.
    SendMessage(hWndReBar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)rbBand);

}

HWND CWinTools::CreateRebar(HWND hWnd)
{
    REBARINFO     rbi = { 0 };
    REBARBANDINFO rbBand = { 0 };

    HWND hwndRB = CreateWindowEx(WS_EX_TOOLWINDOW,
        REBARCLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN | RBS_VARHEIGHT | CCS_NODIVIDER,
        0, 0, 0, 0,
        hWnd,
        NULL,
        hInst,
        NULL);
    if (!hwndRB)
        return NULL;

    // Initialize and send the REBARINFO structure.
            //set up the ReBar
    HIMAGELIST himlRebar = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 1, 0);
    HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
    ImageList_AddIcon(himlRebar, hIcon);
    rbi.cbSize = sizeof(rbi);
    rbi.fMask = RBIM_IMAGELIST;
    rbi.himl = himlRebar;
    if (!SendMessage(hwndRB, RB_SETBARINFO, 0, (LPARAM)&rbi))
        return NULL;
    return (hwndRB);
}

HWND CWinTools::MakeReBar(HWND hWnd)
{
    HWND hWndRebar = CreateRebar(hWnd);
    static TBBUTTON tbb[7] = {
        { MAKELONG(STD_FILENEW, 0), STD_FILENEW, TBSTATE_ENABLED,
        BTNS_AUTOSIZE | TBSTYLE_DROPDOWN, {0}, 0, (INT_PTR)L"New" },
        { MAKELONG(STD_FILEOPEN, 0), STD_FILEOPEN, TBSTATE_ENABLED,
        BTNS_AUTOSIZE | BTNS_WHOLEDROPDOWN, {0}, 0, (INT_PTR)L"Open" },
        { MAKELONG(STD_FILESAVE, 0), STD_FILESAVE, TBSTATE_ENABLED,
        BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"Save" },
        { MAKELONG(0, 0), NULL, 0,
        TBSTYLE_SEP, {0}, 0, (INT_PTR)L"" }, // Separator
        { MAKELONG(STD_COPY, 0), STD_COPY, TBSTATE_ENABLED,
        BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"Copy" },
        { MAKELONG(STD_CUT, 0), STD_CUT, TBSTATE_ENABLED,
        BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"Cut" },
        { MAKELONG(STD_PASTE, 0), STD_PASTE, TBSTATE_ENABLED,
        BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"Paste" }
    };
    static TOOL_TIPS tips[7] = {
        { STD_FILENEW,"New tips" },
        { STD_FILEOPEN,"Open tips" },
        { STD_FILESAVE, "Save tips" },
        { 0, NULL }, // Separator
        { STD_COPY,"Copy tips" },
        { STD_CUT, "Cut tips" },
        { STD_PASTE,"Paste tips" }
    };
    HWND hToolBar = CreateToolbar(hWnd, tbb, 7, tips, 7);

    //// Add images
    //TBADDBITMAP tbAddBmp = { 0 };
    //tbAddBmp.hInst = HINST_COMMCTRL;
    //tbAddBmp.nID = IDB_STD_SMALL_COLOR;
    //SendMessage(hToolBar, TB_ADDBITMAP, 0, (WPARAM)&tbAddBmp);

    CreateRebarBand(hWndRebar, "abc", 1, 500, 0, hToolBar);

    HWND hwndCB = CreateComboBox(hWnd);
    CreateRebarBand(hWndRebar, "combox", 2, 0, 0, hwndCB);
    return hWndRebar;
}

BOOL CWinTools::DoNotify(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define lpnm    ((LPNMHDR)lParam)
    switch (lpnm->code)
    {
    case NM_CUSTOMDRAW:
    {
        LPNMCUSTOMDRAW lplvcd = (LPNMCUSTOMDRAW)lParam;
        switch (lplvcd->dwDrawStage) {
        case CDDS_PREPAINT:
            RECT rt;
            GetClientRect(hWnd, &rt);
            FillRect(lplvcd->hdc, &rt, (HBRUSH)GetStockObject(WHITE_BRUSH));
            return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT:
            //SelectObject(lplvcd->nmcd.hdc, GetFontForItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam));
            //lplvcd->clrText = GetColorForItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam);
            //lplvcd->clrTextBk = GetBkColorForItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam);
            /* At this point, you can change the background colors for the item
            and any subitems and return CDRF_NEWFONT. If the list-view control
            is in report mode, you can simply return CDRF_NOTIFYSUBITEMDRAW
            to customize the item's subitems individually */
            //lplvcd->clrBtnFace = RGB(255, 255, 255);
            return CDRF_NEWFONT;
            // or return CDRF_NOTIFYSUBITEMDRAW;
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
            //SelectObject(lplvcd->nmcd.hdc, GetFontForSubItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam, lplvcd->iSubItem));
            //lplvcd->clrText = GetColorForSubItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam, lplvcd->iSubItem));
            //lplvcd->clrTextBk = GetBkColorForSubItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam, lplvcd->iSubItem));
            /* This notification is received only if you are in report mode and
            returned CDRF_NOTIFYSUBITEMDRAW in the previous step. At
            this point, you can change the background colors for the
            subitem and return CDRF_NEWFONT.*/
            //lplvcd->clrBtnFace = RGB(255, 255, 255);
            return CDRF_NEWFONT;
        }
    }
    break;
    case TBN_DROPDOWN:
    {
#define lpnmTB  ((LPNMTOOLBAR)lParam)
        // Get the coordinates of the button.
        RECT rc = { 0 };
        SendMessage(lpnmTB->hdr.hwndFrom, TB_GETRECT, (WPARAM)lpnmTB->iItem, (LPARAM)&rc);
        // Convert to screen coordinates.
        MapWindowPoints(lpnmTB->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);
        // Get the menu.
        HMENU hMenuLoaded = LoadMenu(hInst, MAKEINTRESOURCE(IDC_MENU_MAIN));
        // Get the submenu for the first menu item.
        HMENU hPopupMenu = GetSubMenu(hMenuLoaded, 0);
        // Set up the pop-up menu.
        // In case the toolbar is too close to the bottom of the screen,
        // set rcExclude equal to the button rectangle and the menu will appear above
        // the button, and not below it.
        TPMPARAMS tpm = { 0 };
        tpm.cbSize = sizeof(TPMPARAMS);
        tpm.rcExclude = rc;
        // Show the menu and wait for input. Using Toolbar Controls Windows common controls demo(CppWindowsCommonControls)
        // If the user selects an item, its WM_COMMAND is sent.
        TrackPopupMenuEx(hPopupMenu,
            TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
            rc.left, rc.bottom, hWnd, &tpm);
        DestroyMenu(hMenuLoaded);
        return FALSE;
    }
    }
    return FALSE;
}

LRESULT CWinTools::RelabelButton(HWND hWndToolbar, UINT id, LPSTR text)
{
    TBBUTTONINFO tbInfo = { 0 };
    tbInfo.cbSize =
        sizeof
        (TBBUTTONINFO);
    tbInfo.dwMask = TBIF_TEXT;
    tbInfo.pszText = text;
    return SendMessage(hWndToolbar, TB_SETBUTTONINFO, (WPARAM)id, (LPARAM)&tbInfo);
}