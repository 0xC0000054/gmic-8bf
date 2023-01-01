////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022, 2023 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "ImageSaveDialog.h"

#ifdef __PIWin__
#include "ImageSaveDialogWin.h"
#else
#error "Missing an ImageSaveDialog header for this platform."
#endif

OSErr GetNewImageFileName(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& defaultFileName,
    boost::filesystem::path& outputFileName,
    int32 imageBitDepth)
{
    return GetNewImageFileNameNative(filterRecord, defaultFileName, outputFileName, imageBitDepth);
}
