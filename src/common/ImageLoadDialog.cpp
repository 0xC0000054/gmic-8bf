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

#include "ImageLoadDialog.h"

#ifdef __PIWin__
#include "ImageLoadDialogWin.h"
#else
#error "Missing an ImageLoadDialog header for this platform."
#endif

OSErr GetImageFileName(
    intptr_t parentWindowHandle,
    boost::filesystem::path& imageFileName)
{
    return GetImageFileNameNative(parentWindowHandle, imageFileName);
}
