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

#include "GmicInputWriter.h"
#include "FileUtil.h"
#include "InputLayerIndex.h"
#include <boost/endian.hpp>
#include <string>

namespace
{
    struct Gmic8bfInputImageHeader
    {
        Gmic8bfInputImageHeader(
            int32 imageWidth,
            int32 imageHeight,
            int32 imageNumberOfChannels,
            int32 imageBitsPerChannel,
            bool planarChannelOrder,
            int32 imageTileWidth,
            int32 imageTileHeight)
        {
            // G8II = GMIC 8BF input image
            signature[0] = 'G';
            signature[1] = '8';
            signature[2] = 'I';
            signature[3] = 'I';
            version = 2;
            width = imageWidth;
            height = imageHeight;
            numberOfChannels = imageNumberOfChannels;
            bitsPerChannel = imageBitsPerChannel;
            flags = planarChannelOrder ? 1 : 0;
            tileWidth = imageTileWidth;
            tileHeight = imageTileHeight;
        }

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

    uint16 Normalize16BitRange(uint16 value)
    {
        // The host provides 16-bit data in the range of [0, 32768], convert it to the
        // normal 16-bit range of [0, 65535].
        return value > 32767 ? 65535 : value * 2;
    }

    void ScaleSixteenBitDataToOutputRange(void* data, int32 width, int32 height, int32 rowBytes) noexcept
    {
        DebugOut("%s, width=%d height=%d", __FUNCTION__, width, height);

        uint8* scan0 = static_cast<uint8*>(data);

        for (int32 y = 0; y < height; y++)
        {
            uint16* row = reinterpret_cast<uint16*>(scan0 + (static_cast<int64>(y) * rowBytes));

            for (int32 x = 0; x < width; x++)
            {
                // We always store 16-bit data in little-endian.
                *row = boost::endian::native_to_little(Normalize16BitRange(*row));
                row++;
            }
        }
    }

    OSErr SaveActiveLayerImpl(FilterRecordPtr filterRecord, const FileHandle* fileHandle)
    {
        const bool hasTransparency = filterRecord->inLayerPlanes != 0 && filterRecord->inTransparencyMask != 0;

        const VPoint imageSize = GetImageSize(filterRecord);

        int32 width = imageSize.h;
        int32 height = imageSize.v;

        int32 bitDepth;

        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeRGBColor:
            bitDepth = 8;
            break;
        case plugInModeGray16:
        case plugInModeRGB48:
            bitDepth = 16;
            break;
        default:
            return filterBadMode;
        }

        int32 numberOfChannels;

        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeGray16:
            numberOfChannels = hasTransparency ? 2 : 1;
            break;
        case plugInModeRGBColor:
        case plugInModeRGB48:
            numberOfChannels = hasTransparency ? 4 : 3;
            break;
        default:
            return filterBadMode;
        }

        const int32 tileWidth = std::min(GetTileWidth(filterRecord->inTileWidth), width);
        const int32 tileHeight = std::min(GetTileHeight(filterRecord->inTileHeight), height);

        Gmic8bfInputImageHeader fileHeader(width, height, numberOfChannels, bitDepth, /* planar */ true, tileWidth, tileHeight);

        OSErr err = WriteFile(fileHandle, &fileHeader, sizeof(fileHeader));

