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

#ifndef GMIC8BFIMAGEHEADER_H
#define GMIC8BFIMAGEHEADER_H

#include "FileIO.h"

struct alignas(4) Gmic8bfImageHeader
{
    Gmic8bfImageHeader(FileHandle* fileHandle);

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
    char endian[4]; // This field is 4 bytes to maintain structure alignment.
    int32_t version;
    int32_t width;
    int32_t height;
    int32_t numberOfChannels;
    int32_t bitsPerChannel;
    int32_t flags;
    int32_t tileWidth;
    int32_t tileHeight;
};

#endif // !GMIC8BFIMAGEHEADER_H
