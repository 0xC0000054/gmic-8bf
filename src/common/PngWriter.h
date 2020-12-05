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

#ifndef PNGWRITER_H
#define PNGWRITER_H

#include "GmicPlugin.h"
#include "InputLayerIndex.h"
#include <boost/filesystem.hpp>

OSErr SaveActiveLayer(const boost::filesystem::path& path, FilterRecordPtr filterRecord);

#if PSSDK_HAS_LAYER_SUPPORT
OSErr SaveAllLayers(
    const boost::filesystem::path& outputDir,
    InputLayerIndex* index,
    int32 targetLayerIndex,
    FilterRecordPtr filterRecord);
#endif // PSSDK_HAS_LAYER_SUPPORT

#endif // !PNGWRITER_H
