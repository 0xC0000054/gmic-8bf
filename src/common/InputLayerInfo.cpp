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
#include <boost/endian.hpp>

namespace
{
    OSErr WriteString(const FileHandle* fileHandle, std::string value)
    {
        if (value.size() > static_cast<size_t>(std::numeric_limits<int32>::max()))
        {
            return ioErr;
        }

        boost::endian::little_int32_t stringLength = static_cast<int32>(value.size());

        OSErr err = WriteFile(fileHandle, &stringLength, sizeof(stringLength));

        if (err == noErr)
        {
            err = WriteFile(fileHandle, value.data(), value.size());
        }

        return err;
    }

    struct IndexLayerInfoHeader
    {
        IndexLayerInfoHeader(int32 width, int32 height, bool visible)
        {
            layerWidth = width;
            layerHeight = height;
            layerIsVisible = visible ? 1 : 0;
        }

        boost::endian::little_int32_t layerWidth;
        boost::endian::little_int32_t layerHeight;
        boost::endian::little_int32_t layerIsVisible;
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

OSErr InputLayerInfo::Write(const FileHandle* fileHandle)
{
    IndexLayerInfoHeader header(layerWidth, layerHeight, layerIsVisible);

    OSErr err = WriteFile(fileHandle, &header, sizeof(header));

    if (err == noErr)
    {
        err = WriteString(fileHandle, utf8LayerName);

        if (err == noErr)
        {
            err = WriteString(fileHandle, imagePath.string());
        }
    }

    return err;
}
