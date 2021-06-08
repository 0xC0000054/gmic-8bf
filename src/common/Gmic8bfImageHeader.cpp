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

#include "Gmic8bfImageHeader.h"
#include <boost/predef.h>

#ifdef _MSC_VER
// Suppress C26495: Always initialize a member variable.
// The member variables are initialized when the data is read from the file.
#pragma warning(push)
#pragma warning(disable: 26495)
#endif // _MSC_VER

Gmic8bfImageHeader::Gmic8bfImageHeader(const FileHandle* fileHandle)
{
    ReadFile(fileHandle, this, sizeof(*this));

    if (strncmp(signature, "G8IM", 4) != 0)
    {
        throw std::runtime_error("The Gmic8bfImage has an invalid file signature.");
    }

#if BOOST_ENDIAN_BIG_BYTE
    const char* const platformEndian = "BEDN";
#elif BOOST_ENDIAN_LITTLE_BYTE
    const char* const platformEndian = "LEDN";
#else
#error "Unknown endianness on this platform."
#endif

    if (strncmp(endian, platformEndian, 4) != 0)
    {
        throw std::runtime_error("The Gmic8bfImage endianess does not match the current platform.");
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

Gmic8bfImageHeader::Gmic8bfImageHeader(
    int32 imageWidth,
    int32 imageHeight,
    int32 imageNumberOfChannels,
    int32 imageBitsPerChannel,
    bool planarChannelOrder,
    int32 imageTileWidth,
    int32 imageTileHeight)
{
    // G8IM = GMIC 8BF image
    signature[0] = 'G';
    signature[1] = '8';
    signature[2] = 'I';
    signature[3] = 'M';
#if BOOST_ENDIAN_BIG_BYTE
    endian[0] = 'B';
    endian[1] = 'E';
    endian[2] = 'D';
    endian[3] = 'N';
#elif BOOST_ENDIAN_LITTLE_BYTE
    endian[0] = 'L';
    endian[1] = 'E';
    endian[2] = 'D';
    endian[3] = 'N';
#else
#error "Unknown endianness on this platform."
#endif
    version = 1;
    width = imageWidth;
    height = imageHeight;
    numberOfChannels = imageNumberOfChannels;
    bitsPerChannel = imageBitsPerChannel;
    flags = planarChannelOrder ? 1 : 0;
    tileWidth = imageTileWidth;
    tileHeight = imageTileHeight;
}

int32 Gmic8bfImageHeader::GetWidth() const
{
    return width;
}

int32 Gmic8bfImageHeader::GetHeight() const
{
    return height;
}

int32 Gmic8bfImageHeader::GetNumberOfChannels() const
{
    return numberOfChannels;
}

int32 Gmic8bfImageHeader::GetBitsPerChannel() const
{
    return bitsPerChannel;
}

int32 Gmic8bfImageHeader::GetTileWidth() const
{
    return tileWidth;
}

int32 Gmic8bfImageHeader::GetTileHeight() const
{
    return tileHeight;
}

bool Gmic8bfImageHeader::HasAlphaChannel() const
{
    return numberOfChannels == 2  // Grayscale + Alpha
        || numberOfChannels == 4; // RGB + Alpha
}

bool Gmic8bfImageHeader::IsPlanar() const
{
    return (flags & 1) != 0;
}
