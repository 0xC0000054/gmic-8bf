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

#include "GmicIOSettings.h"
#include "FileIO.h"
#include "boost/endian.hpp"
#include <vector>

namespace
{
    struct IOSettingsFileHeader
    {
        IOSettingsFileHeader() : version(1), reserved()
        {
            // G8IS = GMIC 8BF I/O settings
            signature[0] = 'G';
            signature[1] = '8';
            signature[2] = 'I';
            signature[3] = 'S';
        }

        char signature[4];
        boost::endian::little_int32_t version;
        char reserved[8];
    };

    void ReadFilePath(FileHandle* fileHandle, boost::filesystem::path& value)
    {
        boost::endian::little_uint32_t stringLength = 0;

        ReadFile(fileHandle, &stringLength, sizeof(stringLength));

        if (stringLength == 0)
        {
            value = boost::filesystem::path();
        }
        else
        {
            constexpr size_t pathCharSize = sizeof(boost::filesystem::path::value_type);

            // Check that the required byte buffer size can fit in a size_t.
            if (stringLength > (std::numeric_limits<size_t>::max() / pathCharSize))
            {
                throw std::runtime_error("The string cannot be written to the file because it is too long.");
            }

            std::vector<boost::filesystem::path::value_type> stringChars(stringLength);

            const size_t stringLengthInBytes = stringLength * pathCharSize;

            ReadFile(fileHandle, &stringChars[0], stringLengthInBytes);

            value.assign(stringChars.begin(), stringChars.end());
        }
    }

    void WriteFilePath(FileHandle* fileHandle, const boost::filesystem::path& value)
    {
        constexpr size_t pathCharSize = sizeof(boost::filesystem::path::value_type);

        // Check that the required byte buffer size can fit in a size_t.
        if (value.size() > (std::numeric_limits<size_t>::max() / pathCharSize))
        {
            throw std::runtime_error("The string cannot be written to the file because it is too long.");
        }

        boost::endian::little_uint32_t stringLength = static_cast<uint32_t>(value.size());

        WriteFile(fileHandle, &stringLength, sizeof(stringLength));

        if (value.size() > 0)
        {
            const size_t stringLengthInBytes = value.size() * pathCharSize;

            WriteFile(fileHandle, value.c_str(), stringLengthInBytes);
        }

    }

    void ReadSecondInputImageSourceValue(FileHandle* fileHandle, SecondInputImageSource& value)
    {
        value = SecondInputImageSource::None;

        boost::endian::little_uint32_t source{};

        ReadFile(fileHandle, &source, sizeof(source));

        constexpr uint32_t firstValue = static_cast<uint32_t>(SecondInputImageSource::None);
        constexpr uint32_t lastValue = static_cast<uint32_t>(SecondInputImageSource::File);

        if (source >= firstValue && source <= lastValue)
        {
            value = static_cast<SecondInputImageSource>(static_cast<uint32_t>(source));
        }
    }

    void WriteSecondInputImageSourceValue(FileHandle* fileHandle, SecondInputImageSource value)
    {
        uint32_t integerValue = static_cast<uint32_t>(value);
        boost::endian::little_uint32_t source = static_cast<boost::endian::little_uint32_t>(integerValue);

        WriteFile(fileHandle, &source, sizeof(source));
    }
}

GmicIOSettings::GmicIOSettings()
    : defaultOutputPath(), secondInputImageSource(SecondInputImageSource::None), secondInputImagePath()
{
}

GmicIOSettings::~GmicIOSettings()
{
}

boost::filesystem::path GmicIOSettings::GetDefaultOutputPath() const
{
    return defaultOutputPath;
}

SecondInputImageSource GmicIOSettings::GetSecondInputImageSource() const
{
    return secondInputImageSource;
}

boost::filesystem::path GmicIOSettings::GetSecondInputImagePath() const
{
    return secondInputImagePath;
}

void GmicIOSettings::SetDefaultOutputPath(const boost::filesystem::path& path)
{
    defaultOutputPath = path;
}

void GmicIOSettings::SetSecondInputImageSource(SecondInputImageSource source)
{
    secondInputImageSource = source;
}

void GmicIOSettings::SetSecondInputImagePath(const boost::filesystem::path& path)
{
    secondInputImagePath = path;
}

void GmicIOSettings::Load(const boost::filesystem::path& path)
{
    if (boost::filesystem::exists(path))
    {
        std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Read);

        IOSettingsFileHeader header{};

        ReadFile(file.get(), &header, sizeof(header));
        if (strncmp(header.signature, "G8IS", 4) != 0)
        {
            throw std::runtime_error("The setting file has an incorrect signature.");
        }

        if (header.version != 1)
        {
            throw std::runtime_error("The setting file has an unknown version.");
        }

        ReadFilePath(file.get(), defaultOutputPath);

        ReadSecondInputImageSourceValue(file.get(), secondInputImageSource);

        ReadFilePath(file.get(), secondInputImagePath);
    }
}

void GmicIOSettings::Save(const boost::filesystem::path& path)
{
    std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Write);

    IOSettingsFileHeader header;

    WriteFile(file.get(), &header, sizeof(header));

    WriteFilePath(file.get(), defaultOutputPath);
    WriteSecondInputImageSourceValue(file.get(), secondInputImageSource);
    WriteFilePath(file.get(), secondInputImagePath);
}