        if (err == noErr)
        {
            switch (filterRecord->imageMode)
            {
            case plugInModeGrayScale:
            case plugInModeRGBColor:
                filterRecord->inPlaneBytes = 1;
                break;
            case plugInModeGray16:
            case plugInModeRGB48:
                filterRecord->inPlaneBytes = 2;
                break;
            default:
                return filterBadMode;
            }

            filterRecord->inColumnBytes = filterRecord->inPlaneBytes;
            filterRecord->inputRate = int2fixed(1);

            for (int32 i = 0; i < numberOfChannels; i++)
            {
                filterRecord->inLoPlane = filterRecord->inHiPlane = static_cast<int16>(i);

                for (int32 y = 0; y < height; y += tileHeight)
                {
                    const int32 top = y;
                    const int32 bottom = std::min(y + tileHeight, imageSize.v);

                    const int32 rowCount = bottom - top;

                    for (int32 x = 0; x < width; x += tileWidth)
                    {
                        const int32 left = x;
                        const int32 right = std::min(x + tileWidth, imageSize.h);

                        const int32 columnCount = right - left;

                        SetInputRect(filterRecord, top, left, bottom, right);

                        err = filterRecord->advanceState();
                        if (err != noErr)
                        {
                            goto error;
                        }

                        int32 outputStride = columnCount;

                        if (bitDepth == 16)
                        {
                            outputStride *= 2;
                            ScaleSixteenBitDataToOutputRange(filterRecord->inData, columnCount, rowCount, filterRecord->inRowBytes);
                        }

                        if (outputStride == filterRecord->inRowBytes)
                        {
                            // If the host's buffer stride matches the output image stride
                            // we can write the buffer directly.

                            err = WriteFile(fileHandle, filterRecord->inData, static_cast<size_t>(rowCount) * outputStride);

                            if (err != noErr)
                            {
                                goto error;
                            }
                        }
                        else
                        {
                            for (int32 j = 0; j < rowCount; j++)
                            {
                                const uint8* row = static_cast<const uint8*>(filterRecord->inData) + (static_cast<int64>(j) * filterRecord->inRowBytes);

                                err = WriteFile(fileHandle, row, outputStride);

                                if (err != noErr)
                                {
                                    goto error;
                                }
                            }
                        }
                    }
                }
            }
        }
        error:

        // Do not set the FilterRecord data pointers to NULL, some hosts
        // (e.g. XnView) will crash if they are set to NULL by a plug-in.
        SetInputRect(filterRecord, 0, 0, 0, 0);

