// Devices.cpp: implementation of the CDevices class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <winuser.h>

#include "myDebug.h"

#include "CWinMain.h"
#include "CDevices.h"

using namespace DEVICES;

CDevices clsDevices;

#define APP_MAX_STRING_RC_CHARS     512

TCHAR   gszIntl[]           = TEXT("Intl");
TCHAR   gszIntlList[]       = TEXT("sList");
TCHAR   gszIntlDecimal[]    = TEXT("sDecimal");
TCHAR   gchIntlList         = ',';
TCHAR   gchIntlDecimal      = '.';

TCHAR           gszNull[]           = TEXT("");
TCHAR           gszYes[]            = TEXT("Yes");
TCHAR           gszNo[]             = TEXT("No");

#define APP_MAX_FILE_PATH_CHARS     144
#define APP_MAX_FILE_PATH_BYTES     (APP_MAX_FILE_PATH_CHARS * sizeof(TCHAR))
#define APP_MAX_FILE_TITLE_CHARS    APP_MAX_FILE_PATH_CHARS
#define APP_MAX_FILE_TITLE_BYTES    (APP_MAX_FILE_TITLE_CHARS * sizeof(TCHAR))

typedef struct ACMAPPFILEDESC_TAG
{
    DWORD           fdwState;

    TCHAR           szFileTitle[APP_MAX_FILE_TITLE_CHARS];
    TCHAR           szFilePath[APP_MAX_FILE_PATH_CHARS];
    
    DWORD           cbFileSize;
    UINT            uDosChangeDate;
    UINT            uDosChangeTime;
    DWORD           fdwFileAttributes;

    LPWAVEFORMATEX  pwfx;
    UINT            cbwfx;

    DWORD           dwDataBytes;
    DWORD           dwDataSamples;

} ACMAPPFILEDESC, *PACMAPPFILEDESC;

ACMAPPFILEDESC  gaafd;

PTSTR gaszWaveInOutCapsFormats[32] =
{
    TEXT("8M11"),           // Bit 0    WAVE_FORMAT_1M08
    TEXT("8S11"),           // Bit 1    WAVE_FORMAT_1S08
    TEXT("16M11"),          // Bit 2    WAVE_FORMAT_1M16
    TEXT("16S11"),          // Bit 3    WAVE_FORMAT_1S16
    TEXT("8M22"),           // Bit 4    WAVE_FORMAT_2M08
    TEXT("8S22"),           // Bit 5    WAVE_FORMAT_2S08
    TEXT("16M22"),          // Bit 6    WAVE_FORMAT_2M16
    TEXT("16S22"),          // Bit 7    WAVE_FORMAT_2S16
    TEXT("8M44"),           // Bit 8    WAVE_FORMAT_4M08
    TEXT("8S44"),           // Bit 9    WAVE_FORMAT_4S08
    TEXT("16M44"),          // Bit 10   WAVE_FORMAT_4M16
    TEXT("16S44"),          // Bit 11   WAVE_FORMAT_4S16
    NULL,                   // Bit 12
    NULL,                   // Bit 13
    NULL,                   // Bit 14
    NULL,                   // Bit 15
    NULL,                   // Bit 16
    NULL,                   // Bit 17
    NULL,                   // Bit 18
    NULL,                   // Bit 19
    NULL,                   // Bit 20
    NULL,                   // Bit 21
    NULL,                   // Bit 22
    NULL,                   // Bit 23
    NULL,                   // Bit 24
    NULL,                   // Bit 25
    NULL,                   // Bit 26
    NULL,                   // Bit 27
    NULL,                   // Bit 28
    NULL,                   // Bit 29
    NULL,                   // Bit 30
    NULL                    // Bit 31
};

PTSTR gaszWaveOutCapsSupport[32] =
{
    TEXT("Pitch"),          // Bit 0    WAVECAPS_PITCH
    TEXT("Playback Rate"),  // Bit 1    WAVECAPS_PLAYBACKRATE
    TEXT("Volume"),         // Bit 2    WAVECAPS_VOLUME
    TEXT("L/R Volume"),     // Bit 3    WAVECAPS_LRVOLUME
    TEXT("Sync"),           // Bit 4    WAVECAPS_SYNC
    NULL,                   // Bit 5
    NULL,                   // Bit 6
    NULL,                   // Bit 7
    NULL,                   // Bit 8
    NULL,                   // Bit 9
    NULL,                   // Bit 10
    NULL,                   // Bit 11
    NULL,                   // Bit 12
    NULL,                   // Bit 13
    NULL,                   // Bit 14
    NULL,                   // Bit 15
    NULL,                   // Bit 16
    NULL,                   // Bit 17
    NULL,                   // Bit 18
    NULL,                   // Bit 19
    NULL,                   // Bit 20
    NULL,                   // Bit 21
    NULL,                   // Bit 22
    NULL,                   // Bit 23
    NULL,                   // Bit 24
    NULL,                   // Bit 25
    NULL,                   // Bit 26
    NULL,                   // Bit 27
    NULL,                   // Bit 28
    NULL,                   // Bit 29
    NULL,                   // Bit 30
    NULL                    // Bit 31
};

