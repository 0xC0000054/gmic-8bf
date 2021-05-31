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

#pragma once

#ifndef IMAGECONVERSIONWIN_H
#define IMAGECONVERSIONWIN_H

#include "Common.h"

void ConvertImageToGmicInputFormatNative(
    const boost::filesystem::path& input,
    const boost::filesystem::path& output);

void ConvertImageToGmicInputFormatNative(
    const void* input,
    size_t inputLength,
    const boost::filesystem::path& output);

#endif // !IMAGECONVERSIONWIN_H
