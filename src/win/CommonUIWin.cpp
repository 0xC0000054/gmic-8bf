////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2025 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "CommonUIWin.h"

// The CenterDialog function was adapted from WinDialogUtils.cpp in the PS6 SDK.
/*******************************************************************************/

/* Centers a dialog template 1/3 of the way down on the main screen */

void CenterDialog(HWND hDlg)
{
    int  nHeight;
    int  nWidth;
    int  nTitleBits;
    RECT rcDialog;
    RECT rcParent;
    int  xOrigin;
    int  yOrigin;
    int  xScreen;
    int  yScreen;
    HWND hParent = GetParent(hDlg);

    if (hParent == NULL)
        hParent = GetDesktopWindow();

    GetClientRect(hParent, &rcParent);
    ClientToScreen(hParent, (LPPOINT)&rcParent.left);  // point(left,  top)
    ClientToScreen(hParent, (LPPOINT)&rcParent.right); // point(right, bottom)

    // Center on Title: title bar has system menu, minimize,  maximize bitmaps
    // Width of title bar bitmaps - assumes 3 of them and dialog has a sysmenu
    nTitleBits = GetSystemMetrics(SM_CXSIZE);

    // If dialog has no sys menu compensate for odd# bitmaps by sub 1 bitwidth
    if (!(GetWindowLong(hDlg, GWL_STYLE) & WS_SYSMENU))
        nTitleBits -= nTitleBits / 3;

    GetWindowRect(hDlg, &rcDialog);
    nWidth = rcDialog.right - rcDialog.left;
    nHeight = rcDialog.bottom - rcDialog.top;

    xOrigin = ::std::max(rcParent.right - rcParent.left - nWidth, 0L) / 2
        + rcParent.left - nTitleBits;
    xScreen = GetSystemMetrics(SM_CXSCREEN);
    if (xOrigin + nWidth > xScreen)
        xOrigin = ::std::max(0, xScreen - nWidth);

    yOrigin = ::std::max(rcParent.bottom - rcParent.top - nHeight, 0L) / 3
        + rcParent.top;
    yScreen = GetSystemMetrics(SM_CYSCREEN);
    if (yOrigin + nHeight > yScreen)
        yOrigin = ::std::max(0, yScreen - nHeight);

    SetWindowPos(hDlg, NULL, xOrigin, yOrigin, nWidth, nHeight, SWP_NOZORDER);
}

OSErr ShowErrorMessageNative(
    const char* message,
    const char* caption,
    const FilterRecordPtr filterRecord,
    OSErr fallbackErrorCode)
{
    PlatformData* platformData = static_cast<PlatformData*>(filterRecord->platformData);

    HWND parent = platformData != nullptr ? reinterpret_cast<HWND>(platformData->hwnd) : nullptr;

    if (MessageBoxA(parent, message, caption, MB_OK | MB_ICONERROR) == IDOK)
    {
        // Any positive number is a plug-in handled error message.
        return 1;
    }
    else
    {
        return fallbackErrorCode;
    }
}