#define ACMINST_NOT_PRESENT     NULL
#define ACMINST_TRY_LINKING     (HINSTANCE)(UINT)-1

static HINSTANCE    ghinstAcm   = ACMINST_TRY_LINKING;

#ifdef WIN32
TCHAR gszAcmModuleName[]  = TEXT("MSACM32.DLL");
#else
char  gszAcmModuleName[]  = "MSACM.DLL";
#endif

FARPROC    *gpafnAcmFunctions;

PSTR  gapszAcmFunctions[] =
{
    "acmGetVersion",
    "acmMetrics",

    "acmDriverEnum",
#ifdef WIN32
    "acmDriverDetailsW",
    "acmDriverDetailsA",
#else
    "acmDriverDetails",
#endif
#ifdef WIN32
    "acmDriverAddW",
    "acmDriverAddA",
#else
    "acmDriverAdd",
#endif
    "acmDriverRemove",
    "acmDriverOpen",
    "acmDriverClose",
    "acmDriverMessage",
    "acmDriverID",
    "acmDriverPriority",

#ifdef WIN32
    "acmFormatTagDetailsW",
    "acmFormatTagDetailsA",
#else
    "acmFormatTagDetails",
#endif
#ifdef WIN32
    "acmFormatTagEnumW",
    "acmFormatTagEnumA",
#else
    "acmFormatTagEnum",
#endif
#ifdef WIN32
    "acmFormatChooseW",
    "acmFormatChooseA",
#else
    "acmFormatChoose",
#endif
#ifdef WIN32
    "acmFormatDetailsW",
    "acmFormatDetailsA",
#else
    "acmFormatDetails",
#endif
#ifdef WIN32
    "acmFormatEnumW",
    "acmFormatEnumA",
#else
    "acmFormatEnum",
#endif
    "acmFormatSuggest",

#ifdef WIN32
    "acmFilterTagDetailsW",
    "acmFilterTagDetailsA",
#else
    "acmFilterTagDetails",
#endif
#ifdef WIN32
    "acmFilterTagEnumW",
    "acmFilterTagEnumA",
#else
    "acmFilterTagEnum",
#endif
#ifdef WIN32
    "acmFilterChooseW",
    "acmFilterChooseA",
#else
    "acmFilterChoose",
#endif
#ifdef WIN32
    "acmFilterDetailsW",
    "acmFilterDetailsA",
#else
    "acmFilterDetails",
#endif
#ifdef WIN32
    "acmFilterEnumW",
    "acmFilterEnumA",
#else
    "acmFilterEnum",
#endif

    "acmStreamOpen",
    "acmStreamClose",
    "acmStreamSize",
    "acmStreamConvert",
    "acmStreamReset",
    "acmStreamPrepareHeader",
    "acmStreamUnprepareHeader"
};

//
//  For Win32
//
enum ACMTHUNK
{
     ACMTHUNK_GETVERSION = 0,
     ACMTHUNK_METRICS,
     ACMTHUNK_DRIVERENUM,
     ACMTHUNK_DRIVERDETAILSW,
     ACMTHUNK_DRIVERDETAILSA,
     ACMTHUNK_DRIVERADDW,
     ACMTHUNK_DRIVERADDA,
     ACMTHUNK_DRIVERREMOVE,
     ACMTHUNK_DRIVEROPEN,
     ACMTHUNK_DRIVERCLOSE,
     ACMTHUNK_DRIVERMESSAGE,
     ACMTHUNK_DRIVERID,
     ACMTHUNK_DRIVERPRIORITY,
     ACMTHUNK_FORMATTAGDETAILSW,
     ACMTHUNK_FORMATTAGDETAILSA,
     ACMTHUNK_FORMATTAGENUMW,
     ACMTHUNK_FORMATTAGENUMA,
     ACMTHUNK_FORMATCHOOSEW,
     ACMTHUNK_FORMATCHOOSEA,
     ACMTHUNK_FORMATDETAILSW,
     ACMTHUNK_FORMATDETAILSA,
     ACMTHUNK_FORMATENUMW,
     ACMTHUNK_FORMATENUMA,
     ACMTHUNK_FORMATSUGGEST,
     ACMTHUNK_FILTERTAGDETAILSW,
     ACMTHUNK_FILTERTAGDETAILSA,
     ACMTHUNK_FILTERTAGENUMW,
     ACMTHUNK_FILTERTAGENUMA,
     ACMTHUNK_FILTERCHOOSEW,
     ACMTHUNK_FILTERCHOOSEA,
     ACMTHUNK_FILTERDETAILSW,
     ACMTHUNK_FILTERDETAILSA,
     ACMTHUNK_FILTERENUMW,
     ACMTHUNK_FILTERENUMA,
     ACMTHUNK_STREAMOPEN,
     ACMTHUNK_STREAMCLOSE,
     ACMTHUNK_STREAMSIZE,
     ACMTHUNK_STREAMCONVERT,
     ACMTHUNK_STREAMRESET,
     ACMTHUNK_STREAMPREPAREHEADER,
     ACMTHUNK_STREAMUNPREPAREHEADER,

     ACMTHUNK_MAX_FUNCTIONS
};

