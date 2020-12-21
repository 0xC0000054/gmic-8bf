////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "PngWriter.h"
#include "FileUtil.h"
#include "InputLayerIndex.h"
#include <boost/endian.hpp>
#include <string>
#include <setjmp.h>
#include <png.h>

namespace
{
    struct PngWriterState
    {
        FileHandle* fileHandle;

        PngWriterState(FileHandle* handle)
            : fileHandle(handle), writeError(noErr), pngErrorSet(false), errorMessage()
        {
        }

        OSErr GetWriteErrorCode() const noexcept
        {
            return writeError;
        }

        std::string GetErrorMessage() const
        {
            return errorMessage;
        }

        void SetFileWriteError(OSErr err) noexcept
        {
            if (err != noErr)
            {
                writeError = err;
            }
        }

        void SetPngError(png_const_charp errorDescription)
        {
            if (!pngErrorSet)
            {
                pngErrorSet = true;
                writeError = ioErr;

                try
                {
                    errorMessage = errorDescription;
                }
                catch (...)
                {
                }
            }
        }

    private:
        OSErr writeError;
        bool pngErrorSet;
        std::string errorMessage;
    };

    void PngWriteErrorHandler(png_structp png, png_const_charp errorDescription)
    {
        PngWriterState* state = static_cast<PngWriterState*>(png_get_error_ptr(png));

        if (state)
        {
            DebugOut("LibPng error: %s", errorDescription);

            state->SetPngError(errorDescription);
        }
    }

    void WritePngData(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        PngWriterState* state = static_cast<PngWriterState*>(png_get_io_ptr(png_ptr));

        state->SetFileWriteError(WriteFile(state->fileHandle, data, length));
    }

    void FlushPngData(png_structp png_ptr)
    {
        (void)png_ptr;
        // We let the OS handle flushing any buffered data to disk.
    }

    uint16 ConvertToPng16BitRange(uint16 value)
    {
        // The host provides 16-bit data in the range of [0, 32768], convert it to the PNG
        // 16-bit range of [0, 65535].
        return value > 32767 ? 65535 : value * 2;
    }

    void ScaleSixteenBitDataToPNG(uint16* data, const size_t dataLength) noexcept
    {
        DebugOut("%s, length=%zu", __FUNCTION__, dataLength);

        for (size_t i = 0; i < dataLength; i++)
        {
            // PNG always stores 16-bit data in big-endian.
            data[i] = boost::endian::native_to_big(ConvertToPng16BitRange(data[i]));
        }
    }

    OSErr SaveActiveLayerImpl(FilterRecordPtr filterRecord, PngWriterState* writerState)
    {
        OSErr err = noErr;

        png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, static_cast<png_voidp>(writerState), PngWriteErrorHandler, nullptr);

        if (!pngPtr)
        {
            return memFullErr;
        }

        png_infop infoPtr = png_create_info_struct(pngPtr);

        if (!infoPtr)
        {
            png_destroy_write_struct(&pngPtr, &infoPtr);

            return memFullErr;
        }

        png_set_write_fn(pngPtr, static_cast<png_voidp>(writerState), WritePngData, FlushPngData);

        // Disable "C4611: interaction between '_setjmp' and C++ object destruction is non-portable" on MSVC.
        // This method does not create any C++ objects.
#if _MSC_VER
#pragma warning(disable:4611)
#endif
        if (setjmp(png_jmpbuf(pngPtr)))
        {
            png_destroy_write_struct(&pngPtr, &infoPtr);

            return ioErr;
        }

#if _MSC_VER
#pragma warning(default:4611)
#endif

        const bool hasTransparency = filterRecord->inLayerPlanes != 0 && filterRecord->inTransparencyMask != 0;

        const VPoint imageSize = GetImageSize(filterRecord);

        int32 width = imageSize.h;
        int32 height = imageSize.v;

