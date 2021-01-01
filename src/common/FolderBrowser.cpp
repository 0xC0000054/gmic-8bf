////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "FolderBrowser.h"

#ifdef __PIWin__
#include "FolderBrowserWin.h"
#else
#error "Missing a FolderBrowser header for this platform."
#endif

OSErr GetGmicOutputFolder(FilterRecordPtr filterRecord, boost::filesystem::path& outputFolderPath)
{
    return GetGmicOutputFolderNative(filterRecord, outputFolderPath);
}

OSErr GetDefaultGmicOutputFolder(intptr_t parentWindowHandle, boost::filesystem::path& outputFolderPath)
{
    return GetDefaultGmicOutputFolderNative(parentWindowHandle, outputFolderPath);
}