#define ACMTHUNK_FORMATTAGDETAILS   ACMTHUNK_FORMATTAGDETAILSA
#define ACMTHUNK_FORMATDETAILS	    ACMTHUNK_FORMATDETAILSA
#define ACMTHUNK_FORMATCHOOSE	    ACMTHUNK_FORMATCHOOSEA

#define ACMTHUNK_SIZE_TABLE_BYTES   (ACMTHUNK_MAX_FUNCTIONS * sizeof(FARPROC))


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDevices::CDevices()
{
	guWaveInId  = (UINT)WAVE_MAPPER;
	acmThunkInitialize();
}

CDevices::~CDevices()
{

}

MMRESULT CDevices::FormatChoose(LPWAVEFORMATEX pwfx)
{
    MMRESULT            mmr;
    DWORD               cbwfx;
    ACMFORMATCHOOSE     afc;

	cbwfx = sizeof(WAVEFORMATEX);

    memset(&afc, 0, sizeof(afc));

    afc.cbStruct        = sizeof(afc);
    afc.fdwStyle        = ACMFORMATCHOOSE_STYLEF_SHOWHELP;
    afc.hwndOwner       = clsWinMain.hWnd;
    afc.pwfx            = pwfx;
    afc.cbwfx           = cbwfx;
    afc.pszTitle        = TEXT("Format Choice");

    afc.szFormatTag[0]  = '\0';
    afc.szFormat[0]     = '\0';
    afc.pszName         = NULL;
    afc.cchName         = 0;

    afc.fdwEnum         = 0;
    afc.pwfxEnum        = NULL;

    afc.hInstance       = NULL;
    afc.pszTemplateName = NULL;
    afc.lCustData       = 0L;
    afc.pfnHook         = NULL;

   mmr = acmFormatChoose(&afc);

    if (MMSYSERR_NOERROR != mmr)
    {
        if (ACMERR_CANCELED != mmr)
        {
            MessageBox(clsWinMain.hWnd,
				TEXT("acmFormatChoose() failed with error"), "",
				MB_OK | MB_ICONEXCLAMATION);
        }
    }
	return mmr;
}

MMRESULT ACMAPI CDevices::acmFormatChoose(LPACMFORMATCHOOSE pafmtc)
{
    MMRESULT (ACMAPI *pfnAcmFormatChoose)
    (
        LPACMFORMATCHOOSE       pafmtc
    );

    MMRESULT        mmr;

    if (NULL == gpafnAcmFunctions)
        return (MMSYSERR_ERROR);

    pfnAcmFormatChoose = (MMRESULT (ACMAPI *)(LPACMFORMATCHOOSE pafmtc))gpafnAcmFunctions[ACMTHUNK_FORMATCHOOSE];
    if (NULL == pfnAcmFormatChoose)
        return (MMSYSERR_ERROR);

    mmr = (* pfnAcmFormatChoose)(pafmtc);

    return (mmr);
}

LRESULT CALLBACK CDevices::WaveDeviceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LONG                lDevice;
    BOOL                fInput;
    UINT                uDevId;

    UINT                cWaveDevs;
    UINT                u;
    UINT                uId;
    UINT                uCmd;
    HWND                hcb;
    HWND                hedit;
