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

#ifndef FOLDERBROWSER_H
#define FOLDERBROWSER_H

#include "GmicPlugin.h"

OSErr GetGmicOutputFolder(FilterRecordPtr filterRecord, boost::filesystem::path& outputFolderPath);

OSErr GetDefaultGmicOutputFolder(intptr_t parentWindowHandle, boost::filesystem::path& outputFolderPath);

#endif // !FOLDERBROWSER_H
