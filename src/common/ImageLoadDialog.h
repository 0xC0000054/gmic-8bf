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

#ifndef IMAGELOADDIALOG_H
#define IMAGELOADDIALOG_H

#include "Common.h"

OSErr GetImageFileName(
    intptr_t parentWindowHandle,
    boost::filesystem::path& imageFileName);

#endif // !IMAGELOADDIALOG_H
