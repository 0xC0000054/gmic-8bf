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

#ifndef GMIC8BFIMAGEHEADER_H
#define GMIC8BFIMAGEHEADER_H

#pragma once

#include "FileUtil.h"
#include <boost/endian.hpp>

struct Gmic8bfImageHeader
{
    Gmic8bfImageHeader(const FileHandle* fileHandle);

    Gmic8bfImageHeader(
        int32 imageWidth,
        int32 imageHeight,
        int32 imageNumberOfChannels,
        int32 imageBitsPerChannel,
        bool planarChannelOrder,
        int32 imageTileWidth,
        int32 imageTileHeight);

    int32 GetWidth() const;

    int32 GetHeight() const;

    int32 GetNumberOfChannels() const;

    int32 GetBitsPerChannel() const;

    int32 GetTileWidth() const;

    int32 GetTileHeight() const;

    bool HasAlphaChannel() const;

    bool IsPlanar() const;

private:

    char signature[4];
    boost::endian::little_int32_t version;
    boost::endian::little_int32_t width;
    boost::endian::little_int32_t height;
    boost::endian::little_int32_t numberOfChannels;
    boost::endian::little_int32_t bitsPerChannel;
    boost::endian::little_int32_t flags;
    boost::endian::little_int32_t tileWidth;
    boost::endian::little_int32_t tileHeight;
};

#endif // !GMIC8BFIMAGEHEADER_H
