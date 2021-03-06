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

#ifndef PNGWRITER_H
#define PNGWRITER_H

#include "GmicPlugin.h"
#include <boost/filesystem.hpp>

void ConvertGmic8bfImageToPng(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& inputFilePath,
    const boost::filesystem::path& outputFilePath);

#endif // !PNGWRITER_H
