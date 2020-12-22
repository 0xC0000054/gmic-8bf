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

#include "ImageConversion.h"

#ifdef __PIWin__
#include "ImageConversionWin.h"
#else
#error "Missing a ImageConversion header for this platform."
#endif

OSErr ConvertImageToGmicInputFormat(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& input,
    const boost::filesystem::path& output)
{
    return ConvertImageToGmicInputFormatNative(filterRecord, input, output);
}

OSErr ConvertImageToGmicInputFormat(
    const FilterRecordPtr filterRecord,
    const void* input,
    size_t inputLength,
    const boost::filesystem::path& output)
{
    return ConvertImageToGmicInputFormatNative(filterRecord, input, inputLength, output);
}