//    HFONT               hfont;

    WAVEINCAPS          wic;
    WAVEOUTCAPS         woc;

    lDevice = GetWindowLong(hwnd, DWLP_USER);
    uDevId  = (UINT)(int)(short)LOWORD(lDevice);
    fInput  = (BOOL)HIWORD(lDevice);

    //
    //
    //
    switch (uMsg)
    {
        case WM_INITDIALOG:
//          hfont = GetStockFont(ANSI_FIXED_FONT);
//            hfont = ghfontApp;
            hedit = GetDlgItem(hwnd, IDD_WAVEDEVICE_EDIT_CAPABILITIES);
//            SetWindowFont(hedit, hfont, FALSE);

            uDevId = (UINT)(int)(short)LOWORD(lParam);
            fInput = (BOOL)HIWORD(lParam);
            SetWindowLong(hwnd, DWLP_USER, lParam);


            //
            //
            //
            hcb = GetDlgItem(hwnd, IDD_WAVEDEVICE_COMBO_DEVICE);
            //SetWindowFont(hcb, hfont, FALSE);

            if (fInput)
                cWaveDevs = waveInGetNumDevs() + 1;
            else
                cWaveDevs = waveOutGetNumDevs() + 1;

            for (u = (UINT)WAVE_MAPPER; (0 != cWaveDevs); u++, cWaveDevs--)
            {
                if (fInput)
                {
                    clsDevices.MyWaveInGetDevCaps(u, &wic);
                    ComboBox_AddString(hcb, wic.szPname);
                }
                else
                {
                   // AcmAppWaveOutGetDevCaps(u, &woc);
                    ComboBox_AddString(hcb, woc.szPname);
                }

                if (uDevId == u)
                {
                    hedit = GetDlgItem(hwnd, IDD_WAVEDEVICE_EDIT_CAPABILITIES);

                    if (fInput)
                        clsDevices.DisplayWaveInDevCaps(hedit, uDevId, &wic, gaafd.pwfx);
                    else
                        ;//AcmAppDisplayWaveOutDevCaps(hedit, uDevId, &woc, gaafd.pwfx);
                }
            }

            ComboBox_SetCurSel(hcb, uDevId + 1);
            return (TRUE);


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);

            switch (uId)
            {
                case IDOK:
                    hcb    = GetDlgItem(hwnd, IDD_WAVEDEVICE_COMBO_DEVICE);
                    uDevId = ComboBox_GetCurSel(hcb);
                    if (CB_ERR != uDevId)
                    {
                        EndDialog(hwnd, uDevId - 1);
                        break;
                    }

                    // -- fall through -- //
                    
                case IDCANCEL:
                    EndDialog(hwnd, uDevId);
                    break;
                    

                case IDD_WAVEDEVICE_COMBO_DEVICE:
                    uCmd = GET_WM_COMMAND_CMD(wParam, lParam);
                    hcb  = GET_WM_COMMAND_HWND(wParam, lParam);
                    switch (uCmd)
                    {
                        case CBN_SELCHANGE:
                            uDevId = ComboBox_GetCurSel(hcb);
                            if (CB_ERR == uDevId)
                                break;

                            uDevId--;

                            hedit = GetDlgItem(hwnd, IDD_WAVEDEVICE_EDIT_CAPABILITIES);
                            if (fInput)
                            {
                                clsDevices.MyWaveInGetDevCaps(uDevId, &wic);
                                clsDevices.DisplayWaveInDevCaps(hedit, uDevId, &wic, gaafd.pwfx);
                            }
                            else
                            {
                                //AcmAppWaveOutGetDevCaps(uDevId, &woc);
                                //AcmAppDisplayWaveOutDevCaps(hedit, uDevId, &woc, gaafd.pwfx);
                            }
                            break;
                    }
            }
            return (TRUE);
    }

    return (FALSE);
} // AcmAppWaveDeviceDlgProc()

MMRESULT CDevices::MyWaveInGetDevCaps(UINT uDevId, LPWAVEINCAPS pwic)
{
    MMRESULT            mmr;

    //
    //
    //
    mmr = waveInGetDevCaps(uDevId, pwic, sizeof(*pwic));
    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  because some people shipped drivers without testing.
        //
        pwic->szPname[sizeof(pwic->szPname) - 1] = '\0';
    }
    else
    {
        _fmemset(pwic, 0, sizeof(*pwic));

        if (MMSYSERR_BADDEVICEID == mmr)
        {
            return (mmr);
        }

        if (WAVE_MAPPER == uDevId)
        {
            lstrcpy(pwic->szPname, TEXT("Default Wave Input Mapper"));
        }
        else
        {
            wsprintf(pwic->szPname, TEXT("Bad Wave Input Device %u"), uDevId);
        }
    }

    return (MMSYSERR_NOERROR);
} // AcmAppWaveInGetDevCaps()


