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

#ifndef IMAGESAVEDIALOG_H
#define IMAGESAVEDIALOG_H

#include "GmicPlugin.h"

OSErr GetNewImageFileName(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& defaultFileName,
    boost::filesystem::path& outputFileName,
    int32 imageBitDepth);

#endif // !IMAGESAVEDIALOG_H
