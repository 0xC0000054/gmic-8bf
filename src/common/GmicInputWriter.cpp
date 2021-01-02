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
            int32 imageBitsPerChannel)
        {
            // G8II = GMIC 8BF input image
            signature[0] = 'G';
            signature[1] = '8';
            signature[2] = 'I';
            signature[3] = 'I';
            version = 1;
            width = imageWidth;
            height = imageHeight;
            numberOfChannels = imageNumberOfChannels;
            bitsPerChannel = imageBitsPerChannel;
        }

        char signature[4];
        boost::endian::little_int32_t version;
        boost::endian::little_int32_t width;
        boost::endian::little_int32_t height;
        boost::endian::little_int32_t numberOfChannels;
        boost::endian::little_int32_t bitsPerChannel;
    };

    uint16 Normalize16BitRange(uint16 value)
    {
        // The host provides 16-bit data in the range of [0, 32768], convert it to the
        // normal 16-bit range of [0, 65535].
        return value > 32767 ? 65535 : value * 2;
    }

    void ScaleSixteenBitDataToOutputRange(uint16* data, const size_t dataLength) noexcept
    {
        DebugOut("%s, length=%zu", __FUNCTION__, dataLength);

        for (size_t i = 0; i < dataLength; i++)
        {
            // We always store 16-bit data in little-endian.
            data[i] = boost::endian::native_to_little(Normalize16BitRange(data[i]));
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

        Gmic8bfInputImageHeader fileHeader(width, height, numberOfChannels, bitDepth);

        OSErr err = WriteFile(fileHandle, &fileHeader, sizeof(fileHeader));

        if (err == noErr)
        {
            if (bitDepth == 16)
            {
                filterRecord->inColumnBytes = numberOfChannels * 2;
                filterRecord->inPlaneBytes = 2;
            }
            else
            {
                filterRecord->inColumnBytes = numberOfChannels;
                filterRecord->inPlaneBytes = 1;
            }
            filterRecord->inputRate = int2fixed(1);

            int32 outputStride;
            if (!TryMultiplyInt32(width, filterRecord->inColumnBytes, outputStride))
            {
                // The multiplication would have resulted in an integer overflow / underflow.
                return memFullErr;
            }

            filterRecord->inLoPlane = 0;
            filterRecord->inHiPlane = static_cast<int16>(numberOfChannels - 1);

            const int32 tileHeight = GetTileHeight(filterRecord->inTileHeight);
            const int32 maxChunkHeight = std::min(tileHeight, height);

            if (err == noErr)
            {
                const int32 left = 0;
                const int32 right = imageSize.h;

                for (int32 y = 0; y < height; y += maxChunkHeight)
                {
                    const int32 top = y;
                    const int32 bottom = std::min(y + maxChunkHeight, imageSize.v);

                    SetInputRect(filterRecord, top, left, bottom, right);

                    err = filterRecord->advanceState();
                    if (err != noErr)
                    {
                        break;
                    }

                    const size_t rowCount = static_cast<size_t>(bottom) - top;

                    if (bitDepth == 16)
                    {
                        size_t length = rowCount * width * numberOfChannels;

                        ScaleSixteenBitDataToOutputRange(static_cast<uint16*>(filterRecord->inData), length);
                    }

                    if (outputStride == filterRecord->inRowBytes)
                    {
                        // If the host's buffer stride matches the output image stride
                        // we can write the buffer directly.

                        err = WriteFile(fileHandle, filterRecord->inData, rowCount * outputStride);

                        if (err != noErr)
                        {
                            break;
                        }
                    }
                    else
                    {
                        for (size_t j = 0; j < rowCount; j++)
                        {
                            const uint8* row = static_cast<const uint8*>(filterRecord->inData) + (j * filterRecord->inRowBytes);

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

        Gmic8bfInputImageHeader fileHeader(width, height, numberOfChannels, bitDepth);

        OSErr err = WriteFile(fileHandle, &fileHeader, sizeof(fileHeader));

        if (err == noErr)
        {
            if (bitDepth == 16)
            {
                filterRecord->inColumnBytes = numberOfChannels * 2;
                filterRecord->inPlaneBytes = 2;
            }
            else
            {
                filterRecord->inColumnBytes = numberOfChannels;
                filterRecord->inPlaneBytes = 1;
            }
            filterRecord->inputRate = int2fixed(1);

            if (!TryMultiplyInt32(width, filterRecord->inColumnBytes, filterRecord->inRowBytes))
            {
                // The multiplication would have resulted in an integer overflow / underflow.
                return memFullErr;
            }

            filterRecord->inLoPlane = 0;
            filterRecord->inHiPlane = static_cast<int16>(numberOfChannels - 1);

            const int32 tileHeight = GetTileHeight(filterRecord->inTileHeight);
            const int32 maxChunkHeight = std::min(tileHeight, height);

            BufferID imageDataBufferID;
            int32 imageDataBufferSize;

            if (!TryMultiplyInt32(maxChunkHeight, filterRecord->inRowBytes, imageDataBufferSize))
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
                dest.bitOffset = 0;
                dest.depth = filterRecord->depth;

                if (!TryMultiplyInt32(filterRecord->inColumnBytes, dest.depth, dest.colBits) ||
                    !TryMultiplyInt32(filterRecord->inRowBytes, dest.depth, dest.rowBits))
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
                        for (int32 y = 0; y < height; y += maxChunkHeight)
                        {
                            VRect writeRect{};
                            writeRect.top = y;
                            writeRect.left = 0;
                            writeRect.bottom = std::min(y + maxChunkHeight, imageSize.v);
                            writeRect.right = imageSize.h;

                            PSScaling scaling{};
                            scaling.sourceRect = scaling.destinationRect = writeRect;

                            for (int32 i = 0; i < numberOfChannels; i++)
                            {
                                VRect wroteRect;

                                dest.bitOffset = i * dest.depth;

                                err = filterRecord->channelPortProcs->readPixelsProc(imageChannels[i]->port, &scaling, &writeRect, &dest, &wroteRect);

                                if (err != noErr)
                                {
                                    break;
                                }

                                if (wroteRect.top != writeRect.top ||
                                    wroteRect.left != writeRect.left ||
                                    wroteRect.bottom != writeRect.bottom ||
                                    wroteRect.right != writeRect.right)
                                {
                                    err = readErr;
                                    break;
                                }
                            }

                            if (err != noErr)
                            {
                                break;
                            }

                            const size_t rowCount = static_cast<size_t>(writeRect.bottom) - writeRect.top;

                            if (bitDepth == 16)
                            {
                                size_t length = rowCount * width * numberOfChannels;

                                ScaleSixteenBitDataToOutputRange(static_cast<uint16*>(dest.data), length);
                            }

                            err = WriteFile(fileHandle, imageDataBuffer, rowCount * filterRecord->inRowBytes);

                            if (err != noErr)
                            {
                                break;
                            }
                        }
                    }
                }

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
        Gmic8bfInputImageHeader fileHeader(width, height, numberOfChannels, bitsPerChannel);

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