BOOL CDevices::DisplayWaveInDevCaps(HWND hedit, UINT uDevId, LPWAVEINCAPS pwic, LPWAVEFORMATEX pwfx)
{
    static TCHAR        szDisplayTitle[]    = TEXT("[Wave Input Device Capabilities]\r\n");
    TCHAR               ach[40];
    PTSTR               psz;
    UINT                u;
    UINT                v;
    DWORD               dw;

    SetWindowRedraw(hedit, FALSE);

    MEditPrintF(hedit, NULL);
    MEditPrintF(hedit, szDisplayTitle);

    //
    //
    //
    if (NULL != pwfx)
    {
        TCHAR           szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
        TCHAR           szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
        MMRESULT        mmr;
        HWAVEIN         hwi;

        //
        //
        //
        GetFormatDescription(pwfx, szFormatTag, szFormat);
        MEditPrintF(hedit, TEXT("%17s: %s"), (LPTSTR)TEXT("Format"), (LPTSTR)szFormatTag);
        MEditPrintF(hedit, TEXT("%17s: %s"), (LPTSTR)TEXT("Attributes"), (LPTSTR)szFormat);


        //
        //
        //
        MEditPrintF(hedit, TEXT("~%17s: "), (LPTSTR)TEXT("Recordable"));
        mmr = waveInOpen(&hwi, uDevId,
#if (WINVER < 0x0400)
                         (LPWAVEFORMAT)pwfx,
#else
                         pwfx,
#endif
                         0L, 0L, 0L);

        if (MMSYSERR_NOERROR == mmr)
        {
            MEditPrintF(hedit, gszYes);
            waveInClose(hwi);
            hwi = NULL;
        }
        else
        {
            GetErrorString(mmr, ach);
            MEditPrintF(hedit, TEXT("%s, %s (%u)"), (LPTSTR)gszNo, (LPSTR)ach, mmr);
        }


        //
        //
        //
        MEditPrintF(hedit, TEXT("~%17s: "), (LPTSTR)TEXT("(Query)"));
        mmr = waveInOpen(NULL, uDevId,
#if (WINVER < 0x0400)
                         (LPWAVEFORMAT)pwfx,
#else
                         pwfx,
#endif
                         0L, 0L, WAVE_FORMAT_QUERY);

        if (MMSYSERR_NOERROR == mmr)
        {
            MEditPrintF(hedit, gszYes);
        }
        else
        {
            GetErrorString(mmr, ach);
            MEditPrintF(hedit, TEXT("%s, %s (%u)"), (LPTSTR)gszNo, (LPSTR)ach, mmr);
        }


        MEditPrintF(hedit, gszNull);
    }

    //
    //
    //
    MEditPrintF(hedit, TEXT("%17s: %d"), (LPTSTR)TEXT("Device Id"), uDevId);

    MEditPrintF(hedit, TEXT("%17s: %u"), (LPTSTR)TEXT("Manufacturer Id"), pwic->wMid);
    MEditPrintF(hedit, TEXT("%17s: %u"), (LPTSTR)TEXT("Product Id"), pwic->wPid);
    MEditPrintF(hedit, TEXT("%17s: %u.%.02u"), (LPTSTR)TEXT("Driver Version"),
                (pwic->vDriverVersion >> 8),
                (pwic->vDriverVersion & 0x00FF));
    MEditPrintF(hedit, TEXT("%17s: '%s'"), (LPTSTR)TEXT("Device Name"), (LPTSTR)pwic->szPname);
    MEditPrintF(hedit, TEXT("%17s: %u"), (LPTSTR)TEXT("Channels"), pwic->wChannels);


    //
    //
    //
    //
    MEditPrintF(hedit, TEXT("%17s: %.08lXh"), (LPTSTR)TEXT("Standard Formats"), pwic->dwFormats);
    for (v = u = 0, dw = pwic->dwFormats; (0L != dw); u++)
    {
        if ((BYTE)dw & (BYTE)1)
        {
            psz = gaszWaveInOutCapsFormats[u];
            if (NULL == psz)
            {
                wsprintf(ach, TEXT("Unknown%u"), u);
                psz = ach;
            }

            if (0 == (v % 4))
            {
                if (v != 0)
                {
                    MEditPrintF(hedit, gszNull);
                }
                MEditPrintF(hedit, TEXT("~%19s%s"), (LPTSTR)gszNull, (LPTSTR)psz);
            }
            else
            {
                MEditPrintF(hedit, TEXT("~, %s"), (LPTSTR)psz);
            }

            v++;
        }

        dw >>= 1;
    }
    MEditPrintF(hedit, gszNull);

    SetWindowRedraw(hedit, TRUE);

    return (TRUE);
} // AcmAppDisplayWaveInDevCaps()

int CDevices::MEditPrintF(HWND hedit, PTSTR pszFormat, ...)
{
    static  TCHAR   szCRLF[]              = TEXT("\r\n");

    va_list     va;
    TCHAR       ach[APP_MAX_STRING_RC_CHARS];
    int         n;
    BOOL        fCRLF;
    BOOL        fDebugLog;

    //
    //  default the escapes
    //
    fCRLF     = TRUE;
    fDebugLog = TRUE;


    //
    //  if the pszFormat argument is NULL, then just clear all text in
    //  the edit control..
    //
    if (NULL == pszFormat)
    {
        SetWindowText(hedit, gszNull);

//        AcmAppDebugLog(NULL);

        return (0);
    }

    //
    //  format and display the string in the window... first search for
    //  escapes to modify default behaviour.
    //
    for (;;)
    {
        switch (*pszFormat)
        {
            case '~':
                fCRLF = FALSE;
                pszFormat++;
                continue;

            case '`':
                fDebugLog = FALSE;
                pszFormat++;
                continue;
        }

        break;
    }

    va_start(va, pszFormat);
#ifdef WIN32
    n = wvsprintf(ach, pszFormat, va);
#else
    n = wvsprintf(ach, pszFormat, (LPSTR)va);
#endif
    va_end(va);

    Edit_SetSel(hedit, (WPARAM)-1, (LPARAM)-1);
    Edit_ReplaceSel(hedit, ach);

    if (fDebugLog)
    {
//        AcmAppDebugLog(ach);
    }

    if (fCRLF)
    {
        Edit_SetSel(hedit, (WPARAM)-1, (LPARAM)-1);
        Edit_ReplaceSel(hedit, szCRLF);

        if (fDebugLog)
        {
 //           AcmAppDebugLog(szCRLF);
        }
    }

    return (n);
} // MEditPrintF()

