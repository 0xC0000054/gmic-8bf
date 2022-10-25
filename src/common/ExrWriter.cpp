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

#include "ExrWriter.h"
#include "FileIO.h"
#include "Gmic8bfImageHeader.h"
#include "ScopedBufferSuite.h"
#include "Utilities.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 26495 26812)
#endif // _MSC_VER

#include "OpenEXR/ImfChannelList.h"
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include "OpenEXR/ImfOutputFile.h"

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

namespace
{
    class OpenExrOutputStream : public OStream
    {
    public:

        // The narrow boost::filesystem::path string is set to use UTF-8 encoding on Windows
        // when the layer index file is written.
        // OpenExr expects the file path string to use US-ASCII or UTF-8.

        OpenExrOutputStream(const boost::filesystem::path& path)
            : file(OpenFile(path, FileOpenMode::Write)), OStream(path.string().c_str())
        {
        }

        void write(const char c[], int n) override
        {
            WriteFile(file.get(), c, n);
        }

        uint64_t tellp() override
        {
            return static_cast<uint64_t>(GetFilePosition(file.get()));
        }

        void seekp(uint64_t pos) override
        {
            if (pos > static_cast<uint64_t>(std::numeric_limits<int64>::max()))
            {
                throw Iex::IoExc("Cannot seek beyond the positive range of a signed 64-bit integer.");
            }

            SetFilePosition(file.get(), static_cast<int64>(pos));
        }

    private:

        std::unique_ptr<FileHandle> file;
    };

    void WriteOpenExrFile(
        const FilterRecordPtr filterRecord,
        FileHandle* inputFile,
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

        ScopedBufferSuiteBuffer scopedBuffer(filterRecord, inputRowBytes);

        Imf::Header header(width, height);

        switch (numberOfChannels)
        {
        case 1:
            header.channels().insert("Y", Channel(PixelType::FLOAT, 1, 1));
            break;
        case 2:
            header.channels().insert("Y", Channel(PixelType::FLOAT, 1, 1));
            header.channels().insert("A", Channel(PixelType::FLOAT, 1, 1));
           break;
        case 3:
            header.channels().insert("R", Channel(PixelType::FLOAT, 1, 1));
            header.channels().insert("G", Channel(PixelType::FLOAT, 1, 1));
            header.channels().insert("B", Channel(PixelType::FLOAT, 1, 1));
            break;
        case 4:
            header.channels().insert("R", Channel(PixelType::FLOAT, 1, 1));
            header.channels().insert("G", Channel(PixelType::FLOAT, 1, 1));
            header.channels().insert("B", Channel(PixelType::FLOAT, 1, 1));
            header.channels().insert("A", Channel(PixelType::FLOAT, 1, 1));
            break;
        default:
            throw std::runtime_error("Unsupported Gmic8bfImage channel count.");
        }

        OpenExrOutputStream outputStream(outputFilePath);

        Imf::OutputFile file(outputStream, header);

        Imf::FrameBuffer frameBuffer;

        char* frameBufferScan0 = static_cast<char*>(scopedBuffer.Lock());

        switch (numberOfChannels)
        {
        case 1:
            frameBuffer.insert("Y", Slice(PixelType::FLOAT, frameBufferScan0, sizeof(float)));
            break;
        case 2:
            frameBuffer.insert("Y", Slice(PixelType::FLOAT, frameBufferScan0 + sizeof(float) * 0, sizeof(float) * 2));
            frameBuffer.insert("A", Slice(PixelType::FLOAT, frameBufferScan0 + sizeof(float) * 1, sizeof(float) * 2));
            break;
        case 3:
            frameBuffer.insert("R", Slice(PixelType::FLOAT, frameBufferScan0 + sizeof(float) * 0, sizeof(float) * 3));
            frameBuffer.insert("G", Slice(PixelType::FLOAT, frameBufferScan0 + sizeof(float) * 1, sizeof(float) * 3));
            frameBuffer.insert("B", Slice(PixelType::FLOAT, frameBufferScan0 + sizeof(float) * 2, sizeof(float) * 3));
            break;
        case 4:
            frameBuffer.insert("R", Slice(PixelType::FLOAT, frameBufferScan0 + sizeof(float) * 0, sizeof(float) * 4));
            frameBuffer.insert("G", Slice(PixelType::FLOAT, frameBufferScan0 + sizeof(float) * 1, sizeof(float) * 4));
            frameBuffer.insert("B", Slice(PixelType::FLOAT, frameBufferScan0 + sizeof(float) * 2, sizeof(float) * 4));
            frameBuffer.insert("A", Slice(PixelType::FLOAT, frameBufferScan0 + sizeof(float) * 3, sizeof(float) * 4));
            break;
        default:
            throw std::runtime_error("Unsupported Gmic8bfImage channel count.");
        }

        file.setFrameBuffer(frameBuffer);

        for (int32 y = 0; y < height; y++)
        {
            // Because the Gmic8bfImage is using the same interleaved float32 format as the OpeEXR frame buffer, we can write the
            // image data directly to the output EXR file.
            ReadFile(inputFile, frameBufferScan0, inputRowBytes);

            file.writePixels();
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
