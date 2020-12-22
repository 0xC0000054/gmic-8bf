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

#ifndef IMAGECONVERSIONWIN_H
#define IMAGECONVERSIONWIN_H

#include "Common.h"

OSErr ConvertImageToGmicInputFormatNative(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& input,
    const boost::filesystem::path& output);

OSErr ConvertImageToGmicInputFormatNative(
    const FilterRecordPtr filterRecord,
    const void* input,
    size_t inputLength,
    const boost::filesystem::path& output);

#endif // !IMAGECONVERSIONWIN_H
