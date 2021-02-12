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

#include "InputLayerIndex.h"
#include "InputLayerInfo.h"
#include "ClipboardUtil.h"
#include "ImageConversion.h"
#include "FileUtil.h"
#include <codecvt>
#include <locale>
#include <boost/endian.hpp>

namespace
{
    struct IndexFileHeader
    {
        IndexFileHeader(
            int32 numberOfLayers,
            int32 activeLayer,
            bool isGrayScale,
            bool isSixteenBitsPerChannel)
        {
            // G8IX = GMIC 8BF index
            signature[0] = 'G';
            signature[1] = '8';
            signature[2] = 'I';
            signature[3] = 'X';
            version = 3;
            layerCount = numberOfLayers;
            activeLayerIndex = activeLayer;
            documentFlags = 0;

            if (isGrayScale)
            {
                documentFlags |= (1 << 0);
            }

            if (isSixteenBitsPerChannel)
            {
                documentFlags |= (1 << 1);
            }
        }

        char signature[4];
        boost::endian::little_int32_t version;
        boost::endian::little_int32_t layerCount;
        boost::endian::little_int32_t activeLayerIndex;
        boost::endian::little_int32_t documentFlags;
    };

    void WriteAlternateInputImagePath(const FileHandle* fileHandle, const boost::filesystem::path& path)
    {
        // The file path will be converted from UTF-16 to UTF-8.
        const std::string& value = path.string();

        if (value.size() > static_cast<size_t>(std::numeric_limits<int32>::max()))
        {
            throw std::runtime_error("The string length exceeds 2GB.");
        }

        boost::endian::little_int32_t stringLength = static_cast<int32>(value.size());

        WriteFile(fileHandle, &stringLength, sizeof(stringLength));

        WriteFile(fileHandle, value.data(), value.size());
    }

    void WriteAlternateInputImageData(
        const FileHandle* fileHandle,
        const FilterRecordPtr filterRecord,
        const GmicIOSettings& settings)
    {
        const SecondInputImageSource source = settings.GetSecondInputImageSource();

        if (source == SecondInputImageSource::None)
        {
            // Write an empty path if document layers are the only input source.
            // This avoids some overhead from creating a file path and trying to open a nonexistent file.
            WriteAlternateInputImagePath(fileHandle, boost::filesystem::path());
        }
        else
        {
            boost::filesystem::path inputDir = GetInputDirectory();

            boost::filesystem::path secondGmicInputImage = GetTemporaryFileName(inputDir, ".g8i");

            if (source == SecondInputImageSource::Clipboard)
            {
                OSErrException::ThrowIfError(ConvertClipboardImageToGmicInput(filterRecord, secondGmicInputImage));
            }
            else if (source == SecondInputImageSource::File)
            {
                OSErrException::ThrowIfError(ConvertImageToGmicInputFormat(
                    filterRecord,
                    settings.GetSecondInputImagePath(),
                    secondGmicInputImage));
            }

            WriteAlternateInputImagePath(fileHandle, secondGmicInputImage);
        }
    }
}

InputLayerIndex::InputLayerIndex(int16 imageMode)
    : inputFiles(), activeLayerIndex(0),
    grayScale(imageMode == plugInModeGrayScale || imageMode == plugInModeGray16),
    sixteenBitsPerChannel(imageMode == plugInModeGray16 || imageMode == plugInModeRGB48)
{
}

InputLayerIndex::~InputLayerIndex()
{
}

void InputLayerIndex::AddFile(
    const boost::filesystem::path& path,
    int32 width,
    int32 height,
    bool visible,
    std::string utf8Name)
{
    InputLayerInfo info(path, width, height, visible, utf8Name);

    inputFiles.push_back(info);
}

void InputLayerIndex::SetActiveLayerIndex(int32 index)
{
    activeLayerIndex = index;
}

void InputLayerIndex::Write(
    const boost::filesystem::path& path,
    const FilterRecordPtr filterRecord,
    const GmicIOSettings& settings)
{
    if (inputFiles.size() > static_cast<size_t>(std::numeric_limits<int32>().max()))
    {
        throw std::runtime_error("The number of input files exceeds 2,147,483,647.");
    }

    // Convert the narrow path string to UTF-8 on Windows.
#if __PIWin__
    boost::filesystem::path::imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>()));
#endif

    std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Write);

    IndexFileHeader header(
        static_cast<int32>(inputFiles.size()),
        activeLayerIndex,
        grayScale,
        sixteenBitsPerChannel);

    WriteFile(file.get(), &header, sizeof(header));

    for (size_t i = 0; i < inputFiles.size(); i++)
    {
        inputFiles[i].Write(file.get());
    }

    if (inputFiles.size() == 1)
    {
        WriteAlternateInputImageData(file.get(), filterRecord, settings);
    }
}
