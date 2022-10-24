////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "Gmic8bfImageReader.h"
#include "Gmic8bfImageHeader.h"
#include "FileIO.h"
#include "Alpha.h"
#include "ImageUtil.h"
#include "ScopedBufferSuite.h"
#include "Utilities.h"
#include <algorithm>
#include <new>

namespace
{
    void CopyTileDataToHostEightBitsPerChannel(
        const uint8* const tileBuffer,
        int32 tileBufferRowBytes,
        int32 tileWidth,
        int32 tileHeight,
        uint8* outData,
        int32 outRowBytes,
        const uint8* maskData,
        int32 maskRowBytes)
    {
        for (int32 y = 0; y < tileHeight; y++)
        {
            const uint8* srcPixel = tileBuffer + (static_cast<int64>(y) * tileBufferRowBytes);
            uint8* dstPixel = outData + (static_cast<int64>(y) * outRowBytes);
            const uint8* mask = maskData != nullptr ? maskData + (static_cast<int64>(y) * maskRowBytes) : nullptr;

            for (int32 x = 0; x < tileWidth; x++)
            {
                // Clip the output to the mask, if one is present.
                if (mask == nullptr || *mask != 0)
                {
                    dstPixel[0] = srcPixel[0];
                }

                srcPixel++;
                dstPixel++;
                if (mask != nullptr)
                {
                    mask++;
                }
            }
        }
    }

    void CopyTileDataToHostSixteenBitsPerChannel(
        const uint8* const tileBuffer,
        int32 tileBufferRowBytes,
        int32 tileWidth,
        int32 tileHeight,
        uint8* outData,
        int32 outRowBytes,
        const uint8* maskData,
        int32 maskRowBytes)
    {
        static const ::std::vector<uint16> sixteenBitToHostLUT = BuildSixteenBitToHostLUT();

        for (int32 y = 0; y < tileHeight; y++)
        {
            const uint16* srcPixel = reinterpret_cast<const uint16*>(tileBuffer + (static_cast<int64>(y) * tileBufferRowBytes));
            uint16* dstPixel = reinterpret_cast<uint16*>(outData + (static_cast<int64>(y) * outRowBytes));
            const uint8* mask = maskData != nullptr ? maskData + (static_cast<int64>(y) * maskRowBytes) : nullptr;

            for (int32 x = 0; x < tileWidth; x++)
            {
                // Clip the output to the mask, if one is present.
                if (mask == nullptr || *mask != 0)
                {
                    dstPixel[0] = sixteenBitToHostLUT[srcPixel[0]];
                }

                srcPixel++;
                dstPixel++;
                if (mask != nullptr)
                {
                    mask++;
                }
            }
        }
    }