BOOL CDevices::GetFormatDescription(LPWAVEFORMATEX pwfx, LPTSTR pszFormatTag, LPTSTR pszFormat)
{
    MMRESULT            mmr;
    BOOL                f;

    f = TRUE;

    //
    //  get the name for the format tag of the specified format
    //
    if (NULL != pszFormatTag)
    {
        ACMFORMATTAGDETAILS aftd;

        //
        //  initialize all unused members of the ACMFORMATTAGDETAILS
        //  structure to zero
        //
        memset(&aftd, 0, sizeof(aftd));

        //
        //  fill in the required members of the ACMFORMATTAGDETAILS
        //  structure for the ACM_FORMATTAGDETAILSF_FORMATTAG query
        //
        aftd.cbStruct    = sizeof(aftd);
        aftd.dwFormatTag = pwfx->wFormatTag;

        //
        //  ask the ACM to find the first available driver that
        //  supports the specified format tag
        //
        mmr = acmFormatTagDetails(NULL,
                                  &aftd,
                                  ACM_FORMATTAGDETAILSF_FORMATTAG);
        if (MMSYSERR_NOERROR == mmr)
        {
            //
            //  copy the format tag name into the caller's buffer
            //
            lstrcpy(pszFormatTag, aftd.szFormatTag);
        }
        else
        {
            PTSTR           psz;

            //
            //  no ACM driver is available that supports the
            //  specified format tag
            //

            f   = FALSE;
            psz = NULL;

            //
            //  the following stuff if proof that the world does NOT need
            //  yet another ADPCM algorithm!!
            //
            switch (pwfx->wFormatTag)
            {
                case WAVE_FORMAT_UNKNOWN:
                    psz = TEXT("** RESERVED INVALID TAG **");
                    break;

                case WAVE_FORMAT_PCM:
                    psz = TEXT("PCM");
                    break;

                case WAVE_FORMAT_ADPCM:
                    psz = TEXT("Microsoft ADPCM");
                    break;

                case 0x0003:
                    psz = TEXT("MV's *UNREGISTERED* ADPCM");
                    break;

                case WAVE_FORMAT_IBM_CVSD:
                    psz = TEXT("IBM CVSD");
                    break;

                case WAVE_FORMAT_ALAW:
                    psz = TEXT("A-Law");
                    break;

                case WAVE_FORMAT_MULAW:
                    psz = TEXT("u-Law");
                    break;

                case WAVE_FORMAT_OKI_ADPCM:
                    psz = TEXT("OKI ADPCM");
                    break;

                case WAVE_FORMAT_IMA_ADPCM:
                    psz = TEXT("IMA/DVI ADPCM");
                    break;

                case WAVE_FORMAT_DIGISTD:
                    psz = TEXT("DIGI STD");
                    break;

                case WAVE_FORMAT_DIGIFIX:
                    psz = TEXT("DIGI FIX");
                    break;

                case WAVE_FORMAT_YAMAHA_ADPCM:
                    psz = TEXT("Yamaha ADPCM");
                    break;

                case WAVE_FORMAT_SONARC:
                    psz = TEXT("Sonarc");
                    break;

                case WAVE_FORMAT_DSPGROUP_TRUESPEECH:
                    psz = TEXT("DSP Group TrueSpeech");
                    break;

                case WAVE_FORMAT_ECHOSC1:
                    psz = TEXT("Echo SC1");
                    break;

                case WAVE_FORMAT_AUDIOFILE_AF36:
                    psz = TEXT("Audiofile AF36");
                    break;

                case WAVE_FORMAT_CREATIVE_ADPCM:
                    psz = TEXT("Creative Labs ADPCM");
                    break;

                case WAVE_FORMAT_APTX:
                    psz = TEXT("APTX");
                    break;

                case WAVE_FORMAT_AUDIOFILE_AF10:
                    psz = TEXT("Audiofile AF10");
                    break;

                case WAVE_FORMAT_DOLBY_AC2:
                    psz = TEXT("Dolby AC2");
                    break;

                case WAVE_FORMAT_MEDIASPACE_ADPCM:
                    psz = TEXT("Media Space ADPCM");
                    break;

                case WAVE_FORMAT_SIERRA_ADPCM:
                    psz = TEXT("Sierra ADPCM");
                    break;

                case WAVE_FORMAT_G723_ADPCM:
                    psz = TEXT("CCITT G.723 ADPCM");
                    break;

                case WAVE_FORMAT_GSM610:
                    psz = TEXT("GSM 6.10");
                    break;

                case WAVE_FORMAT_G721_ADPCM:
                    psz = TEXT("CCITT G.721 ADPCM");
                    break;

                case WAVE_FORMAT_DEVELOPMENT:
                    psz = TEXT("** RESERVED DEVELOPMENT ONLY TAG **");
                    break;

                default:
                    wsprintf(pszFormatTag, TEXT("[%u] (unknown)"), pwfx->wFormatTag);
                    break;
            }

            if (NULL != psz)
            {
                lstrcpy(pszFormatTag, psz);
            }
        }
    }

    //
    //  get the description of the attributes for the specified
    //  format
    //
    if (NULL != pszFormat)
    {
        ACMFORMATDETAILS    afd;

        //
        //  initialize all unused members of the ACMFORMATDETAILS
        //  structure to zero
        //
        memset(&afd, 0, sizeof(afd));

        //
        //  fill in the required members of the ACMFORMATDETAILS
        //  structure for the ACM_FORMATDETAILSF_FORMAT query
        //
        afd.cbStruct    = sizeof(afd);
        afd.dwFormatTag = pwfx->wFormatTag;
        afd.pwfx        = pwfx;

        //
        //  the cbwfx member must be initialized to the total size
        //  in bytes needed for the specified format. for a PCM 
        //  format, the cbSize member of the WAVEFORMATEX structure
        //  is not valid.
        //
        if (WAVE_FORMAT_PCM == pwfx->wFormatTag)
        {
            afd.cbwfx   = sizeof(PCMWAVEFORMAT);
        }
        else
        {
            afd.cbwfx   = sizeof(WAVEFORMATEX) + pwfx->cbSize;
        }

        //
        //  ask the ACM to find the first available driver that
        //  supports the specified format
        //
        mmr = acmFormatDetails(NULL, &afd, ACM_FORMATDETAILSF_FORMAT);
        if (MMSYSERR_NOERROR == mmr)
        {
            //
            //  copy the format attributes description into the caller's
            //  buffer
            //
            lstrcpy(pszFormat, afd.szFormat);
        }
        else
        {
            TCHAR           ach[2];
            TCHAR           szChannels[24];
            UINT            cBits;

            //
            //  no ACM driver is available that supports the
            //  specified format
            //

            f = FALSE;

            //
            //
            //
            ach[0] = gchIntlList;
            ach[1] = '\0';

            GetProfileString(gszIntl, gszIntlList, ach, ach, sizeof(ach));
            gchIntlList = ach[0];

            ach[0] = gchIntlDecimal;
            ach[1] = '\0';

            GetProfileString(gszIntl, gszIntlDecimal, ach, ach, sizeof(ach));
            gchIntlDecimal = ach[0];


            //
            //  compute the bit depth--this _should_ be the same as
            //  wBitsPerSample, but isn't always...
            //
            cBits = (UINT)(pwfx->nAvgBytesPerSec * 8 /
                           pwfx->nSamplesPerSec /
                           pwfx->nChannels);

            if ((1 == pwfx->nChannels) || (2 == pwfx->nChannels))
            {
                if (1 == pwfx->nChannels)
                    lstrcpy(szChannels, TEXT("Mono"));
                else
                    lstrcpy(szChannels, TEXT("Stereo"));

                wsprintf(pszFormat, TEXT("%lu%c%.03u kHz%c %u Bit%c %s"),
                            pwfx->nSamplesPerSec / 1000,
                            gchIntlDecimal,
                            (UINT)(pwfx->nSamplesPerSec % 1000),
                            gchIntlList,
                            cBits,
                            gchIntlList,
                            (LPTSTR)szChannels);
            }
            else
            {
                wsprintf(pszFormat, TEXT("%lu%c%.03u kHz%c %u Bit%c %u Channels"),
                            pwfx->nSamplesPerSec / 1000,
                            gchIntlDecimal,
                            (UINT)(pwfx->nSamplesPerSec % 1000),
                            gchIntlList,
                            cBits,
                            gchIntlList,
                            pwfx->nChannels);
            }
        }
    }

    //
    //
    //
    return (f);
} // AcmAppGetFormatDescription()

