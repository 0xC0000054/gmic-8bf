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

#ifndef GMIC8BFIMAGEREADER_H
#define GMIC8BFIMAGEREADER_H

#include "GmicPlugin.h"

bool ImageSizeMatchesDocument(
    const boost::filesystem::path& path,
    const VPoint& documentSize);

void CopyImageToActiveLayer(
    const boost::filesystem::path& path,
    FilterRecord* filterRecord,
    const int32& hostBitDepth);

#endif // !GMIC8BFIMAGEREADER_H
