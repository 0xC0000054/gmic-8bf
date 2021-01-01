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

#include "PngReader.h"
#include "FileUtil.h"
#include <algorithm>
#include <new>
#include <boost/predef.h>
#include <setjmp.h>
#include <png.h>

namespace
{
    struct PngReaderState
    {
        FileHandle* fileHandle;

        PngReaderState(FileHandle* handle)
            : fileHandle(handle), readError(noErr), pngErrorSet(false), errorMessage()
        {
        }

        OSErr GetReadErrorCode() const noexcept
        {
            return readError;
        }

        std::string GetErrorMessage() const
        {
            return errorMessage;
        }

        void SetFileReadError(OSErr err) noexcept
        {
            if (err != noErr)
            {
                readError = err;
            }
        }

        void SetPngError(png_const_charp errorDescription)
        {
            if (!pngErrorSet)
            {
                pngErrorSet = true;
                readError = ioErr;

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
        OSErr readError;
        bool pngErrorSet;
        std::string errorMessage;
    };

    void PngReadErrorHandler(png_structp png, png_const_charp errorDescription)
    {
        PngReaderState* state = static_cast<PngReaderState*>(png_get_error_ptr(png));

        if (state)
        {
            DebugOut("LibPng error: %s", errorDescription);

            state->SetPngError(errorDescription);
        }
    }

    void ReadPngData(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        PngReaderState* state = static_cast<PngReaderState*>(png_get_io_ptr(png_ptr));

        state->SetFileReadError(ReadFile(state->fileHandle, data, length));
    }

    void CopyDataFromPngEightBitsPerChannel(
        const uint8* const pngBuffer,
        int32 pngColumnStep,
        int32 pngRowBytes,
        FilterRecordPtr filterRecord,
        int32 imageWidth,
        int32 imageHeight,
        bool premultiplyAlpha)
    {
        uint8* outData = static_cast<uint8*>(filterRecord->outData);
        const uint8* maskData = filterRecord->haveMask ? static_cast<const uint8*>(filterRecord->maskData) : nullptr;

        if (pngColumnStep == 1)
        {
            for (int32 y = 0; y < imageHeight; y++)
            {
                const uint8* pngPixel = pngBuffer + (static_cast<int64>(y) * pngRowBytes);
                uint8* pixel = outData + (static_cast<int64>(y) * filterRecord->outRowBytes);
                const uint8* mask = filterRecord->haveMask ? maskData + (static_cast<int64>(y) * filterRecord->maskRowBytes) : nullptr;

                for (int32 x = 0; x < imageWidth; x++)
                {
                    // Clip the output to the mask, if one is present.
                    if (mask == nullptr || *mask != 0)
                    {
                        uint8 gray = pngPixel[0];

                        switch (filterRecord->outColumnBytes)
                        {
                        case 1:
                            pixel[0] = gray;
                            break;
                        case 3:
                            pixel[0] = gray;
                            pixel[1] = gray;
                            pixel[2] = gray;
                            break;
                        }
                    }

                    pngPixel++;
                    pixel += filterRecord->outColumnBytes;
                    if (mask != nullptr)
                    {
                        mask++;
                    }
                }
            }
        }
        else
        {
            for (int32 y = 0; y < imageHeight; y++)
            {
                const uint8* pngPixel = pngBuffer + (static_cast<int64>(y) * pngRowBytes);
                uint8* pixel = outData + (static_cast<int64>(y) * filterRecord->outRowBytes);
                const uint8* mask = filterRecord->haveMask ? maskData + (static_cast<int64>(y) * filterRecord->maskRowBytes) : nullptr;

                for (int32 x = 0; x < imageWidth; x++)
                {
                    // Clip the output to the mask, if one is present.
                    if (mask == nullptr || *mask != 0)
                    {
                        if (premultiplyAlpha)
                        {
                            float alpha = static_cast<float>(pngPixel[3]) / 255.0f;

                            switch (filterRecord->outColumnBytes)
                            {
                            case 1:
                                pixel[0] = static_cast<uint8>((static_cast<float>(pngPixel[0]) * alpha) + 0.5f);
                                break;
                            case 3:
                                pixel[0] = static_cast<uint8>((static_cast<float>(pngPixel[0]) * alpha) + 0.5f);
                                pixel[1] = static_cast<uint8>((static_cast<float>(pngPixel[1]) * alpha) + 0.5f);
                                pixel[2] = static_cast<uint8>((static_cast<float>(pngPixel[2]) * alpha) + 0.5f);
                                break;
                            }
                        }
                        else
                        {
                            switch (filterRecord->outColumnBytes)
                            {
                            case 1:
                                pixel[0] = pngPixel[0];
                                break;
                            case 2:
                                pixel[0] = pngPixel[0];
                                pixel[1] = pngPixel[3];
                                break;
                            case 3:
                                pixel[0] = pngPixel[0];
                                pixel[1] = pngPixel[1];
                                pixel[2] = pngPixel[2];
                                break;
                            case 4:
                                pixel[0] = pngPixel[0];
                                pixel[1] = pngPixel[1];
                                pixel[2] = pngPixel[2];
                                pixel[3] = pngPixel[3];
                                break;
                            }
                        }
                    }

                    pngPixel += pngColumnStep;
                    pixel += filterRecord->outColumnBytes;
                    if (mask != nullptr)
                    {
                        mask++;
                    }
                }
            }
        }
    }

    std::vector<uint16> BuildSixteenBitPngToHostLUT()
    {
        std::vector<uint16> sixteenBitPngToHostLUT;
        sixteenBitPngToHostLUT.reserve(65536);

        for (size_t i = 0; i < sixteenBitPngToHostLUT.capacity(); i++)
        {
            // According to the Photoshop SDK 16-bit image data is stored in the range of [0, 32768].
            sixteenBitPngToHostLUT.push_back(static_cast<uint16>(((i * 32768) + 32767) / 65535));
        }

        return sixteenBitPngToHostLUT;
    }

    OSErr CopyDataFromPngSixteenBitsPerChannel(
        const uint8* const pngBuffer,
        int32 pngColumnStep,
        int32 pngStride,
        FilterRecordPtr filterRecord,
        int32 imageWidth,
        int32 imageHeight,
        bool premultiplyAlpha)
    {
        try
        {
            static const std::vector<uint16> pngToHostLUT = BuildSixteenBitPngToHostLUT();

            const int32 outChannelCount = filterRecord->outColumnBytes / 2;
            const int32 outRowWords = filterRecord->outRowBytes / 2;

            uint16* outData = static_cast<uint16*>(filterRecord->outData);
            const uint8* maskData = filterRecord->haveMask ? static_cast<const uint8*>(filterRecord->maskData) : nullptr;

            if (pngColumnStep == 1)
            {
                for (int32 y = 0; y < imageHeight; y++)
                {
                    const uint16* pngPixel = reinterpret_cast<const uint16*>(pngBuffer + (static_cast<int64>(y) * pngStride));
                    uint16* pixel = outData + (static_cast<int64>(y) * outRowWords);
                    const uint8* mask = filterRecord->haveMask ? maskData + (static_cast<int64>(y) * filterRecord->maskRowBytes) : nullptr;

                    for (int32 x = 0; x < imageWidth; x++)
                    {
                        // Clip the output to the mask, if one is present.
                        if (mask == nullptr || *mask != 0)
                        {
                            uint16 gray = pngToHostLUT[pngPixel[0]];

                            switch (outChannelCount)
                            {
                            case 1:
                                pixel[0] = gray;
                                break;
                            case 3:
                                pixel[0] = gray;
                                pixel[1] = gray;
                                pixel[2] = gray;
                                break;
                            }
                        }

                        pngPixel++;
                        pixel += outChannelCount;
                        if (mask != nullptr)
                        {
                            mask++;
                        }
                    }
                }
            }
            else
            {
                for (int32 y = 0; y < imageHeight; y++)
                {
                    const uint16* pngPixel = reinterpret_cast<const uint16*>(pngBuffer + (static_cast<int64>(y) * pngStride));
                    uint16* pixel = outData + (static_cast<int64>(y) * outRowWords);
                    const uint8* mask = filterRecord->haveMask ? maskData + (static_cast<int64>(y) * filterRecord->maskRowBytes) : nullptr;

                    for (int32 x = 0; x < imageWidth; x++)
                    {
                        // Clip the output to the mask, if one is present.
                        if (mask == nullptr || *mask != 0)
                        {
                            if (premultiplyAlpha)
                            {
                                double alpha = static_cast<double>(pngPixel[3]) / 65535.0;

                                switch (outChannelCount)
                                {
                                case 1:
                                    pixel[0] = pngToHostLUT[static_cast<uint16>((static_cast<double>(pngPixel[0]) * alpha) + 0.5)];
                                    break;
                                case 3:
                                    pixel[0] = pngToHostLUT[static_cast<uint16>((static_cast<double>(pngPixel[0]) * alpha) + 0.5)];
                                    pixel[1] = pngToHostLUT[static_cast<uint16>((static_cast<double>(pngPixel[1]) * alpha) + 0.5)];
                                    pixel[2] = pngToHostLUT[static_cast<uint16>((static_cast<double>(pngPixel[2]) * alpha) + 0.5)];
                                    break;
                                }
                            }
                            else
                            {
                                switch (outChannelCount)
                                {
                                case 1:
                                    pixel[0] = pngToHostLUT[pngPixel[0]];
                                    break;
                                case 2:
                                    pixel[0] = pngToHostLUT[pngPixel[0]];
                                    pixel[1] = pngToHostLUT[pngPixel[3]];
                                    break;
                                case 3:
                                    pixel[0] = pngToHostLUT[pngPixel[0]];
                                    pixel[1] = pngToHostLUT[pngPixel[1]];
                                    pixel[2] = pngToHostLUT[pngPixel[2]];
                                    break;
                                case 4:
                                    pixel[0] = pngToHostLUT[pngPixel[0]];
                                    pixel[1] = pngToHostLUT[pngPixel[1]];
                                    pixel[2] = pngToHostLUT[pngPixel[2]];
                                    pixel[3] = pngToHostLUT[pngPixel[3]];
                                    break;
                                }
                            }
                        }

                        pngPixel += pngColumnStep;
                        pixel += outChannelCount;
                        if (mask != nullptr)
                        {
                            mask++;
                        }
                    }
                }
            }
        }
        catch (const std::bad_alloc&)
        {
            return memFullErr;
        }

        return noErr;
    }

    OSErr ImageSizeMatchesDocument(const VPoint& documentSize, PngReaderState* readerState, bool& imageSizeMatchesDocument)
    {
        imageSizeMatchesDocument = false;

        OSErr err = noErr;

        png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, static_cast<png_voidp>(readerState), PngReadErrorHandler, nullptr);

        if (!pngPtr)
        {
            return memFullErr;
        }

        png_infop infoPtr = png_create_info_struct(pngPtr);

        if (!infoPtr)
        {
            png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

            return memFullErr;
        }

        png_set_read_fn(pngPtr, static_cast<png_voidp>(readerState), ReadPngData);

        // Disable "C4611: interaction between '_setjmp' and C++ object destruction is non-portable" on MSVC.
        // This method does not create any C++ objects.
#if _MSC_VER
#pragma warning(disable:4611)
#endif
        if (setjmp(png_jmpbuf(pngPtr)))
        {
            png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

            return ioErr;
        }

#if _MSC_VER
#pragma warning(default:4611)
#endif

        png_read_info(pngPtr, infoPtr);

        err = readerState->GetReadErrorCode();
        if (err == noErr)
        {
            png_uint_32 pngWidth;
            png_uint_32 pngHeight;
            int bitDepth;
            int colorType;
            int interlaceType;
            int compressionType;
            int filterType;

            if (!png_get_IHDR(
                pngPtr,
                infoPtr,
                &pngWidth,
                &pngHeight,
                &bitDepth,
                &colorType,
                &interlaceType,
                &compressionType,
                &filterType))
            {
                err = readErr;
            }

            if (err == noErr)
            {
                const png_uint_32 documentWidth = static_cast<const png_uint_32>(documentSize.h);
                const png_uint_32 documentHeight = static_cast<const png_uint_32>(documentSize.v);

                imageSizeMatchesDocument = pngWidth == documentWidth && pngHeight == documentHeight;
            }
        }

        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

        return err;
    }

