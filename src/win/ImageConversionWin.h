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

#ifndef IMAGECONVERSIONWIN_H
#define IMAGECONVERSIONWIN_H

#include "Common.h"
#include "InputLayerInfo.h"
#include <memory>

void ConvertImageToGmicInputFormatNative(
    const boost::filesystem::path& input,
    ::std::unique_ptr<InputLayerInfo>& output);

void ConvertImageToGmicInputFormatNative(
    const void* input,
    size_t inputLength,
    ::std::unique_ptr<InputLayerInfo>& output);

#endif // !IMAGECONVERSIONWIN_H
