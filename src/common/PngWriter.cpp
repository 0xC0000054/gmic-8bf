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

#include "PngWriter.h"
#include "FileUtil.h"
#include "Gmic8bfImageHeader.h"
#include "Utilities.h"
#include <boost/endian.hpp>
#include <boost/predef.h>
#include <new>
#include <string>
#include <setjmp.h>
#include <png.h>

namespace
{
    struct PngErrorData
    {
        PngErrorData()
            : errorCode(noErr), errorMessageSet(false), errorMessage()
        {
        }

        OSErr GetWriteErrorCode() const noexcept
        {
            return errorCode;
        }

        std::string GetErrorMessage() const
        {
            return errorMessage;
        }

        void SetErrorMessage(const char* const message) noexcept
        {
            if (!errorMessageSet)
            {
                errorMessageSet = true;
                errorCode = ioErr;

                try
                {
                    errorMessage = message;
                }
                catch (...)
                {
                }
            }
        }

    private:
        OSErr errorCode;
        bool errorMessageSet;
        std::string errorMessage;
    };

    void PngWriteErrorHandler(png_structp png, png_const_charp errorDescription)
    {
        PngErrorData* errorData = static_cast<PngErrorData*>(png_get_error_ptr(png));

        if (errorData)
        {
            DebugOut("LibPng error: %s", errorDescription);

            errorData->SetErrorMessage(errorDescription);
        }

        longjmp(png_jmpbuf(png), 1);
    }

    void WritePngData(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        FileHandle* fileHandle = static_cast<FileHandle*>(png_get_io_ptr(png_ptr));

        try
        {
            WriteFile(fileHandle, data, length);
        }
        catch (const std::exception& e)
        {
            png_error(png_ptr, e.what());
        }
        catch (...)
        {
            png_error(png_ptr, "An unknown error occurred when writing the PNG data.");
        }
    }

    void FlushPngData(png_structp png_ptr)
    {
        (void)png_ptr;
        // We let the OS handle flushing any buffered data to disk.
    }

    void FillInputDataBuffer(
        FileHandle* fileHandle,
        uint8* buffer,
        int32 rowBytes,
        int32 height,
        PngErrorData* errorData)
    {
        try
        {
            size_t bufferSize = static_cast<size_t>(rowBytes) * static_cast<size_t>(height);

            ReadFile(fileHandle, buffer, bufferSize);
        }
        catch (const std::exception& e)
        {
            errorData->SetErrorMessage(e.what());
        }
        catch (...)
        {
            errorData->SetErrorMessage("An unknown error occurred when reading the G'MIC image data.");
        }
    }

    int32 GetMaxInputChunkHeight(
        const FilterRecordPtr filterRecord,
        int32 inputRowBytes,
        int32 inputHeight)
    {
        const int32 maxBufferSpace = filterRecord->bufferProcs->spaceProc();
        const int32 maxHeight = std::min(GetTileHeight(filterRecord->outTileHeight), inputHeight);

        return std::min(std::max(maxBufferSpace / inputRowBytes, 1), maxHeight);
    }

    OSErr SavePngImage(
        const FilterRecordPtr filterRecord,
        FileHandle* inputFile,
        const Gmic8bfImageHeader& inputFileHeader,
        FileHandle* outputFile,
        PngErrorData* errorData)
    {
        OSErr err = noErr;

        BufferID inputDataBufferID;
        bool inputBufferValid = false;

        png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, static_cast<png_voidp>(errorData), PngWriteErrorHandler, nullptr);

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

        png_set_write_fn(pngPtr, static_cast<png_voidp>(outputFile), WritePngData, FlushPngData);

        // Disable "C4611: interaction between '_setjmp' and C++ object destruction is non-portable" on MSVC.
        // This method does not create any C++ objects.
#if _MSC_VER
#pragma warning(disable:4611)
#endif
        if (setjmp(png_jmpbuf(pngPtr)))
        {
            png_destroy_write_struct(&pngPtr, &infoPtr);

            if (inputBufferValid)
            {
                filterRecord->bufferProcs->unlockProc(inputDataBufferID);
                filterRecord->bufferProcs->freeProc(inputDataBufferID);
                inputBufferValid = false;
            }

            return ioErr;
        }

#if _MSC_VER
#pragma warning(default:4611)
#endif

        const int32 width = inputFileHeader.GetWidth();
        const int32 height = inputFileHeader.GetHeight();
        const int32 numberOfChannels = inputFileHeader.GetNumberOfChannels();
        const int32 bitsPerChannel = inputFileHeader.GetBitsPerChannel();

        int colorType = 0;

