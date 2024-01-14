////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022, 2023, 2024 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "InputLayerIndex.h"
#include "InputLayerInfo.h"
#include "FileUtil.h"
#include "StringIO.h"
#include <codecvt>
#include <locale>
#include <boost/predef.h>

namespace
{
    struct alignas(4) IndexFileHeader
    {
        IndexFileHeader(
            int32 numberOfLayers,
            int32 activeLayer,
            uint8 imageBitDepth,
            bool isGrayScale,
            bool writeIccProfiles)
        {
            // G8LI = GMIC 8BF layer index
            signature[0] = 'G';
            signature[1] = '8';
            signature[2] = 'L';
            signature[3] = 'I';
#if BOOST_ENDIAN_BIG_BYTE
            endian[0] = 'B';
            endian[1] = 'E';
            endian[2] = 'D';
            endian[3] = 'N';
#elif BOOST_ENDIAN_LITTLE_BYTE
            endian[0] = 'L';
            endian[1] = 'E';
            endian[2] = 'D';
            endian[3] = 'N';
#else
#error "Unknown endianness on this platform."
#endif
            version = 3;
            layerCount = numberOfLayers;
            activeLayerIndex = activeLayer;
            bitsPerChannel = imageBitDepth;
            grayScale = isGrayScale;
            haveIccProfiles = writeIccProfiles;
            padding = 0;
        }

        char signature[4];
        char endian[4]; // This field is 4 bytes to maintain structure alignment.
        int32_t version;
        int32_t layerCount;
        int32_t activeLayerIndex;
        uint8_t bitsPerChannel;
        uint8_t grayScale;
        uint8_t haveIccProfiles;
        uint8_t padding; // Required due to the 4-byte structure alignment.
    };
}

InputLayerIndex::InputLayerIndex(uint8 imageBitDepth, bool grayScale)
    : inputFiles(), activeLayerIndex(0), imageBitDepth(imageBitDepth), grayScale(grayScale)
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
    ::std::string utf8Name)
{
    InputLayerInfo info(path, width, height, visible, utf8Name);

    inputFiles.push_back(info);
}

void InputLayerIndex::AddFile(const InputLayerInfo& info)
{
    inputFiles.push_back(info.Clone());
}

int32 InputLayerIndex::GetLayerCount() const
{
    if (inputFiles.size() > static_cast<size_t>(::std::numeric_limits<int32>().max()))
    {
        throw ::std::runtime_error("The number of input files exceeds 2,147,483,647.");
    }

    return static_cast<int32>(inputFiles.size());
}

void InputLayerIndex::SetActiveLayerIndex(int32 index)
{
    activeLayerIndex = index;
}

void InputLayerIndex::SetColorProfiles(
    const boost::filesystem::path& imageProfile,
    const boost::filesystem::path& displayProfile)
{
    this->imageProfilePath = imageProfile;
    this->displayProfilePath = displayProfile;
}

void InputLayerIndex::Write(const boost::filesystem::path& path)
{
    if (inputFiles.size() > static_cast<size_t>(::std::numeric_limits<int32>().max()))
    {
        throw ::std::runtime_error("The number of input files exceeds 2,147,483,647.");
    }

    // Convert the narrow path string to UTF-8 on Windows.
#if __PIWin__
    boost::filesystem::path::imbue(::std::locale(::std::locale(), new ::std::codecvt_utf8_utf16<wchar_t>()));
#endif

    const bool writeIccProfiles = !imageProfilePath.empty() && !displayProfilePath.empty();

    ::std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Write);

    IndexFileHeader header(
        static_cast<int32>(inputFiles.size()),
        activeLayerIndex,
        imageBitDepth,
        grayScale,
        writeIccProfiles);

    WriteFile(file.get(), &header, sizeof(header));

    if (writeIccProfiles)
    {
        WriteUtf8String(file.get(), imageProfilePath.string());
        WriteUtf8String(file.get(), displayProfilePath.string());
    }

    for (size_t i = 0; i < inputFiles.size(); i++)
    {
        inputFiles[i].Write(file.get());
    }
}
