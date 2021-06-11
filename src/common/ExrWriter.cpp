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

#include "ExrWriter.h"
#include "FileUtil.h"
#include "Gmic8bfImageHeader.h"
#include "ScopedBufferSuite.h"
#include "Utilities.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 26495 26812)
#endif // _MSC_VER

#include <OpenEXR/ImfRgbaFile.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

namespace
{
    void ConvertInputToOpenExrImage(
        const uint8* inputBuffer,
        int32 inputRowBytes,
        int32 inputChannelCount,
        Imf::Rgba* outputBuffer,
        int32 width,
        int32 height)
    {
        for (int32 y = 0; y < height; y++)
        {
            const float* src = reinterpret_cast<const float*>(inputBuffer + (static_cast<int64>(y) * inputRowBytes));
            Imf::Rgba* dst = &outputBuffer[y * width];

            for (int32 x = 0; x < width; x++)
            {
                switch (inputChannelCount)
                {
                case 1:
                    // OpenExr writes the G channel as Y.
                    dst->r = 0;
                    dst->g = src[0];
                    dst->b = 0;
                    dst->a = 1;
                    break;
                case 2:
                    // OpenExr writes the G channel as Y.
                    dst->r = 0;
                    dst->g = src[0];
                    dst->b = 0;
                    dst->a = src[1];
                    break;
                case 3:
                    dst->r = src[0];
                    dst->g = src[1];
                    dst->b = src[2];
                    dst->a = 1;
                    break;
                case 4:
                    dst->r = src[0];
                    dst->g = src[1];
                    dst->b = src[2];
                    dst->a = src[3];
                    break;
                default:
                    throw std::runtime_error("Unsupported Gmic8bfImage channel count.");
                }

                src += inputChannelCount;
                dst++;
            }
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

    void WriteOpenExrFile(
        const FilterRecordPtr filterRecord,
        const FileHandle* inputFile,
        const Gmic8bfImageHeader& inputFileHeader,
        const boost::filesystem::path& outputFilePath)
    {
        const int32 width = inputFileHeader.GetWidth();
        const int32 height = inputFileHeader.GetHeight();
        const int32 numberOfChannels = inputFileHeader.GetNumberOfChannels();

        assert(!inputFileHeader.IsPlanar());

        int32 inputRowBytes = 0;

        if (!TryMultiplyInt32(width, numberOfChannels, inputRowBytes))
        {
            // The multiplication would have resulted in an integer overflow / underflow.
            throw std::bad_alloc();
        }

        if (!TryMultiplyInt32(inputRowBytes, 4, inputRowBytes))
        {
            // The multiplication would have resulted in an integer overflow / underflow.
            throw std::bad_alloc();
        }

        const int32 maxInputChunkHeight = GetMaxInputChunkHeight(filterRecord, inputRowBytes, height);

        int32 inputImageBufferSize;

        if (!TryMultiplyInt32(inputRowBytes, maxInputChunkHeight, inputImageBufferSize))
        {
            // The multiplication would have resulted in an integer overflow / underflow.
            throw std::bad_alloc();
        }

        unique_buffer_suite_buffer scopedBuffer(filterRecord, inputImageBufferSize);

        uint8* inputBuffer = reinterpret_cast<uint8*>(scopedBuffer.Lock());

        Imf::RgbaChannels channelsToWrite;

        switch (numberOfChannels)
        {
        case 1:
            channelsToWrite = Imf::RgbaChannels::WRITE_Y;
            break;
        case 2:
            channelsToWrite = Imf::RgbaChannels::WRITE_YA;
            break;
        case 3:
            channelsToWrite = Imf::RgbaChannels::WRITE_RGB;
            break;
        case 4:
            channelsToWrite = Imf::RgbaChannels::WRITE_RGBA;
            break;
        default:
            throw std::runtime_error("Unsupported Gmic8bfImage channel count.");
        }

        const size_t outputBufferSize = static_cast<size_t>(width) * static_cast<size_t>(maxInputChunkHeight);

        std::unique_ptr<Imf::Rgba[]> outputBuffer = std::make_unique<Imf::Rgba[]>(outputBufferSize);

        // The narrow boost::filesystem::path string is set to use UTF-8 encoding on Windows
        // when the layer index file is written.
        // OpenExr expects the file path string to use US-ASCII or UTF-8.

        Imf::RgbaOutputFile file(outputFilePath.string().c_str(), width, height, channelsToWrite);

        for (int32 y = 0; y < height; y += maxInputChunkHeight)
        {
            const int32 top = y;
            const int32 bottom = std::min(y + maxInputChunkHeight, height);

            const int32 rowCount = bottom - top;

            const size_t readBufferSize = static_cast<size_t>(inputRowBytes) * static_cast<size_t>(rowCount);

            ReadFile(inputFile, inputBuffer, readBufferSize);

            ConvertInputToOpenExrImage(
                inputBuffer,
                inputRowBytes,
                numberOfChannels,
                outputBuffer.get(),
                width,
                rowCount);

            file.setFrameBuffer(outputBuffer.get(), 1, width);
            file.writePixels(rowCount);
        }
    }
}

void ConvertGmic8bfImageToExr(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& inputFilePath,
    const boost::filesystem::path& outputFilePath)
{
    std::unique_ptr<FileHandle> inputFile = OpenFile(inputFilePath, FileOpenMode::Read);
    Gmic8bfImageHeader inputFileHeader(inputFile.get());

    WriteOpenExrFile(filterRecord, inputFile.get(), inputFileHeader, outputFilePath);
}
