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

#include "ImageConversion.h"

#ifdef __PIWin__
#include "ImageConversionWin.h"
#else
#error "Missing a ImageConversion header for this platform."
#endif

void ConvertImageToGmicInputFormat(
    const boost::filesystem::path& input,
    ::std::unique_ptr<InputLayerInfo>& output)
{
    ConvertImageToGmicInputFormatNative(input, output);
}

void ConvertImageToGmicInputFormat(
    const void* input,
    size_t inputLength,
    ::std::unique_ptr<InputLayerInfo>& output)
{
    ConvertImageToGmicInputFormatNative(input, inputLength, output);
}