    OSErr ReadPngImage(FilterRecordPtr filterRecord, PngReaderState* readerState)
    {
        OSErr err = noErr;

        png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, static_cast<png_voidp>(readerState), PngReadErrorHandler, nullptr);

        if (!pngPtr)
        {
            return memFullErr;
        }

        png_infop infoPtr = png_create_info_struct(pngPtr);

        if (!infoPtr)
        {
            png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

            return memFullErr;
        }

        png_set_read_fn(pngPtr, static_cast<png_voidp>(readerState), ReadPngData);

        // Disable "C4611: interaction between '_setjmp' and C++ object destruction is non-portable" on MSVC.
        // This method does not create any C++ objects.
#if _MSC_VER
#pragma warning(disable:4611)
#endif
        if (setjmp(png_jmpbuf(pngPtr)))
        {
            png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

            return ioErr;
        }

#if _MSC_VER
#pragma warning(default:4611)
#endif

        png_read_info(pngPtr, infoPtr);

        err = readerState->GetReadErrorCode();
        if (err == noErr)
        {
            png_uint_32 pngWidth;
            png_uint_32 pngHeight;
            int bitDepth;
            int colorType;
            int interlaceType;
            int compressionType;
            int filterType;

            if (!png_get_IHDR(
                pngPtr,
                infoPtr,
                &pngWidth,
                &pngHeight,
                &bitDepth,
                &colorType,
                &interlaceType,
                &compressionType,
                &filterType))
            {
                err = readErr;
            }

            if (err == noErr)
            {
                if (pngWidth > static_cast<png_uint_32>(std::numeric_limits<int32>::max()) ||
                    pngHeight > static_cast<png_uint_32>(std::numeric_limits<int32>::max()))
                {
                    // The image width and/or height is greater than the largest positive int32 value.
                    png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

                    return memFullErr;
                }

                const int32 width = static_cast<int32>(pngWidth);
                const int32 height = static_cast<int32>(pngHeight);
                const bool includeTransparency = colorType == PNG_COLOR_TYPE_RGB_ALPHA && filterRecord->outLayerPlanes != 0 && filterRecord->outTransparencyMask != 0;

                const VPoint imageSize = GetImageSize(filterRecord);

                const int32 maxWidth = std::min(imageSize.h, width);
                const int32 maxHeight = std::min(imageSize.v, height);

                int32 numberOfChannels;

                switch (filterRecord->imageMode)
                {
                case plugInModeGrayScale:
                case plugInModeGray16:
                    numberOfChannels = includeTransparency ? 2 : 1;
                    break;
                case plugInModeRGBColor:
                case plugInModeRGB48:
                    numberOfChannels = includeTransparency ? 4 : 3;
                    break;
                default:
                    png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
                    return filterBadMode;
                }

                if (bitDepth == 16)
                {
                    filterRecord->outColumnBytes = numberOfChannels * 2;
                    filterRecord->outPlaneBytes = 2;

#if BOOST_ENDIAN_LITTLE_BYTE
                    png_set_swap(pngPtr);
#endif
                }
                else
                {
                    filterRecord->outColumnBytes = numberOfChannels;
                    filterRecord->outPlaneBytes = 1;
                }

                if (!TryMultiplyInt32(width, filterRecord->outColumnBytes, filterRecord->outRowBytes))
                {
                    // The multiplication would have resulted in an integer overflow / underflow.
                    png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

                    return memFullErr;
                }

                filterRecord->outLoPlane = 0;
                filterRecord->outHiPlane = static_cast<int16>(numberOfChannels - 1);

                const bool premultiplyAlpha = colorType == PNG_COLOR_TYPE_RGB_ALPHA && !includeTransparency;
                const int32 pngColumnStep = png_get_channels(pngPtr, infoPtr);
                int32 pngRowBytes;
                if (!TryMultiplyInt32(width, pngColumnStep, pngRowBytes))
                {
                    // The multiplication would have resulted in an integer overflow / underflow.
                    png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

                    return memFullErr;
                }

                if (bitDepth == 16)
                {
                    if (!TryMultiplyInt32(pngRowBytes, 2, pngRowBytes))
                    {
                        // The multiplication would have resulted in an integer overflow / underflow.
                        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

                        return memFullErr;
                    }
                }

                if (interlaceType == PNG_INTERLACE_NONE)
                {
                    // A non-interlaced image can be read in smaller chunks.
                    const int32 tileHeight = GetTileHeight(filterRecord->outTileHeight);
                    const int32 maxChunkHeight = std::min(tileHeight, height);

                    BufferID pngImageRows;
                    int32 pngImageRowsSize = 0;

                    if (!TryMultiplyInt32(maxChunkHeight, pngRowBytes, pngImageRowsSize))
                    {
                        // The multiplication would have resulted in an integer overflow / underflow.
                        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

                        return memFullErr;
                    }

                    err = filterRecord->bufferProcs->allocateProc(pngImageRowsSize, &pngImageRows);

                    if (err == noErr)
                    {
                        uint8* buffer = reinterpret_cast<uint8*>(filterRecord->bufferProcs->lockProc(pngImageRows, false));

                        // The LibPNG row pointers are allocated using new instead of the Buffer suite
                        // because some hosts crash when trying to free nested Buffer suite buffers.
                        //
                        // We do not use an unique_ptr because LibPNG uses setjmp for error handling, and that
                        // is not guaranteed to work with the automatic C++ object destruction.
                        png_bytepp pngRows = new (std::nothrow) png_bytep[maxChunkHeight];

                        if (pngRows == nullptr)
                        {
                            err = memFullErr;
                        }
                        else
                        {
                            for (int32 y = 0; y < maxChunkHeight; y++)
                            {
                                pngRows[y] = buffer + (static_cast<int64>(y) * pngRowBytes);
                            }

                            if (filterRecord->haveMask)
                            {
                                filterRecord->maskRate = int2fixed(1);
                            }

                            const int32 left = 0;
                            const int32 right = maxWidth;

                            for (int32 y = 0; y < maxHeight; y += maxChunkHeight)
                            {
                                const int32 top = y;
                                const int32 bottom = std::min(top + maxChunkHeight, maxHeight);

                                SetOutputRect(filterRecord, top, left, bottom, right);

                                if (filterRecord->haveMask)
                                {
                                    SetMaskRect(filterRecord, top, left, bottom, right);
                                }

                                err = filterRecord->advanceState();
                                if (err != noErr)
                                {
                                    break;
                                }

                                const int32 rowCount = bottom - top;

                                png_read_rows(pngPtr, pngRows, nullptr, static_cast<png_uint_32>(rowCount));

                                err = readerState->GetReadErrorCode();

                                if (err != noErr)
                                {
                                    break;
                                }

                                if (bitDepth == 16)
                                {
                                    err = CopyDataFromPngSixteenBitsPerChannel(
                                        buffer,
                                        pngColumnStep,
                                        pngRowBytes,
                                        filterRecord,
                                        maxWidth,
                                        rowCount,
                                        premultiplyAlpha);

                                    if (err != noErr)
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    CopyDataFromPngEightBitsPerChannel(
                                        buffer,
                                        pngColumnStep,
                                        pngRowBytes,
                                        filterRecord,
                                        maxWidth,
                                        rowCount,
                                        premultiplyAlpha);
                                }
                            }

                            delete[] pngRows;
                        }

                        filterRecord->bufferProcs->unlockProc(pngImageRows);
                        filterRecord->bufferProcs->freeProc(pngImageRows);
                    }
                }
                else if (interlaceType == PNG_INTERLACE_ADAM7)
                {
                    // If the image is interlaced we need to load the entire image into memory.
                    BufferID pngImageRows;
                    int32 pngImageRowsSize = 0;

                    if (!TryMultiplyInt32(height, pngRowBytes, pngImageRowsSize))
                    {
                        // The multiplication would have resulted in an integer overflow / underflow.
                        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
                        return memFullErr;
                    }

                    err = filterRecord->bufferProcs->allocateProc(pngImageRowsSize, &pngImageRows);

                    if (err == noErr)
                    {
                        uint8* buffer = reinterpret_cast<uint8*>(filterRecord->bufferProcs->lockProc(pngImageRows, false));

                        // The LibPNG row pointers are allocated using new instead of the Buffer suite
                        // because some hosts crash when trying to free nested Buffer suite buffers.
                        //
                        // We do not use an unique_ptr because LibPNG uses setjmp for error handling, and that
                        // is not guaranteed to work with the automatic C++ object destruction.
                        png_bytepp pngRows = new (std::nothrow) png_bytep[height];

                        if (pngRows == nullptr)
                        {
                            err = memFullErr;
                        }
                        else
                        {
                            for (int32 y = 0; y < height; y++)
                            {
                                pngRows[y] = buffer + (static_cast<int64>(y) * pngRowBytes);
                            }

                            SetOutputRect(filterRecord, 0, 0, maxHeight, maxWidth);

                            if (filterRecord->haveMask)
                            {
                                filterRecord->maskRate = int2fixed(1);
                                SetMaskRect(filterRecord, 0, 0, maxHeight, maxWidth);
                            }

                            err = filterRecord->advanceState();

                            if (err == noErr)
                            {
                                png_read_rows(pngPtr, pngRows, nullptr, static_cast<png_uint_32>(height));

                                err = readerState->GetReadErrorCode();

                                if (err == noErr)
                                {
                                    if (bitDepth == 16)
                                    {
                                        err = CopyDataFromPngSixteenBitsPerChannel(
                                            buffer,
                                            pngColumnStep,
                                            pngRowBytes,
                                            filterRecord,
                                            maxWidth,
                                            maxHeight,
                                            premultiplyAlpha);
                                    }
                                    else
                                    {
                                        CopyDataFromPngEightBitsPerChannel(
                                            buffer,
                                            pngColumnStep,
                                            pngRowBytes,
                                            filterRecord,
                                            maxWidth,
                                            maxHeight,
                                            premultiplyAlpha);
                                    }
                                }
                            }

                            delete[] pngRows;
                        }

                        filterRecord->bufferProcs->unlockProc(pngImageRows);
                        filterRecord->bufferProcs->freeProc(pngImageRows);
                    }
                }
                else
                {
                    // Unsupported interlacing type.
                    err = readErr;
                }
            }
        }

        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
        // Do not set the FilterRecord data pointers to NULL, some hosts
        // (e.g. XnView) will crash if they are set to NULL by a plug-in.
        SetOutputRect(filterRecord, 0, 0, 0, 0);
        SetMaskRect(filterRecord, 0, 0, 0, 0);

        return err;
    }
}

OSErr PngImageSizeMatchesDocument(
    const boost::filesystem::path& path,
    const VPoint& documentSize,
    bool& imageSizeMatchesDocument)
{
    OSErr err = noErr;

    try
    {
        std::unique_ptr<FileHandle> file;

        err = OpenFile(path, FileOpenMode::Read, file);

        if (err == noErr)
        {
            std::unique_ptr<PngReaderState> readerState = std::make_unique<PngReaderState>(file.get());

            err = ImageSizeMatchesDocument(documentSize, readerState.get(), imageSizeMatchesDocument);
        }
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }

    return err;
}

OSErr LoadPngImage(const boost::filesystem::path& path, FilterRecord* filterRecord)
{
    std::unique_ptr<FileHandle> file;

    OSErr err = OpenFile(path, FileOpenMode::Read, file);

    if (err == noErr)
    {
        try
        {
            std::unique_ptr<PngReaderState> readerState = std::make_unique<PngReaderState>(file.get());

            err = ReadPngImage(filterRecord, readerState.get());
        }
        catch (const std::bad_alloc&)
        {
            err = memFullErr;
        }
    }

    return err;
}