        switch (numberOfChannels)
        {
        case 1:
            colorType = PNG_COLOR_TYPE_GRAY;
            break;
        case 2:
            colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
            break;
        case 3:
            colorType = PNG_COLOR_TYPE_RGB;
            break;
        case 4:
            colorType = PNG_COLOR_TYPE_RGB_ALPHA;
            break;
        default:
            png_destroy_write_struct(&pngPtr, &infoPtr);
            return ShowErrorMessage("Unsupported Gmic8bfImage channel count.", filterRecord, paramErr);
        }

        png_set_IHDR(
            pngPtr,
            infoPtr,
            static_cast<png_uint_32>(width),
            static_cast<png_uint_32>(height),
            bitsPerChannel,
            colorType,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

        png_write_info(pngPtr, infoPtr);

        err = errorData->GetWriteErrorCode();

        if (err == noErr)
        {
            int32 inputRowBytes = 0;

            if (!TryMultiplyInt32(width, numberOfChannels, inputRowBytes))
            {
                // The multiplication would have resulted in an integer overflow / underflow.
                png_destroy_write_struct(&pngPtr, &infoPtr);
                return memFullErr;
            }

            if (bitsPerChannel == 16)
            {
                if (!TryMultiplyInt32(inputRowBytes, 2, inputRowBytes))
                {
                    // The multiplication would have resulted in an integer overflow / underflow.
                    png_destroy_write_struct(&pngPtr, &infoPtr);
                    return memFullErr;
                }
            }

            const int32 maxInputChunkHeight = GetMaxInputChunkHeight(filterRecord, inputRowBytes, height);

            int32 inputImageBufferSize;

            if (!TryMultiplyInt32(inputRowBytes, maxInputChunkHeight, inputImageBufferSize))
            {
                // The multiplication would have resulted in an integer overflow / underflow.
                err = memFullErr;
            }

            if (err == noErr)
            {
                err = filterRecord->bufferProcs->allocateProc(inputImageBufferSize, &inputDataBufferID);

                if (err == noErr)
                {
                    inputBufferValid = true;
                    uint8* inputBuffer = reinterpret_cast<uint8*>(filterRecord->bufferProcs->lockProc(inputDataBufferID, false));

                    const int32 left = 0;
                    const int32 right = width;

                    for (int32 y = 0; y < height; y += maxInputChunkHeight)
                    {
                        const int32 top = y;
                        const int32 bottom = std::min(y + maxInputChunkHeight, height);

                        const int32 rowCount = bottom - top;

                        FillInputDataBuffer(
                            inputFile,
                            inputBuffer,
                            inputRowBytes,
                            rowCount,
                            errorData);

                        err = errorData->GetWriteErrorCode();

                        if (err != noErr)
                        {
                            break;
                        }

                        // PNG uses big-endian byte order for 16-bit image data, so we need to
                        // byte-swap on little-endian platforms.
#if BOOST_ENDIAN_LITTLE_BYTE
                        if (bitsPerChannel == 16)
                        {
                            const size_t sampleCount = static_cast<size_t>(width) * rowCount * numberOfChannels;
                            uint16* data = reinterpret_cast<uint16*>(inputBuffer);

                            // This loop should be automatically vectorized by the compiler.
                            for (size_t i = 0; i < sampleCount; i++)
                            {
                                boost::endian::endian_reverse_inplace(data[i]);
                            }
                        }
#endif // BOOST_ENDIAN_LITTLE_BYTE

                        for (int32 i = 0; i < rowCount; i++)
                        {
                            png_bytep row = inputBuffer + (static_cast<int64>(i) * inputRowBytes);

                            png_write_row(pngPtr, row);
                        }

                        err = errorData->GetWriteErrorCode();

                        if (err != noErr)
                        {
                            break;
                        }
                    }

                    if (err == noErr)
                    {
                        png_write_end(pngPtr, infoPtr);

                        err = errorData->GetWriteErrorCode();
                    }

                    filterRecord->bufferProcs->unlockProc(inputDataBufferID);
                    filterRecord->bufferProcs->freeProc(inputDataBufferID);
                    inputBufferValid = false;
                }
            }
        }

        png_destroy_write_struct(&pngPtr, &infoPtr);

        return err;
    }
}

void ConvertGmic8bfImageToPng(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& inputFilePath,
    const boost::filesystem::path& outputFilePath)
{
    std::unique_ptr<FileHandle> inputFile = OpenFile(inputFilePath, FileOpenMode::Read);
    Gmic8bfImageHeader inputFileHeader(inputFile.get());

    std::unique_ptr<FileHandle> outputFile = OpenFile(outputFilePath, FileOpenMode::Write);
    std::unique_ptr<PngErrorData> errorData = std::make_unique<PngErrorData>();

    if (SavePngImage(filterRecord, inputFile.get(), inputFileHeader, outputFile.get(), errorData.get()) != noErr)
    {
        std::string errorMessage = errorData->GetErrorMessage();

        if (errorMessage.empty())
        {
            throw OSErrException(writErr);
        }
        else
        {
            throw std::runtime_error(errorMessage);
        }
    }
}
