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

#ifndef GMICINPUTWRITER_H
#define GMICINPUTWRITER_H

#include "GmicPlugin.h"
#include "InputLayerIndex.h"
#include <boost/filesystem.hpp>

// Copies the pixel data from an existing buffer.
// The buffer must use one of the following layouts:
// Grayscale
// Grayscale, Alpha
// Red, Green, Blue
// Red, Green, Blue, Alpha
OSErr CopyFromPixelBuffer(
    int32 width,
    int32 height,
    int32 numberOfChannels,
    int32 bitsPerChannel,
    const void* scan0,
    size_t stride,
    const boost::filesystem::path& outputPath);

OSErr SaveActiveLayer(
    const boost::filesystem::path& outputDir,
    InputLayerIndex* index,
    FilterRecordPtr filterRecord);

#if PSSDK_HAS_LAYER_SUPPORT
OSErr SaveAllLayers(
    const boost::filesystem::path& outputDir,
    InputLayerIndex* index,
    int32 targetLayerIndex,
    FilterRecordPtr filterRecord);
#endif // PSSDK_HAS_LAYER_SUPPORT

#endif // !GMICINPUTWRITER_H
