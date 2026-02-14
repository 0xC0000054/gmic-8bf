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

#ifndef FOLDERBROWSERWIN_H
#define FOLDERBROWSERWIN_H

#include "GmicPlugin.h"

OSErr GetGmicOutputFolderNative(FilterRecordPtr filterRecord, boost::filesystem::path& outputFolderPath);

OSErr GetDefaultGmicOutputFolderNative(intptr_t parentWindowHandle, boost::filesystem::path& outputFolderPath);

#endif // !FOLDERBROWSERWIN_H