    void CopyImageToActiveLayerCore(FilterRecordPtr filterRecord, FileHandle* fileHandle)
    {
        Gmic8bfImageHeader header(fileHandle);

        const int32 width = header.GetWidth();
        const int32 height = header.GetHeight();
        const int32 numberOfChannels = header.GetNumberOfChannels();
        const int32 bitsPerChannel = header.GetBitsPerChannel();
        const bool hasAlphaChannel = header.HasAlphaChannel();
        const bool canEditLayerTransparency = filterRecord->outLayerPlanes != 0 && filterRecord->outTransparencyMask != 0;
        const int32 numberOfOutputPlanes = GetImagePlaneCount(
            filterRecord->imageMode,
            filterRecord->outLayerPlanes,
            hasAlphaChannel ? filterRecord->outTransparencyMask : 0);

        const VPoint imageSize = GetImageSize(filterRecord);

        // This method will only be called with planar images that match the input document size.
        assert(width == imageSize.h);
        assert(height == imageSize.v);
        assert(header.IsPlanar());

        const int32 hostBitDepth = GetImageDepth(filterRecord);

        if (bitsPerChannel != hostBitDepth)
        {
            char formatBuffer[256]{};

            const int result = snprintf(
                formatBuffer,
                sizeof(formatBuffer),
                "The G'MIC image bit depth (%d) does not math the host bit depth (%d).",
                bitsPerChannel,
                hostBitDepth);

            if (result > 0)
            {
                throw std::runtime_error(formatBuffer);
            }
            else
            {
                throw std::runtime_error("The G'MIC image bit depth does not math the host bit depth.");
            }
        }

        if (!hasAlphaChannel && canEditLayerTransparency)
        {
            SetAlphaChannelToOpaque(filterRecord, imageSize, bitsPerChannel);
        }

        const bool premultiplyAlpha = hasAlphaChannel && !canEditLayerTransparency;

        const int32 tileWidth = header.GetTileWidth();
        const int32 tileHeight = header.GetTileHeight();

        int32 tileMaxRowBytes = 0;

        if (!TryMultiplyInt32(tileWidth, bitsPerChannel / 8, tileMaxRowBytes))
        {
            // The multiplication would have resulted in an integer overflow / underflow.
            throw ::std::bad_alloc();
        }

        int32 tileBufferSize = 0;

        if (!TryMultiplyInt32(tileMaxRowBytes, tileHeight, tileBufferSize))
        {
            // The multiplication would have resulted in an integer overflow / underflow.
            throw ::std::bad_alloc();
        }

        ScopedBufferSuiteBuffer scopedBuffer(filterRecord, tileBufferSize);

        uint8* tileBuffer = reinterpret_cast<uint8*>(scopedBuffer.Lock());

        if (filterRecord->haveMask)
        {
            filterRecord->maskRate = int2fixed(1);
        }

        const int32 alphaChannelPlaneIndex = hasAlphaChannel ? numberOfChannels - 1 : -1;

        for (int32 i = 0; i < numberOfChannels; i++)
        {
            if (i == alphaChannelPlaneIndex && premultiplyAlpha)
            {
                PremultiplyAlpha(
                    fileHandle,
                    tileBuffer,
                    tileWidth,
                    tileHeight,
                    filterRecord,
                    imageSize,
                    bitsPerChannel);
            }
            else
            {
                for (int32 y = 0; y < imageSize.v; y += tileHeight)
                {
                    const int32 top = y;
                    const int32 bottom = ::std::min(top + tileHeight, imageSize.v);

                    const int32 rowCount = bottom - top;

                    for (int32 x = 0; x < imageSize.h; x += tileWidth)
                    {
                        const int32 left = x;
                        const int32 right = ::std::min(left + tileWidth, imageSize.h);

                        const int32 columnCount = right - left;

                        int32 tileBufferRowBytes;

                        switch (bitsPerChannel)
                        {
                        case 8:
                            tileBufferRowBytes = columnCount;
                            break;
                        case 16:
                            tileBufferRowBytes = columnCount * 2;
                            break;
                        default:
                            throw ::std::runtime_error("Unsupported bit depth.");
                        }

                        const size_t tileDataSize = static_cast<size_t>(rowCount) * static_cast<size_t>(tileBufferRowBytes);

                        ReadFile(fileHandle, tileBuffer, tileDataSize);

                        if (numberOfChannels <= 2 && numberOfOutputPlanes >= 3)
                        {
                            // Convert a gray or gray + alpha image to RGB or RGB + alpha.

                            if (i == 0)
                            {
                                // Gray plane

                                for (int16 colorPlane = 0; colorPlane < 3; colorPlane++)
                                {
                                    filterRecord->outLoPlane = filterRecord->outHiPlane = colorPlane;

                                    SetOutputRect(filterRecord, top, left, bottom, right);

                                    if (filterRecord->haveMask)
                                    {
                                        SetMaskRect(filterRecord, top, left, bottom, right);
                                    }

                                    OSErrException::ThrowIfError(filterRecord->advanceState());

                                    const uint8* maskData = filterRecord->haveMask ? static_cast<const uint8*>(filterRecord->maskData) : nullptr;

                                    switch (bitsPerChannel)
                                    {
                                    case 8:
                                        CopyTileDataToHostEightBitsPerChannel(
                                            tileBuffer,
                                            tileBufferRowBytes,
                                            columnCount,
                                            rowCount,
                                            static_cast<uint8*>(filterRecord->outData),
                                            filterRecord->outRowBytes,
                                            maskData,
                                            filterRecord->maskRowBytes);
                                        break;
                                    case 16:
                                        CopyTileDataToHostSixteenBitsPerChannel(
                                            tileBuffer,
                                            tileBufferRowBytes,
                                            columnCount,
                                            rowCount,
                                            static_cast<uint8*>(filterRecord->outData),
                                            filterRecord->outRowBytes,
                                            maskData,
                                            filterRecord->maskRowBytes);
                                        break;
                                    default:
                                        throw ::std::runtime_error("Unsupported image depth.");
                                    }
                                }
                            }
                            else
                            {
                                // Alpha plane

                                filterRecord->outLoPlane = filterRecord->outHiPlane = 3;

                                SetOutputRect(filterRecord, top, left, bottom, right);

                                if (filterRecord->haveMask)
                                {
                                    SetMaskRect(filterRecord, top, left, bottom, right);
                                }

                                OSErrException::ThrowIfError(filterRecord->advanceState());

                                const uint8* maskData = filterRecord->haveMask ? static_cast<const uint8*>(filterRecord->maskData) : nullptr;

                                switch (bitsPerChannel)
                                {
                                case 8:
                                    CopyTileDataToHostEightBitsPerChannel(
                                        tileBuffer,
                                        tileBufferRowBytes,
                                        columnCount,
                                        rowCount,
                                        static_cast<uint8*>(filterRecord->outData),
                                        filterRecord->outRowBytes,
                                        maskData,
                                        filterRecord->maskRowBytes);
                                    break;
                                case 16:
                                    CopyTileDataToHostSixteenBitsPerChannel(
                                        tileBuffer,
                                        tileBufferRowBytes,
                                        columnCount,
                                        rowCount,
                                        static_cast<uint8*>(filterRecord->outData),
                                        filterRecord->outRowBytes,
                                        maskData,
                                        filterRecord->maskRowBytes);
                                    break;
                                default:
                                    throw ::std::runtime_error("Unsupported image depth.");
                                }
                            }
                        }
                        else
                        {
                            filterRecord->outLoPlane = filterRecord->outHiPlane = static_cast<int16>(i);

                            SetOutputRect(filterRecord, top, left, bottom, right);

                            if (filterRecord->haveMask)
                            {
                                SetMaskRect(filterRecord, top, left, bottom, right);
                            }

                            OSErrException::ThrowIfError(filterRecord->advanceState());

                            const uint8* maskData = filterRecord->haveMask ? static_cast<const uint8*>(filterRecord->maskData) : nullptr;

                            switch (bitsPerChannel)
                            {
                            case 8:
                                CopyTileDataToHostEightBitsPerChannel(
                                    tileBuffer,
                                    tileBufferRowBytes,
                                    columnCount,
                                    rowCount,
                                    static_cast<uint8*>(filterRecord->outData),
                                    filterRecord->outRowBytes,
                                    maskData,
                                    filterRecord->maskRowBytes);
                                break;
                            case 16:
                                CopyTileDataToHostSixteenBitsPerChannel(
                                    tileBuffer,
                                    tileBufferRowBytes,
                                    columnCount,
                                    rowCount,
                                    static_cast<uint8*>(filterRecord->outData),
                                    filterRecord->outRowBytes,
                                    maskData,
                                    filterRecord->maskRowBytes);
                                break;
                            default:
                                throw ::std::runtime_error("Unsupported image depth.");
                            }
                        }
                    }
                }
            }
        }
    }
}

bool ImageSizeMatchesDocument(
    const boost::filesystem::path& path,
    const VPoint& documentSize)
{
    ::std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Read);
    Gmic8bfImageHeader header(file.get());

    return header.GetWidth() == documentSize.h && header.GetHeight() == documentSize.v;
}

void CopyImageToActiveLayer(const boost::filesystem::path& path, FilterRecord* filterRecord)
{
    ::std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Read);

    CopyImageToActiveLayerCore(filterRecord, file.get());
}
