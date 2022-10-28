////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#ifndef COMMONUIWIN_H
#define COMMONUIWIN_H

#include <Common.h>

void CenterDialog(HWND hDlg);

OSErr ShowErrorMessageNative(
    const char* message,
    const char* caption,
    const FilterRecordPtr filterRecord,
    OSErr fallbackErrorCode);

#endif // !COMMONUIWIN_H
