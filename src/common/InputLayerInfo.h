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

#ifndef INPUTLAYERINFO_H
#define INPUTLAYERINFO_H

#include "GmicPlugin.h"
#include "FileIO.h"
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
        ::std::string utf8Name);

    ~InputLayerInfo();

    InputLayerInfo Clone() const;

    void Write(FileHandle* fileHandle);

private:
    boost::filesystem::path imagePath;
    int32 layerWidth;
    int32 layerHeight;
    bool layerIsVisible;
    ::std::string utf8LayerName;
};

#endif // !INPUTLAYERINFO_H
