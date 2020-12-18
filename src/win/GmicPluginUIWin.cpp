////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GmicPlugin.h"
#include "CommonUIWin.h"
#include <stdio.h>
#include "resource.h"
#include "version.h"
#include <CommCtrl.h>
#include <shellapi.h>
#include <objbase.h> // Required by wil/win32_helpers.h
#include <wil/win32_helpers.h>
#include <libpng16/png.h>

namespace
{
    void InitAboutDialog(HWND dp) noexcept
    {
        char s[384]{}, format[384]{};

        GetDlgItemTextA(dp, ABOUTFORMAT, format, 384);
        sprintf_s(s, format, VI_VERSION_STR);
        SetDlgItemTextA(dp, ABOUTFORMAT, s);

        memset(&s, 0, sizeof(s));
        memset(&format, 0, sizeof(format));

        GetDlgItemTextA(dp, IDC_LIBPNG, format, 384);
        png_const_charp str = png_get_copyright(nullptr);
        sprintf_s(s, format, str);
        SetDlgItemTextA(dp, IDC_LIBPNG, s);
    }

    INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam) noexcept
    {
        UNREFERENCED_PARAMETER(lParam);

        int cmd;
        switch (wMsg)
        {
        case WM_INITDIALOG:
            CenterDialog(hDlg);
            InitAboutDialog(hDlg);
            break;
        case WM_LBUTTONUP:
            EndDialog(hDlg, IDOK);
            break;
        case WM_COMMAND:
            cmd = HIWORD(wParam);

            if (cmd == BN_CLICKED)
            {
                EndDialog(hDlg, IDOK);
            }
            break;
        case WM_NOTIFY:
            switch (reinterpret_cast<LPNMHDR>(lParam)->code)
            {
            case NM_CLICK:          // Fall through to the next case.
            case NM_RETURN:
            {
                const PNMLINK pNMLink = reinterpret_cast<const PNMLINK>(lParam);

                if (pNMLink->hdr.idFrom == IDC_GMICQT && (pNMLink->item.iLink == 0))
                {
                    // The item.szUrl field is always empty, so the link has to be hard-coded.
                    ShellExecuteW(nullptr, L"open", L"https://github.com/c-koi/gmic-qt", nullptr, nullptr, SW_SHOW);
                }
                else if (pNMLink->hdr.idFrom == IDC_GMICCORE && (pNMLink->item.iLink == 0))
                {
                    // The item.szUrl field is always empty, so the link has to be hard-coded.
                    ShellExecuteW(nullptr, L"open", L"https://github.com/dtschump/gmic", nullptr, nullptr, SW_SHOW);
                }
                else if (pNMLink->hdr.idFrom == IDC_LIBPNG && (pNMLink->item.iLink == 0))
                {
                    // The item.szUrl field is always empty, so the link has to be hard-coded.
                    ShellExecuteW(nullptr, L"open", L"http://www.libpng.org/pub/png/libpng.html", nullptr, nullptr, SW_SHOW);
                }
                else if (pNMLink->hdr.idFrom == IDC_ZLIB && (pNMLink->item.iLink == 0))
                {
                    // The item.szUrl field is always empty, so the link has to be hard-coded.
                    ShellExecuteW(nullptr, L"open", L"http://zlib.net", nullptr, nullptr, SW_SHOW);
                }
                break;
            }
            }
            break;

        default:
            return FALSE;
        }

        return TRUE;
    }
}

OSErr DoAbout(const AboutRecord* about) noexcept
{
    PlatformData* platform = static_cast<PlatformData*>(about->platformData);

    HWND parent = platform != nullptr ? reinterpret_cast<HWND>(platform->hwnd) : nullptr;

    DialogBoxParam(wil::GetModuleInstanceHandle(), MAKEINTRESOURCE(IDD_ABOUT), parent, AboutDlgProc, 0);

    return noErr;
}
