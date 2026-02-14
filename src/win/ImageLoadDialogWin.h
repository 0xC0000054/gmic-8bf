////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2026 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#ifndef IMAGELOADDIALOGWIN_H
#define IMAGELOADDIALOGWIN_H

#include "Common.h"

OSErr GetImageFileNameNative(
    intptr_t parentWindowHandle,
    boost::filesystem::path& imageFileName);

#endif // !IMAGELOADDIALOGWIN_H
