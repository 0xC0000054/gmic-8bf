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

#pragma once

#ifndef GMIC8BFIMAGEWRITER_H
#define GMIC8BFIMAGEWRITER_H

#include "InputLayerIndex.h"
#include "FileUtil.h"
#include "Utilities.h"
#include <boost/filesystem.hpp>

typedef void(*WritePixelsCallback)(
    FileHandle* file,
    int32 imageWidth,
    int32 imageHeight,
    int32 numberOfChannels,
    int32 bitsPerChannel,
    void* userState);

// Writes the pixel data from an existing source using a write callback.
// The pixel data must be written using one of the following layouts:
// Grayscale
// Grayscale, Alpha
// Red, Green, Blue
// Red, Green, Blue, Alpha
void WritePixelsFromCallback(
    int32 width,
    int32 height,
    int32 numberOfChannels,
    int32 bitsPerChannel,
    bool planar,
    int32 tileWidth,
    int32 tileHeight,
    WritePixelsCallback writeCallback,
    void* writeCallbackUserState,
    const boost::filesystem::path& outputPath);

void SaveActiveLayer(
    const boost::filesystem::path& outputDir,
    InputLayerIndex* index,
    FilterRecordPtr filterRecord);

#if PSSDK_HAS_LAYER_SUPPORT
void SaveAllLayers(
    const boost::filesystem::path& outputDir,
    InputLayerIndex* index,
    int32 targetLayerIndex,
    FilterRecordPtr filterRecord);
#endif // PSSDK_HAS_LAYER_SUPPORT

#endif // !GMIC8BFIMAGEWRITER_H
