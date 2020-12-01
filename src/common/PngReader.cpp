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

#include "PngReader.h"
#include "FileUtil.h"
#include <algorithm>
#include <array>
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

    constexpr std::array<float, 256> BuildPremultipliedAlphaLookupTable()
    {
        std::array<float, 256> items = {};

        for (size_t i = 0; i < items.size(); i++)
        {
            items[i] = static_cast<float>(i) / 255.0f;
        }

        return items;
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
            png_uint_32 width;
            png_uint_32 height;
            int bitDepth;
            int colorType;
            int interlaceType;
            int compressionType;
            int filterType;

            png_get_IHDR(
                pngPtr,
                infoPtr,
                &width,
                &height,
                &bitDepth,
                &colorType,
                &interlaceType,
                &compressionType,
                &filterType);

            const bool includeTransparency = colorType == PNG_COLOR_TYPE_RGB_ALPHA && filterRecord->outLayerPlanes != 0 && filterRecord->outTransparencyMask != 0;

            const VPoint imageSize = GetImageSize(filterRecord);

            const int32 maxWidth = static_cast<int32>(std::min(static_cast<png_uint_32>(imageSize.h), width));
            const int32 maxHeight = static_cast<int32>(std::min(static_cast<png_uint_32>(imageSize.v), height));;

            filterRecord->outColumnBytes = includeTransparency ? 4 : 3;
            filterRecord->outPlaneBytes = 1;
            filterRecord->outRowBytes = maxWidth * filterRecord->outColumnBytes;
            filterRecord->outLoPlane = 0;
            filterRecord->outHiPlane = static_cast<int16>(filterRecord->outColumnBytes - 1);

            if (err == noErr)
            {
                static constexpr std::array<float, 256> premultipliedAlphaTable = BuildPremultipliedAlphaLookupTable();

                const int32 pngColumnBytes = colorType == PNG_COLOR_TYPE_RGB_ALPHA ? 4 : 3;
                const int32 pngRowBytes = maxWidth * pngColumnBytes;
                const bool premultiplyAlpha = colorType == PNG_COLOR_TYPE_RGB_ALPHA && !includeTransparency;

                if (interlaceType == PNG_INTERLACE_NONE)
                {
                    // A non-interlaced image can be read in smaller chunks.
                    const int32 tileHeight = filterRecord->outTileHeight != 0 ? filterRecord->outTileHeight : 256;
                    const int32 maxChunkHeight = static_cast<int32>(std::min(static_cast<png_uint_32>(tileHeight), height));

                    BufferID pngImageRows;

                    err = filterRecord->bufferProcs->allocateProc(maxChunkHeight * pngRowBytes, &pngImageRows);

                    if (err == noErr)
                    {
                        uint8* buffer = reinterpret_cast<uint8*>(filterRecord->bufferProcs->lockProc(pngImageRows, false));

                        BufferID rowStrideBuffer;

                        err = filterRecord->bufferProcs->allocateProc(maxChunkHeight * sizeof(png_bytep), &rowStrideBuffer);

                        if (err == noErr)
                        {
                            png_bytepp pngRows = reinterpret_cast<png_bytepp>(filterRecord->bufferProcs->lockProc(rowStrideBuffer, false));

                            for (int16 y = 0; y < maxChunkHeight; y++)
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

                                const png_uint_32 rowCount = static_cast<png_uint_32>(bottom - top);

                                png_read_rows(pngPtr, pngRows, nullptr, rowCount);

                                err = readerState->GetReadErrorCode();

                                if (err != noErr)
                                {
                                    break;
                                }

                                for (size_t row = 0; row < rowCount; row++)
                                {
                                    const uint8* pngPixel = buffer + (row * pngRowBytes);
                                    uint8* pixel = static_cast<uint8*>(filterRecord->outData) + (row * filterRecord->outRowBytes);
                                    uint8* mask = filterRecord->haveMask ? static_cast<uint8*>(filterRecord->maskData) + (row * filterRecord->maskRowBytes) : nullptr;

                                    for (int16 x = left; x < right; x++)
                                    {
                                        // Clip the output to the mask, if one is present.
                                        if (mask == nullptr || *mask != 0)
                                        {
                                            if (premultiplyAlpha)
                                            {
                                                float alpha = premultipliedAlphaTable[pngPixel[3]];

                                                pixel[0] = static_cast<uint8>((static_cast<float>(pngPixel[0]) * alpha) + 0.5f);
                                                pixel[1] = static_cast<uint8>((static_cast<float>(pngPixel[1]) * alpha) + 0.5f);
                                                pixel[2] = static_cast<uint8>((static_cast<float>(pngPixel[2]) * alpha) + 0.5f);
                                            }
                                            else
                                            {
                                                switch (filterRecord->outColumnBytes)
                                                {
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

                                        pngPixel += pngColumnBytes;
                                        pixel += filterRecord->outColumnBytes;
                                        if (mask != nullptr)
                                        {
                                            mask++;
                                        }
                                    }
                                }
                            }

                            filterRecord->bufferProcs->unlockProc(rowStrideBuffer);
                            filterRecord->bufferProcs->freeProc(rowStrideBuffer);
                        }

                        filterRecord->bufferProcs->unlockProc(pngImageRows);
                        filterRecord->bufferProcs->freeProc(pngImageRows);
                    }
                }
                else if (interlaceType == PNG_INTERLACE_ADAM7)
                {
                    // If the image is interlaced we need to load the entire image into memory.
                    BufferID pngImageRows;

                    err = filterRecord->bufferProcs->allocateProc(maxHeight * pngRowBytes, &pngImageRows);

                    if (err == noErr)
                    {
                        uint8* buffer = reinterpret_cast<uint8*>(filterRecord->bufferProcs->lockProc(pngImageRows, false));

                        BufferID pngRowBuffer;

                        err = filterRecord->bufferProcs->allocateProc(maxHeight * sizeof(png_bytep), &pngRowBuffer);

                        if (err == noErr)
                        {
                            png_bytepp pngRows = reinterpret_cast<png_bytepp>(filterRecord->bufferProcs->lockProc(pngRowBuffer, false));

                            for (int16 y = 0; y < maxHeight; y++)
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
                                const png_uint_32 rowCount = static_cast<png_uint_32>(maxHeight);

                                png_read_rows(pngPtr, pngRows, nullptr, rowCount);

                                err = readerState->GetReadErrorCode();

                                if (err == noErr)
                                {
                                    for (size_t y = 0; y < rowCount; y++)
                                    {
                                        const uint8* pngPixel = buffer + (y * pngRowBytes);
                                        uint8* pixel = static_cast<uint8*>(filterRecord->outData) + (y * filterRecord->outRowBytes);
                                        uint8* mask = filterRecord->haveMask ? static_cast<uint8*>(filterRecord->maskData) + (y * filterRecord->maskRowBytes) : nullptr;

                                        for (int16 x = 0; x < maxWidth; x++)
                                        {
                                            // Clip the output to the mask, if one is present.
                                            if (mask == nullptr || *mask != 0)
                                            {
                                                if (premultiplyAlpha)
                                                {
                                                    float alpha = premultipliedAlphaTable[pngPixel[3]];

                                                    pixel[0] = static_cast<uint8>((static_cast<float>(pngPixel[0]) * alpha) + 0.5f);
                                                    pixel[1] = static_cast<uint8>((static_cast<float>(pngPixel[1]) * alpha) + 0.5f);
                                                    pixel[2] = static_cast<uint8>((static_cast<float>(pngPixel[2]) * alpha) + 0.5f);
                                                }
                                                else
                                                {
                                                    switch (filterRecord->outColumnBytes)
                                                    {
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

                                            pngPixel += pngColumnBytes;
                                            pixel += filterRecord->outColumnBytes;
                                            if (mask != nullptr)
                                            {
                                                mask++;
                                            }
                                        }
                                    }
                                }
                            }

                            filterRecord->bufferProcs->unlockProc(pngRowBuffer);
                            filterRecord->bufferProcs->freeProc(pngRowBuffer);
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
