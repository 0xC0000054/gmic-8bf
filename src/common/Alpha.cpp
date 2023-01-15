////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022, 2023 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "Alpha.h"
#include "ImageUtil.h"
#include "Utilities.h"

namespace
{
    void PremultiplyAlphaEightBitsPerChannel(
        const uint8* const alphaData,
        int32 alphaRowBytes,
        int32 tileWidth,
        int32 tileHeight,
        uint8* outData,
        int32 outRowBytes,
        const uint8* maskData,
        int32 maskRowBytes)
    {
        constexpr double maxValue = 255.0;

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
                    double alpha = static_cast<double>(*alphaPixel);

                    double premultipliedColor = (static_cast<double>(pixel[0]) * alpha) / maxValue;

                    pixel[0] = static_cast<uint8>(std::min(std::round(premultipliedColor), maxValue));
                }

                alphaPixel++;
                pixel++;
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
        const uint8* maskData,
        int32 maskRowBytes)
    {
        static const ::std::vector<uint16> sixteenBitToHostLUT = BuildSixteenBitToHostLUT();
        constexpr double maxValue = 32768.0;

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
                    double alpha = static_cast<double>(sixteenBitToHostLUT[*alphaPixel]);

                    double premultipliedColor = (static_cast<double>(pixel[0]) * alpha) / maxValue;

                    pixel[0] = static_cast<uint16>(std::min(std::round(premultipliedColor), maxValue));
                }

                alphaPixel++;
                pixel++;
                if (mask != nullptr)
                {
                    mask++;
                }
            }
        }
    }

    void PremultiplyAlphaThirtyTwoBitsPerChannel(
        const uint8* const alphaData,
        int32 alphaRowBytes,
        int32 tileWidth,
        int32 tileHeight,
        uint8* outData,
        int32 outRowBytes,
        const uint8* maskData,
        int32 maskRowBytes)
    {
        constexpr double maxValue = 1.0;

        for (int32 y = 0; y < tileHeight; y++)
        {
            const float* alphaPixel = reinterpret_cast<const float*>(alphaData + (static_cast<int64>(y) * alphaRowBytes));
            float* pixel = reinterpret_cast<float*>(outData + (static_cast<int64>(y) * outRowBytes));
            const uint8* mask = maskData != nullptr ? maskData + (static_cast<int64>(y) * maskRowBytes) : nullptr;

            for (int32 x = 0; x < tileWidth; x++)
            {
                // Clip the output to the mask, if one is present.
                if (mask == nullptr || *mask != 0)
                {
                    double alpha = static_cast<double>(*alphaPixel);

                    double premultipliedColor = (static_cast<double>(pixel[0]) * alpha) / maxValue;

                    pixel[0] = static_cast<float>(std::min(premultipliedColor, maxValue));
                }

                alphaPixel++;
                pixel++;
                if (mask != nullptr)
                {
                    mask++;
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

    void SetAlphaChannelToOpaqueThirtyTwoBitsPerChannel(
        int32 tileWidth,
        int32 tileHeight,
        uint8* outData,
        int32 outDataStride,
        const uint8* maskData,
        int32 maskDataStride)
    {
        for (int32 y = 0; y < tileHeight; y++)
        {
            float* pixel = reinterpret_cast<float*>(outData + (static_cast<int64>(y) * outDataStride));
            const uint8* mask = maskData != nullptr ? maskData + (static_cast<int64>(y) * maskDataStride) : nullptr;

            for (int32 x = 0; x < tileWidth; x++)
            {
                if (mask == nullptr || *mask != 0)
                {
                    *pixel = 1.0f;
                }

                pixel++;
                if (mask != nullptr)
                {
                    mask++;
                }
            }
        }
    }
}

void PremultiplyAlpha(
    FileHandle* fileHandle,
    uint8* tileBuffer,
    int32 tileWidth,
    int32 tileHeight,
    FilterRecord* filterRecord,
    const VPoint& imageSize,
    int32 bitsPerChannel)
{
    int16 numberOfImagePlanes;
    switch (filterRecord->imageMode)
    {
    case plugInModeGrayScale:
    case plugInModeGray16:
    case plugInModeGray32:
        numberOfImagePlanes = 1;
        break;
    case plugInModeRGBColor:
    case plugInModeRGB48:
    case plugInModeRGB96:
        numberOfImagePlanes = 3;
        break;
    default:
        throw ::std::runtime_error("Unsupported image mode.");
    }

    if (filterRecord->haveMask)
    {
        filterRecord->maskRate = int2fixed(1);
    }

    for (int32 y = 0; y < imageSize.v; y += tileHeight)
    {
        const int32 top = y;
        const int32 bottom = ::std::min(y + tileHeight, imageSize.v);

        const int32 rowCount = bottom - top;

        for (int32 x = 0; x < imageSize.h; x += tileWidth)
        {
            const int32 left = x;
            const int32 right = ::std::min(x + tileWidth, imageSize.h);

            const int32 columnCount = right - left;

            const int32 tileBufferRowBytes = columnCount;
            const size_t tileDataSize = static_cast<size_t>(rowCount) * static_cast<size_t>(tileBufferRowBytes);

            ReadFile(fileHandle, tileBuffer, tileDataSize);

            for (int16 i = 0; i < numberOfImagePlanes; i++)
            {
                filterRecord->outLoPlane = filterRecord->outHiPlane = i;

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
                    PremultiplyAlphaEightBitsPerChannel(
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
                    PremultiplyAlphaSixteenBitsPerChannel(
                        tileBuffer,
                        tileBufferRowBytes,
                        columnCount,
                        rowCount,
                        static_cast<uint8*>(filterRecord->outData),
                        filterRecord->outRowBytes,
                        maskData,
                        filterRecord->maskRowBytes);
                    break;
                case 32:
                    PremultiplyAlphaThirtyTwoBitsPerChannel(
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

void SetAlphaChannelToOpaque(FilterRecord* filterRecord, const VPoint& imageSize, int32 bitsPerChannel)
{
    int16 alphaChannelPlane;
    switch (filterRecord->imageMode)
    {
    case plugInModeGrayScale:
    case plugInModeGray16:
    case plugInModeGray32:
        alphaChannelPlane = 1;
        break;
    case plugInModeRGBColor:
    case plugInModeRGB48:
    case plugInModeRGB96:
        alphaChannelPlane = 3;
        break;
    default:
        throw ::std::runtime_error("Unsupported image mode.");
    }

    filterRecord->outLoPlane = filterRecord->outHiPlane = alphaChannelPlane;

    const int32 tileWidth = ::std::min(GetTileWidth(filterRecord->outTileWidth), imageSize.h);
    const int32 tileHeight = ::std::min(GetTileHeight(filterRecord->outTileHeight), imageSize.v);

    if (filterRecord->haveMask)
    {
        filterRecord->maskRate = int2fixed(1);
    }

    for (int32 y = 0; y < imageSize.v; y += tileHeight)
    {
        const int32 top = y;
        const int32 bottom = ::std::min(y + tileHeight, imageSize.v);

        const int32 rowCount = bottom - top;

        for (int32 x = 0; x < imageSize.h; x += tileWidth)
        {
            const int32 left = x;
            const int32 right = ::std::min(x + tileWidth, imageSize.h);

            const int32 columnCount = right - left;

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
                SetAlphaChannelToOpaqueEightBitsPerChannel(
                    columnCount,
                    rowCount,
                    static_cast<uint8*>(filterRecord->outData),
                    filterRecord->outRowBytes,
                    maskData,
                    filterRecord->maskRowBytes);
                break;
            case 16:
                SetAlphaChannelToOpaqueSixteenBitsPerChannel(
                    columnCount,
                    rowCount,
                    static_cast<uint8*>(filterRecord->outData),
                    filterRecord->outRowBytes,
                    maskData,
                    filterRecord->maskRowBytes);
                break;
            case 32:
                SetAlphaChannelToOpaqueThirtyTwoBitsPerChannel(
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