        int bitDepth = 0;

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
            png_destroy_write_struct(&pngPtr, &infoPtr);
            return filterBadMode;
        }

        int colorType = 0;

        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeGray16:
            colorType = hasTransparency ? PNG_COLOR_TYPE_GRAY_ALPHA : PNG_COLOR_TYPE_GRAY;
            break;
        case plugInModeRGBColor:
        case plugInModeRGB48:
            colorType = hasTransparency ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB;
            break;
        default:
            png_destroy_write_struct(&pngPtr, &infoPtr);
            return filterBadMode;
        }

        png_set_IHDR(
            pngPtr,
            infoPtr,
            static_cast<png_uint_32>(width),
            static_cast<png_uint_32>(height),
            bitDepth,
            colorType,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

        png_write_info(pngPtr, infoPtr);

        err = writerState->GetWriteErrorCode();

        if (err == noErr)
        {
            // Do not compress or filter the image data when writing it to the file.
            // This can noticeably speedup launching G'MIC-Qt with large images.
            png_set_compression_level(pngPtr, 0);
            png_set_filter(pngPtr, 0, PNG_FILTER_NONE);

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
                png_destroy_write_struct(&pngPtr, &infoPtr);
                return filterBadMode;
            }

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
                png_destroy_write_struct(&pngPtr, &infoPtr);
                return memFullErr;
            }

            filterRecord->inLoPlane = 0;
            filterRecord->inHiPlane = static_cast<int16>(numberOfChannels - 1);

            const int32 tileHeight = filterRecord->inTileHeight != 0 ? filterRecord->inTileHeight : 256;
            const int32 maxChunkHeight = std::min(tileHeight, height);

            if (err == noErr)
            {
                int32 rowStrideBufferSize;

                if (!TryMultiplyInt32(maxChunkHeight, sizeof(int32), rowStrideBufferSize))
                {
                    // The multiplication would have resulted in an integer overflow / underflow.
                    err = memFullErr;
                }

                if (err == noErr)
                {
                    BufferID rowStrideBuffer;

                    err = filterRecord->bufferProcs->allocateProc(rowStrideBufferSize, &rowStrideBuffer);

                    if (err == noErr)
                    {
                        int32* rowStride = reinterpret_cast<int32*>(filterRecord->bufferProcs->lockProc(rowStrideBuffer, false));

                        for (int32 y = 0; y < maxChunkHeight; y++)
                        {
                            rowStride[y] = static_cast<int32>(y) * filterRecord->inRowBytes;
                        }

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

                            const int32 rowCount = bottom - top;

                            if (bitDepth == 16)
                            {
                                size_t length = static_cast<size_t>(rowCount) * width * numberOfChannels;

                                ScaleSixteenBitDataToPNG(static_cast<uint16*>(filterRecord->inData), length);
                            }

                            // AdvanceState will update the filterRecord->inData pointer, so we have to use
                            // png_write_row instead of png_write_rows.
                            for (int32 i = 0; i < rowCount; i++)
                            {
                                png_bytep row = static_cast<png_bytep>(filterRecord->inData) + rowStride[i];

                                png_write_row(pngPtr, row);
                            }

                            err = writerState->GetWriteErrorCode();

                            if (err != noErr)
                            {
                                break;
                            }
                        }

                        if (err == noErr)
                        {
                            png_write_end(pngPtr, infoPtr);

                            err = writerState->GetWriteErrorCode();
                        }

                        filterRecord->bufferProcs->unlockProc(rowStrideBuffer);
                        filterRecord->bufferProcs->freeProc(rowStrideBuffer);
                    }
                }
            }
        }

        png_destroy_write_struct(&pngPtr, &infoPtr);

        // Do not set the FilterRecord data pointers to NULL, some hosts
        // (e.g. XnView) will crash if they are set to NULL by a plug-in.
        SetInputRect(filterRecord, 0, 0, 0, 0);

        return err;
    }

    OSErr SaveDocumentLayer(
        FilterRecordPtr filterRecord,
        const ReadLayerDesc* layerDescriptor,
        PngWriterState* writerState)
    {
        OSErr err = noErr;

        png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, static_cast<png_voidp>(writerState), PngWriteErrorHandler, nullptr);

        if (!pngPtr)
        {
            return memFullErr;
        }

        png_infop infoPtr = png_create_info_struct(pngPtr);

        if (!infoPtr)
        {
            png_destroy_write_struct(&pngPtr, &infoPtr);

            return memFullErr;
        }

        png_set_write_fn(pngPtr, static_cast<png_voidp>(writerState), WritePngData, FlushPngData);

        // Disable "C4611: interaction between '_setjmp' and C++ object destruction is non-portable" on MSVC.
        // This method does not create any C++ objects.
#if _MSC_VER
#pragma warning(disable:4611)
#endif
        if (setjmp(png_jmpbuf(pngPtr)))
        {
            png_destroy_write_struct(&pngPtr, &infoPtr);

            return ioErr;
        }

