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

#include "InputLayerInfo.h"

namespace
{
    void WriteString(const FileHandle* fileHandle, std::string value)
    {
        if (value.size() > static_cast<size_t>(std::numeric_limits<int32>::max()))
        {
            throw std::runtime_error("The string length exceeds 2GB.");
        }

        int32_t stringLength = static_cast<int32>(value.size());

        WriteFile(fileHandle, &stringLength, sizeof(stringLength));

        WriteFile(fileHandle, value.data(), value.size());
    }

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

InputLayerInfo::InputLayerInfo(boost::filesystem::path path, int32 width, int32 height, bool visible, std::string utf8Name)
    : imagePath(path), layerWidth(width), layerHeight(height),
      layerIsVisible(visible), utf8LayerName(utf8Name)
{
}

InputLayerInfo::~InputLayerInfo()
{
}

void InputLayerInfo::Write(const FileHandle* fileHandle)
{
    IndexLayerInfoHeader header(layerWidth, layerHeight, layerIsVisible);

    WriteFile(fileHandle, &header, sizeof(header));

    WriteString(fileHandle, utf8LayerName);
    WriteString(fileHandle, imagePath.string());
}
