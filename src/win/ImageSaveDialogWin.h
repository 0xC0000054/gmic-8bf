////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IMAGESAVEDIALOGWIN_H
#define IMAGESAVEDIALOGWIN_H

#include "GmicPlugin.h"

OSErr GetNewImageFileNameNative(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& defaultFileName,
    boost::filesystem::path& outputFileName);

#endif // !IMAGESAVEDIALOGWIN_H