#if _MSC_VER
#pragma warning(default:4611)
#endif

        const bool hasTransparency = layerDescriptor->transparency != nullptr;
        const VPoint imageSize = GetImageSize(filterRecord);

        int32 width = imageSize.h;
        int32 height = imageSize.v;

        int bitDepth = 0;

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
            png_destroy_write_struct(&pngPtr, &infoPtr);
            return filterBadMode;
        }

        int colorType = 0;

        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeGray16:
            colorType = hasTransparency ? PNG_COLOR_TYPE_GRAY_ALPHA : PNG_COLOR_TYPE_GRAY;
            break;
        case plugInModeRGBColor:
        case plugInModeRGB48:
            colorType = hasTransparency ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB;
            break;
        default:
            png_destroy_write_struct(&pngPtr, &infoPtr);
            return filterBadMode;
        }

        png_set_IHDR(
            pngPtr,
            infoPtr,
            static_cast<png_uint_32>(width),
            static_cast<png_uint_32>(height),
            bitDepth,
            colorType,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

        png_write_info(pngPtr, infoPtr);

        err = writerState->GetWriteErrorCode();

        if (err == noErr)
        {
            // Do not compress or filter the image data when writing it to the file.
            // This can noticeably speedup launching G'MIC-Qt with large images.
            png_set_compression_level(pngPtr, 0);
            png_set_filter(pngPtr, 0, PNG_FILTER_NONE);

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
                png_destroy_write_struct(&pngPtr, &infoPtr);
                return filterBadMode;
            }

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
                png_destroy_write_struct(&pngPtr, &infoPtr);
                return memFullErr;
            }

            filterRecord->inLoPlane = 0;
            filterRecord->inHiPlane = static_cast<int16>(numberOfChannels - 1);

            const int32 tileHeight = filterRecord->inTileHeight != 0 ? filterRecord->inTileHeight : 256;
            const int32 maxChunkHeight = std::min(tileHeight, height);

            BufferID imageDataBufferID;
            int32 imageDataBufferSize;

            if (!TryMultiplyInt32(maxChunkHeight, filterRecord->inRowBytes, imageDataBufferSize))
            {
                // The multiplication would have resulted in an integer overflow / underflow.
                png_destroy_write_struct(&pngPtr, &infoPtr);
                return memFullErr;
            }

            err = filterRecord->bufferProcs->allocateProc(imageDataBufferSize, &imageDataBufferID);

            if (err == noErr)
            {
                void* imageDataBuffer = filterRecord->bufferProcs->lockProc(imageDataBufferID, false);

                BufferID pngRowStrideBuffer;
                int32 pngRowStrideBufferSize;

                if (!TryMultiplyInt32(maxChunkHeight, sizeof(int32), pngRowStrideBufferSize))
                {
                    // The multiplication would have resulted in an integer overflow / underflow.
                    err = memFullErr;
                }

                if (err == noErr)
                {
                    err = filterRecord->bufferProcs->allocateProc(pngRowStrideBufferSize, &pngRowStrideBuffer);

                    if (err == noErr)
                    {
                        int32* rowStride = reinterpret_cast<int32*>(filterRecord->bufferProcs->lockProc(pngRowStrideBuffer, false));

                        for (int32 y = 0; y < maxChunkHeight; y++)
                        {
                            rowStride[y] = y * filterRecord->inRowBytes;
                        }

                        PixelMemoryDesc dest{};
                        dest.data = imageDataBuffer;
                        dest.bitOffset = 0;
                        dest.depth = filterRecord->depth;

                        if (!TryMultiplyInt32(filterRecord->inColumnBytes, 8, dest.colBits) ||
                            !TryMultiplyInt32(filterRecord->inRowBytes, 8, dest.rowBits))
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
                                png_destroy_write_struct(&pngPtr, &infoPtr);
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

                                        dest.bitOffset = i * bitDepth;

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

                                    const int32 rowCount = writeRect.bottom - writeRect.top;

                                    if (bitDepth == 16)
                                    {
                                        size_t length = static_cast<size_t>(rowCount) * width * numberOfChannels;

                                        ScaleSixteenBitDataToPNG(static_cast<uint16*>(dest.data), length);
                                    }

                                    for (int32 i = 0; i < rowCount; i++)
                                    {
                                        png_bytep row = static_cast<png_bytep>(imageDataBuffer) + rowStride[i];

                                        png_write_row(pngPtr, row);
                                    }

                                    err = writerState->GetWriteErrorCode();

                                    if (err != noErr)
                                    {
                                        break;
                                    }
                                }

                                if (err == noErr)
                                {
                                    png_write_end(pngPtr, infoPtr);

                                    err = writerState->GetWriteErrorCode();
                                }
                            }
                        }

                        filterRecord->bufferProcs->unlockProc(pngRowStrideBuffer);
                        filterRecord->bufferProcs->freeProc(pngRowStrideBuffer);
                    }
                }

                filterRecord->bufferProcs->unlockProc(imageDataBufferID);
                filterRecord->bufferProcs->freeProc(imageDataBufferID);
            }
        }

        png_destroy_write_struct(&pngPtr, &infoPtr);
        SetInputRect(filterRecord, 0, 0, 0, 0);
        filterRecord->inData = nullptr;

        return err;
    }
}

OSErr SaveActiveLayer(
    const boost::filesystem::path& path,
    FilterRecordPtr filterRecord)
{
    OSErr err = noErr;

    try
    {
        std::unique_ptr<FileHandle> file;

        err = OpenFile(path, FileOpenMode::Write, file);

        if (err == noErr)
        {
            std::unique_ptr<PngWriterState> writerState = std::make_unique<PngWriterState>(file.get());

            err = SaveActiveLayerImpl(filterRecord, writerState.get());
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

                err = GetTemporaryFileName(outputDir, imagePath, ".png");

                if (err == noErr)
                {
                    std::unique_ptr<FileHandle> file;

                    err = OpenFile(imagePath, FileOpenMode::Write, file);

                    if (err == noErr)
                    {
                        std::unique_ptr<PngWriterState> writerState = std::make_unique<PngWriterState>(file.get());

                        err = SaveDocumentLayer(filterRecord, layerDescriptor, writerState.get());

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
