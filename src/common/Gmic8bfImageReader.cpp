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

#include "Gmic8bfImageReader.h"
#include "Gmic8bfImageHeader.h"
#include "FileUtil.h"
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

    std::vector<uint16> BuildSixteenBitToHostLUT()
    {
        std::vector<uint16> sixteenBitToHostLUT;
        sixteenBitToHostLUT.reserve(65536);

        for (size_t i = 0; i < sixteenBitToHostLUT.capacity(); i++)
        {
            // According to the Photoshop SDK 16-bit image data is stored in the range of [0, 32768].
            sixteenBitToHostLUT.push_back(static_cast<uint16>(((i * 32768) + 32767) / 65535));
        }

        return sixteenBitToHostLUT;
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
        static const std::vector<uint16> sixteenBitToHostLUT = BuildSixteenBitToHostLUT();

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

    void PremultiplyAlphaEightBitsPerChannel(
        const uint8* const alphaData,
        int32 alphaRowBytes,
        int32 tileWidth,
        int32 tileHeight,
        uint8* outData,
        int32 outRowBytes,
        int32 outColumnStep,
        const uint8* maskData,
        int32 maskRowBytes)
    {
        for (int32 y = 0; y < tileHeight; y++)
        {
            const uint8* alphaPixel = alphaData + (static_cast<int64>(y) * alphaRowBytes);
            uint8* pixel = outData + (static_cast<int64>(y) * outRowBytes);
            const uint8* mask = maskData != nullptr ? maskData + (static_cast<int64>(y) * maskRowBytes) : nullptr;

            for (int32 x = 0; x < tileWidth; x++)
            {
                // Clip the output to the mask, if one is present.
                if (mask == nullptr || *mask != 0)
                {
                    double alpha = static_cast<double>(*alphaPixel) / 255.0;

                    switch (outColumnStep)
                    {
                    case 1:
                        pixel[0] = static_cast<uint8>((static_cast<double>(pixel[0]) * alpha) + 0.5);
                        break;
                    case 3:
                        pixel[0] = static_cast<uint8>((static_cast<double>(pixel[0]) * alpha) + 0.5);
                        pixel[1] = static_cast<uint8>((static_cast<double>(pixel[1]) * alpha) + 0.5);
                        pixel[2] = static_cast<uint8>((static_cast<double>(pixel[2]) * alpha) + 0.5);
                        break;
                    default:
                        throw std::runtime_error("Unsupported image mode.");
                    }
                }

                alphaPixel++;
                pixel += outColumnStep;
                if (mask != nullptr)
                {
                    mask++;
                }
            }
        }
    }

    void PremultiplyAlphaSixteenBitsPerChannel(
        const uint8* const alphaData,
        int32 alphaRowBytes,
        int32 tileWidth,
        int32 tileHeight,
        uint8* outData,
        int32 outRowBytes,
        int32 outColumnStep,
        const uint8* maskData,
        int32 maskRowBytes)
    {
        for (int32 y = 0; y < tileHeight; y++)
        {
            const uint16* alphaPixel = reinterpret_cast<const uint16*>(alphaData + (static_cast<int64>(y) * alphaRowBytes));
            uint16* pixel = reinterpret_cast<uint16*>(outData + (static_cast<int64>(y) * outRowBytes));
            const uint8* mask = maskData != nullptr ? maskData + (static_cast<int64>(y) * maskRowBytes) : nullptr;

            for (int32 x = 0; x < tileWidth; x++)
            {
                // Clip the output to the mask, if one is present.
                if (mask == nullptr || *mask != 0)
                {
                    double alpha = static_cast<double>(*alphaPixel) / 65535.0;

                    switch (outColumnStep)
                    {
                    case 1:
                        pixel[0] = static_cast<uint16>((static_cast<double>(pixel[0]) * alpha) + 0.5);
                        break;
                    case 3:
                        pixel[0] = static_cast<uint16>((static_cast<double>(pixel[0]) * alpha) + 0.5);
                        pixel[1] = static_cast<uint16>((static_cast<double>(pixel[1]) * alpha) + 0.5);
                        pixel[2] = static_cast<uint16>((static_cast<double>(pixel[2]) * alpha) + 0.5);
                        break;
                    default:
                        throw std::runtime_error("Unsupported image mode.");
                    }
                }

                alphaPixel++;
                pixel += outColumnStep;
                if (mask != nullptr)
                {
                    mask++;
                }
            }
        }
    }

    void PremultiplyAlpha(
        const FileHandle* fileHandle,
        uint8* tileBuffer,
        int32 tileWidth,
        int32 tileHeight,
        FilterRecord* filterRecord,
        const VPoint& imageSize)
    {
        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeGray16:
            filterRecord->outLoPlane = filterRecord->outHiPlane = 0;
            break;
        case plugInModeRGBColor:
        case plugInModeRGB48:
            filterRecord->outLoPlane = 0;
            filterRecord->outHiPlane = 2;
            break;
        default:
            throw std::runtime_error("Unsupported image mode.");
        }

        const int32 outColumnStep = (filterRecord->outHiPlane - filterRecord->outLoPlane) + 1;

        if (filterRecord->haveMask)
        {
            filterRecord->maskRate = int2fixed(1);
        }

        for (int32 y = 0; y < imageSize.v; y += tileHeight)
        {
            const int32 top = y;
            const int32 bottom = std::min(y + tileHeight, imageSize.v);

            const int32 rowCount = bottom - top;

            for (int32 x = 0; x < imageSize.h; x += tileWidth)
            {
                const int32 left = x;
                const int32 right = std::min(x + tileWidth, imageSize.h);

                const int32 columnCount = right - left;

                const int32 tileBufferRowBytes = columnCount;
                const size_t tileDataSize = static_cast<size_t>(rowCount) * static_cast<size_t>(tileBufferRowBytes);

                ReadFile(fileHandle, tileBuffer, tileDataSize);

                SetOutputRect(filterRecord, top, left, bottom, right);

                if (filterRecord->haveMask)
                {
                    SetMaskRect(filterRecord, top, left, bottom, right);
                }

                OSErrException::ThrowIfError(filterRecord->advanceState());

                const uint8* maskData = filterRecord->haveMask ? static_cast<const uint8*>(filterRecord->maskData) : nullptr;

                switch (filterRecord->imageMode)
                {
                case plugInModeGrayScale:
                case plugInModeRGBColor:
                    PremultiplyAlphaEightBitsPerChannel(
                        tileBuffer,
                        tileBufferRowBytes,
                        columnCount,
                        rowCount,
                        static_cast<uint8*>(filterRecord->outData),
                        filterRecord->outRowBytes,
                        outColumnStep,
                        maskData,
                        filterRecord->maskRowBytes);
                    break;
                case plugInModeGray16:
                case plugInModeRGB48:
                    PremultiplyAlphaSixteenBitsPerChannel(
                        tileBuffer,
                        tileBufferRowBytes,
                        columnCount,
                        rowCount,
                        static_cast<uint8*>(filterRecord->outData),
                        filterRecord->outRowBytes,
                        outColumnStep,
                        maskData,
                        filterRecord->maskRowBytes);
                    break;
                default:
                    throw std::runtime_error("Unsupported image mode.");
                }
            }
        }
    }

    void SetAlphaChannelToOpaqueEightBitsPerChannel(
        int32 tileWidth,
        int32 tileHeight,
        uint8* outData,
        int32 outDataStride,
        const uint8* maskData,
        int32 maskDataStride)
    {
        for (int32 y = 0; y < tileHeight; y++)
        {
            uint8* pixel = outData + (static_cast<int64>(y) * outDataStride);
            const uint8* mask = maskData != nullptr ? maskData + (static_cast<int64>(y) * maskDataStride) : nullptr;

            for (int32 x = 0; x < tileWidth; x++)
            {
                if (mask == nullptr || *mask != 0)
                {
                    *pixel = 255;
                }

                pixel++;
                if (mask != nullptr)
                {
                    mask++;
                }
            }
        }
    }

    void SetAlphaChannelToOpaqueSixteenBitsPerChannel(
        int32 tileWidth,
        int32 tileHeight,
        uint8* outData,
        int32 outDataStride,
        const uint8* maskData,
        int32 maskDataStride)
    {
        for (int32 y = 0; y < tileHeight; y++)
        {
            uint16* pixel = reinterpret_cast<uint16*>(outData + (static_cast<int64>(y) * outDataStride));
            const uint8* mask = maskData != nullptr ? maskData + (static_cast<int64>(y) * maskDataStride) : nullptr;

            for (int32 x = 0; x < tileWidth; x++)
            {
                if (mask == nullptr || *mask != 0)
                {
                    *pixel = 32768;
                }

                pixel++;
                if (mask != nullptr)
                {
                    mask++;
                }
            }
        }
    }

    void SetAlphaChannelToOpaque(FilterRecord* filterRecord, const VPoint& imageSize)
    {
        int16 alphaChannelPlane;
        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeGray16:
            alphaChannelPlane = 1;
            break;
        case plugInModeRGBColor:
        case plugInModeRGB48:
            alphaChannelPlane = 3;
            break;
        default:
            throw std::runtime_error("Unsupported image mode.");
        }

        filterRecord->outLoPlane = filterRecord->outHiPlane = alphaChannelPlane;

        const int32 tileWidth = std::min(GetTileWidth(filterRecord->outTileWidth), imageSize.h);
        const int32 tileHeight = std::min(GetTileHeight(filterRecord->outTileHeight), imageSize.v);

        if (filterRecord->haveMask)
        {
            filterRecord->maskRate = int2fixed(1);
        }

        for (int32 y = 0; y < imageSize.v; y += tileHeight)
        {
            const int32 top = y;
            const int32 bottom = std::min(y + tileHeight, imageSize.v);

            const int32 rowCount = bottom - top;

            for (int32 x = 0; x < imageSize.h; x += tileWidth)
            {
                const int32 left = x;
                const int32 right = std::min(x + tileWidth, imageSize.h);

                const int32 columnCount = right - left;

                SetOutputRect(filterRecord, top, left, bottom, right);

                if (filterRecord->haveMask)
                {
                    SetMaskRect(filterRecord, top, left, bottom, right);
                }

                OSErrException::ThrowIfError(filterRecord->advanceState());

                const uint8* maskData = filterRecord->haveMask ? static_cast<const uint8*>(filterRecord->maskData) : nullptr;

                switch (filterRecord->imageMode)
                {
                case plugInModeGrayScale:
                case plugInModeRGBColor:
                    SetAlphaChannelToOpaqueEightBitsPerChannel(
                        columnCount,
                        rowCount,
                        static_cast<uint8*>(filterRecord->outData),
                        filterRecord->outRowBytes,
                        maskData,
                        filterRecord->maskRowBytes);
                    break;
                case plugInModeGray16:
                case plugInModeRGB48:
                    SetAlphaChannelToOpaqueSixteenBitsPerChannel(
                        columnCount,
                        rowCount,
                        static_cast<uint8*>(filterRecord->outData),
                        filterRecord->outRowBytes,
                        maskData,
                        filterRecord->maskRowBytes);
                    break;
                default:
                    throw std::runtime_error("Unsupported image mode.");
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

        if (!hasAlphaChannel && canEditLayerTransparency)
        {
            SetAlphaChannelToOpaque(filterRecord, imageSize);
        }

        const bool premultiplyAlpha = hasAlphaChannel && !canEditLayerTransparency;

        const int32 tileWidth = header.GetTileWidth();
        const int32 tileHeight = header.GetTileHeight();

        int32 tileMaxRowBytes = 0;

        if (!TryMultiplyInt32(tileWidth, bitsPerChannel / 8, tileMaxRowBytes))
        {
            // The multiplication would have resulted in an integer overflow / underflow.
            throw std::bad_alloc();
        }

        int32 tileBufferSize = 0;

        if (!TryMultiplyInt32(tileMaxRowBytes, tileHeight, tileBufferSize))
        {
            // The multiplication would have resulted in an integer overflow / underflow.
            throw std::bad_alloc();
        }

        unique_buffer_suite_buffer scopedBuffer(filterRecord, tileBufferSize);

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
                    imageSize);
            }
            else
            {
                filterRecord->outLoPlane = filterRecord->outHiPlane = static_cast<int16>(i);

                for (int32 y = 0; y < imageSize.v; y += tileHeight)
                {
                    const int32 top = y;
                    const int32 bottom = std::min(top + tileHeight, imageSize.v);

                    const int32 rowCount = bottom - top;

                    for (int32 x = 0; x < imageSize.h; x += tileWidth)
                    {
                        const int32 left = x;
                        const int32 right = std::min(left + tileWidth, imageSize.h);

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
                            throw std::runtime_error("Unsupported bit depth.");
                        }

                        const size_t tileDataSize = static_cast<size_t>(rowCount) * static_cast<size_t>(tileBufferRowBytes);

                        ReadFile(fileHandle, tileBuffer, tileDataSize);

                        SetOutputRect(filterRecord, top, left, bottom, right);

                        if (filterRecord->haveMask)
                        {
                            SetMaskRect(filterRecord, top, left, bottom, right);
                        }

                        OSErrException::ThrowIfError(filterRecord->advanceState());

                        const uint8* maskData = filterRecord->haveMask ? static_cast<const uint8*>(filterRecord->maskData) : nullptr;

                        switch (filterRecord->imageMode)
                        {
                        case plugInModeGrayScale:
                        case plugInModeRGBColor:
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
                        case plugInModeGray16:
                        case plugInModeRGB48:
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
                            throw std::runtime_error("Unsupported image mode.");
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
    std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Read);
    Gmic8bfImageHeader header(file.get());

    return header.GetWidth() == documentSize.h && header.GetHeight() == documentSize.v;
}

void CopyImageToActiveLayer(const boost::filesystem::path& path, FilterRecord* filterRecord)
{
    std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Read);

    CopyImageToActiveLayerCore(filterRecord, file.get());
}