        return err;
    }

    OSErr SaveDocumentLayer(
        FilterRecordPtr filterRecord,
        const ReadLayerDesc* layerDescriptor,
        const FileHandle* fileHandle)
    {
        const bool hasTransparency = layerDescriptor->transparency != nullptr;
        const VPoint imageSize = GetImageSize(filterRecord);

        int32 width = imageSize.h;
        int32 height = imageSize.v;

        int32 bitDepth;

        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeRGBColor:
            bitDepth = 8;
            break;
        case plugInModeGray16:
        case plugInModeRGB48:
            bitDepth = 16;
            break;
        default:
            return filterBadMode;
        }

        int32 numberOfChannels;

        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeGray16:
            numberOfChannels = hasTransparency ? 2 : 1;
            break;
        case plugInModeRGBColor:
        case plugInModeRGB48:
            numberOfChannels = hasTransparency ? 4 : 3;
            break;
        default:
            return filterBadMode;
        }

        const int32 tileWidth = std::min(GetTileWidth(filterRecord->inTileWidth), width);
        const int32 tileHeight = std::min(GetTileHeight(filterRecord->inTileHeight), height);

        Gmic8bfInputImageHeader fileHeader(width, height, numberOfChannels, bitDepth, /* planar */ true, tileWidth, tileHeight);

        OSErr err = WriteFile(fileHandle, &fileHeader, sizeof(fileHeader));

        if (err == noErr)
        {
            switch (filterRecord->imageMode)
            {
            case plugInModeGrayScale:
            case plugInModeRGBColor:
                filterRecord->inPlaneBytes = 1;
                break;
            case plugInModeGray16:
            case plugInModeRGB48:
                filterRecord->inPlaneBytes = 2;
                break;
            default:
                return filterBadMode;
            }

            filterRecord->inColumnBytes = filterRecord->inPlaneBytes;
            filterRecord->inputRate = int2fixed(1);
            int32 tileRowBytes;

            if (!TryMultiplyInt32(tileWidth, filterRecord->inColumnBytes, tileRowBytes))
            {
                // The multiplication would have resulted in an integer overflow / underflow.
                return memFullErr;
            }

            BufferID imageDataBufferID;
            int32 imageDataBufferSize;

            if (!TryMultiplyInt32(tileHeight, tileRowBytes, imageDataBufferSize))
            {
                // The multiplication would have resulted in an integer overflow / underflow.
                return memFullErr;
            }

            err = filterRecord->bufferProcs->allocateProc(imageDataBufferSize, &imageDataBufferID);

            if (err == noErr)
            {
                void* imageDataBuffer = filterRecord->bufferProcs->lockProc(imageDataBufferID, false);

                PixelMemoryDesc dest{};
                dest.data = imageDataBuffer;
                dest.depth = filterRecord->depth;

                if (!TryMultiplyInt32(filterRecord->inColumnBytes, dest.depth, dest.colBits))
                {
                    // The multiplication would have resulted in an integer overflow / underflow.
                    err = memFullErr;
                }

                if (err == noErr)
                {
                    ReadChannelDesc* imageChannels[4] = { nullptr, nullptr, nullptr, nullptr };

                    switch (filterRecord->imageMode)
                    {
                    case plugInModeGrayScale:
                    case plugInModeGray16:
                        imageChannels[0] = layerDescriptor->compositeChannelsList;
                        imageChannels[1] = layerDescriptor->transparency;
                        break;
                    case plugInModeRGBColor:
                    case plugInModeRGB48:
                        imageChannels[0] = layerDescriptor->compositeChannelsList;
                        imageChannels[1] = imageChannels[0]->next;
                        imageChannels[2] = imageChannels[1]->next;
                        imageChannels[3] = layerDescriptor->transparency;
                        break;
                    default:
                        err = filterBadMode;
                        break;
                    }

                    if (err == noErr)
                    {
                        for (int32 i = 0; i < numberOfChannels; i++)
                        {
                            dest.bitOffset = i * dest.depth;

                            for (int32 y = 0; y < height; y += tileHeight)
                            {
                                VRect writeRect{};
                                writeRect.top = y;
                                writeRect.bottom = std::min(y + tileHeight, imageSize.v);

                                const int32 rowCount = writeRect.bottom - writeRect.top;

                                for (int32 x = 0; x < width; x += tileWidth)
                                {
                                    writeRect.left = x;
                                    writeRect.right = std::min(x + tileWidth, imageSize.h);

                                    const int32 columnCount = writeRect.right - writeRect.left;
                                    tileRowBytes = columnCount * filterRecord->inColumnBytes;

                                    if (!TryMultiplyInt32(tileRowBytes, dest.depth, dest.rowBits))
                                    {
                                        err = memFullErr;
                                        goto error;
                                    }

                                    PSScaling scaling{};
                                    scaling.sourceRect = scaling.destinationRect = writeRect;

                                    VRect wroteRect;

                                    err = filterRecord->channelPortProcs->readPixelsProc(imageChannels[i]->port, &scaling, &writeRect, &dest, &wroteRect);

                                    if (err != noErr)
                                    {
                                        goto error;
                                    }

                                    if (wroteRect.top != writeRect.top ||
                                        wroteRect.left != writeRect.left ||
                                        wroteRect.bottom != writeRect.bottom ||
                                        wroteRect.right != writeRect.right)
                                    {
                                        err = readErr;
                                        goto error;
                                    }

                                    if (bitDepth == 16)
                                    {
                                        ScaleSixteenBitDataToOutputRange(dest.data, width, rowCount, tileRowBytes);
                                    }

                                    err = WriteFile(fileHandle, imageDataBuffer, static_cast<size_t>(rowCount) * tileRowBytes);

                                    if (err != noErr)
                                    {
                                        goto error;
                                    }
                                }
                            }
                        }
                    }
                }
            error:

                filterRecord->bufferProcs->unlockProc(imageDataBufferID);
                filterRecord->bufferProcs->freeProc(imageDataBufferID);
            }
        }

        SetInputRect(filterRecord, 0, 0, 0, 0);
        filterRecord->inData = nullptr;

        return err;
    }
}

OSErr WritePixelsFromCallback(
    int32 width,
    int32 height,
    int32 numberOfChannels,
    int32 bitsPerChannel,
    bool planar,
    int32 tileWidth,
    int32 tileHeight,
    WritePixelsCallback writeCallback,
    void* writeCallbackUserState,
    const boost::filesystem::path& outputPath)
{
    if (writeCallback == nullptr)
    {
        return nilHandleErr;
    }

    OSErr err = noErr;

    std::unique_ptr<FileHandle> file;

    err = OpenFile(outputPath, FileOpenMode::Write, file);

    if (err == noErr)
    {
        Gmic8bfInputImageHeader fileHeader(width, height, numberOfChannels, bitsPerChannel, planar, tileWidth, tileHeight);

        err = WriteFile(file.get(), &fileHeader, sizeof(fileHeader));

        if (err == noErr)
        {
            err = writeCallback(
                file.get(),
                width,
                height,
                numberOfChannels,
                bitsPerChannel,
                writeCallbackUserState);
        }
    }

    return err;
}

