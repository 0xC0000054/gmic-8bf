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

#include "GmicPlugin.h"
#include "FileUtil.h"
#include <string>
#include <boost/filesystem.hpp>

class InputLayerInfo
{
public:
    InputLayerInfo(
        boost::filesystem::path path,
        int32 width,
        int32 height,
        bool visible,
        std::string utf8Name);

    ~InputLayerInfo();

    OSErr Write(const FileHandle* fileHandle);

private:
    boost::filesystem::path imagePath;
    int32 layerWidth;
    int32 layerHeight;
    bool layerIsVisible;
    std::string utf8LayerName;
};