BOOL CDevices::GetErrorString(MMRESULT mmr, LPTSTR pszError)
{
    PTSTR               psz;

    switch (mmr)
    {
        case MMSYSERR_NOERROR:
            psz = TEXT("MMSYSERR_NOERROR");
            break;

        case MMSYSERR_ERROR:
            psz = TEXT("MMSYSERR_ERROR");
            break;

        case MMSYSERR_BADDEVICEID:
            psz = TEXT("MMSYSERR_BADDEVICEID");
            break;

        case MMSYSERR_NOTENABLED:
            psz = TEXT("MMSYSERR_NOTENABLED");
            break;

        case MMSYSERR_ALLOCATED:
            psz = TEXT("MMSYSERR_ALLOCATED");
            break;

        case MMSYSERR_INVALHANDLE:
            psz = TEXT("MMSYSERR_INVALHANDLE");
            break;

        case MMSYSERR_NODRIVER:
            psz = TEXT("MMSYSERR_NODRIVER");
            break;

        case MMSYSERR_NOMEM:
            psz = TEXT("MMSYSERR_NOMEM");
            break;

        case MMSYSERR_NOTSUPPORTED:
            psz = TEXT("MMSYSERR_NOTSUPPORTED");
            break;

        case MMSYSERR_BADERRNUM:
            psz = TEXT("MMSYSERR_BADERRNUM");
            break;

        case MMSYSERR_INVALFLAG:
            psz = TEXT("MMSYSERR_INVALFLAG");
            break;

        case MMSYSERR_INVALPARAM:
            psz = TEXT("MMSYSERR_INVALPARAM");
            break;


        case WAVERR_BADFORMAT:
            psz = TEXT("WAVERR_BADFORMAT");
            break;

        case WAVERR_STILLPLAYING:
            psz = TEXT("WAVERR_STILLPLAYING");
            break;

        case WAVERR_UNPREPARED:
            psz = TEXT("WAVERR_UNPREPARED");
            break;

        case WAVERR_SYNC:
            psz = TEXT("WAVERR_SYNC");
            break;


        case ACMERR_NOTPOSSIBLE:
            psz = TEXT("ACMERR_NOTPOSSIBLE");
            break;

        case ACMERR_BUSY:
            psz = TEXT("ACMERR_BUSY");
            break;

        case ACMERR_UNPREPARED:
            psz = TEXT("ACMERR_UNPREPARED");
            break;

        case ACMERR_CANCELED:
            psz = TEXT("ACMERR_CANCELED");
            break;


        default:
            lstrcpy(pszError, TEXT("(unknown)"));
            return (FALSE);
    }

    lstrcpy(pszError, psz);
    return (TRUE);
} // AcmAppGetErrorString()

