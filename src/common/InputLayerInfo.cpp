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

#include "InputLayerInfo.h"
#include "StringIO.h"

namespace
{
    struct IndexLayerInfoHeader
    {
        IndexLayerInfoHeader(int32 width, int32 height, bool visible)
        {
            layerWidth = width;
            layerHeight = height;
            layerIsVisible = visible ? 1 : 0;
        }

        int32_t layerWidth;
        int32_t layerHeight;
        int32_t layerIsVisible;
    };
}

InputLayerInfo::InputLayerInfo(boost::filesystem::path path, int32 width, int32 height, bool visible, ::std::string utf8Name)
    : imagePath(path), layerWidth(width), layerHeight(height),
      layerIsVisible(visible), utf8LayerName(utf8Name)
{
}

InputLayerInfo::~InputLayerInfo()
{
}

InputLayerInfo InputLayerInfo::Clone() const
{
    return InputLayerInfo(imagePath, layerWidth, layerHeight, layerIsVisible, utf8LayerName);
}

void InputLayerInfo::Write(FileHandle* fileHandle)
{
    IndexLayerInfoHeader header(layerWidth, layerHeight, layerIsVisible);

    WriteFile(fileHandle, &header, sizeof(header));

    WriteUtf8String(fileHandle, utf8LayerName);
    WriteUtf8String(fileHandle, imagePath.string());
}