OSErr SaveActiveLayer(
    const boost::filesystem::path& outputDir,
    InputLayerIndex* index,
    FilterRecordPtr filterRecord)
{
    OSErr err = noErr;

    try
    {
        boost::filesystem::path activeLayerPath;

        err = GetTemporaryFileName(outputDir, activeLayerPath, ".g8i");

        if (err == noErr)
        {
            std::unique_ptr<FileHandle> file;

            err = OpenFile(activeLayerPath, FileOpenMode::Write, file);

            if (err == noErr)
            {
                err = SaveActiveLayerImpl(filterRecord, file.get());

                if (err == noErr)
                {
                    const VPoint imageSize = GetImageSize(filterRecord);

                    int32 layerWidth = imageSize.h;
                    int32 layerHeight = imageSize.v;
                    bool layerIsVisible = true;
                    std::string layerName;

                    if (!TryGetLayerNameAsUTF8String(filterRecord, layerName))
                    {
                        layerName = "Layer 0";
                    }

                    err = index->AddFile(activeLayerPath, layerWidth, layerHeight, layerIsVisible, layerName);
                }
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }

    return err;
}

#ifdef PSSDK_HAS_LAYER_SUPPORT
OSErr SaveAllLayers(
    const boost::filesystem::path& outputDir,
    InputLayerIndex* index,
    int32 targetLayerIndex,
    FilterRecordPtr filterRecord)
{
    OSErr err = noErr;

    try
    {
        ReadLayerDesc* layerDescriptor = filterRecord->documentInfo->layersDescriptor;

        int32 activeLayerIndex = 0;
        int32 pixelBasedLayerCount = 0;
        int32 layerIndex = 0;
        char layerNameBuffer[128]{};

        while (layerDescriptor != nullptr && err == noErr)
        {
            // Skip over any vector layers.
            if (layerDescriptor->isPixelBased)
            {
                boost::filesystem::path imagePath;

                err = GetTemporaryFileName(outputDir, imagePath, ".g8i");

                if (err == noErr)
                {
                    std::unique_ptr<FileHandle> file;

                    err = OpenFile(imagePath, FileOpenMode::Write, file);

                    if (err == noErr)
                    {
                        err = SaveDocumentLayer(filterRecord, layerDescriptor, file.get());

                        if (err == noErr)
                        {
                            const VPoint imageSize = GetImageSize(filterRecord);

                            int32 layerWidth = imageSize.h;
                            int32 layerHeight = imageSize.v;
                            bool layerIsVisible = true;
                            std::string utf8Name;

                            if (layerDescriptor->maxVersion >= 2)
                            {
                                layerIsVisible = layerDescriptor->isVisible;

                                if (layerDescriptor->unicodeName != nullptr)
                                {
                                    utf8Name = ConvertLayerNameToUTF8(layerDescriptor->unicodeName);
                                }
                            }

                            if (utf8Name.empty())
                            {
                                int written = std::snprintf(layerNameBuffer, sizeof(layerNameBuffer), "Layer %d", pixelBasedLayerCount);

                                if (written > 0)
                                {
                                    utf8Name.assign(layerNameBuffer, layerNameBuffer + written);
                                }
                                else
                                {
                                    err = writErr;
                                }
                            }

                            if (err == noErr)
                            {
                                err = index->AddFile(imagePath, layerWidth, layerHeight, layerIsVisible, utf8Name);
                            }
                        }
                    }
                }

                if (layerIndex == targetLayerIndex)
                {
                    activeLayerIndex = pixelBasedLayerCount;
                }

                pixelBasedLayerCount++;
            }

            layerDescriptor = layerDescriptor->next;
            layerIndex++;
        }

        index->SetActiveLayerIndex(activeLayerIndex);
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }

    return err;
}

#endif // PSSDK_HAS_LAYER_SUPPORT