MMRESULT ACMAPI CDevices::acmFormatDetails(HACMDRIVER had, LPACMFORMATDETAILS pafd, DWORD fdwDetails)
{
  	MMRESULT (ACMAPI *pfnAcmFormatDetails)
    (
        HACMDRIVER              had,
        LPACMFORMATDETAILS      pafd,
        DWORD                   fdwDetails
    );

	MMRESULT        mmr;

    if (NULL == gpafnAcmFunctions)
        return (MMSYSERR_ERROR);

    pfnAcmFormatDetails = (MMRESULT(ACMAPI *)(HACMDRIVER, LPACMFORMATDETAILS, DWORD))gpafnAcmFunctions[ACMTHUNK_FORMATDETAILS];
    if (NULL == pfnAcmFormatDetails)
        return (MMSYSERR_ERROR);

    mmr = (* pfnAcmFormatDetails)(had, pafd, fdwDetails);

    return (mmr);
} // acmFormatDetails()

MMRESULT ACMAPI CDevices::acmFormatTagDetails(HACMDRIVER had, LPACMFORMATTAGDETAILS paftd, DWORD fdwDetails)
{
    MMRESULT (ACMAPI *pfnAcmFormatTagDetails)
    (
        HACMDRIVER              had,
        LPACMFORMATTAGDETAILS   paftd,
        DWORD                   fdwDetails
    );

    MMRESULT        mmr;

    if (NULL == gpafnAcmFunctions)
        return (MMSYSERR_ERROR);

    pfnAcmFormatTagDetails = (MMRESULT (ACMAPI *)(HACMDRIVER, LPACMFORMATTAGDETAILS, DWORD))gpafnAcmFunctions[ACMTHUNK_FORMATTAGDETAILS];
    if (NULL == pfnAcmFormatTagDetails)
        return (MMSYSERR_ERROR);

    mmr = (* pfnAcmFormatTagDetails)(had, paftd, fdwDetails);

    return (mmr);
} // acmFormatTagDetails()

BOOL CDevices::acmThunkInitialize(void)
{
    int (ACMAPI *pfnAcmGetVersion)
    (
        void
    );

    UINT            fuErrorMode;
    DWORD           dwVersion;
    UINT            u;

    //
    //  if we have already linked to the API's, then just succeed...
    //
    if (NULL != gpafnAcmFunctions)
    {
        //
        //  someone isn't satisfied with calling this API only once?
        //
        return (TRUE);
    }


    //
    //  if we have already tried to link to the ACM, then fail this
    //  call--it isn't present.
    //
    if (ACMINST_TRY_LINKING != ghinstAcm)
        return (FALSE);


    //
    //  try to get a handle on the ACM--if we cannot do this, then fail
    //
    fuErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    ghinstAcm = LoadLibrary(gszAcmModuleName);
    SetErrorMode(fuErrorMode);
    if (ghinstAcm < (HINSTANCE)HINSTANCE_ERROR)
    {
        ghinstAcm = ACMINST_NOT_PRESENT;
        return (FALSE);
    }

    pfnAcmGetVersion = (int(__cdecl*)(void))GetProcAddress(ghinstAcm, gapszAcmFunctions[ACMTHUNK_GETVERSION]);
    if (NULL == pfnAcmGetVersion)
    {
        FreeLibrary(ghinstAcm);
        ghinstAcm = ACMINST_NOT_PRESENT;

        return (FALSE);
    }


    //
    //  allocate our array of function pointers to the ACM... note that
    //  this is dynamically allocated so if the ACM is _not_ present,
    //  then this code and data takes up very little space.
    //
    gpafnAcmFunctions = (FARPROC *)LocalAlloc(LPTR, ACMTHUNK_SIZE_TABLE_BYTES);
    if (NULL == gpafnAcmFunctions)
    {
        FreeLibrary(ghinstAcm);
        ghinstAcm = ACMINST_NOT_PRESENT;

        return (FALSE);
    }

    gpafnAcmFunctions[ACMTHUNK_GETVERSION] = (FARPROC)pfnAcmGetVersion;

    //
    //  if the version of the ACM is *NOT* V2.00 or greater, then
    //  all other API's are unavailable--so don't waste time trying
    //  to link to them.
    //
    dwVersion = (* pfnAcmGetVersion)();
    if (0x0200 > HIWORD(dwVersion))
    {
        return (TRUE);
    }


    //
    //  yipee! the ACM V2.00 or greater appears to be installed and
    //  happy with us--so link to the rest of the nifty cool API's.
    //
    //  start at index 1 since we already linked to acmGetVersion above
    //
    for (u = 1; u < ACMTHUNK_MAX_FUNCTIONS; u++)
    {
        gpafnAcmFunctions[u] = GetProcAddress(ghinstAcm, gapszAcmFunctions[u]);
    }


    //
    //  finally, return success
    //
    return (TRUE);
} // acmThunkInitialize()
