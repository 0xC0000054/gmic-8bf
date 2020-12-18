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

#include "GmicIOSettings.h"
#include "FileUtil.h"
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

    OSErr ReadFilePath(const FileHandle* fileHandle, boost::filesystem::path& value)
    {
        OSErr err = noErr;

        try
        {
            boost::endian::little_uint32_t stringLength = 0;

            err = ReadFile(fileHandle, &stringLength, sizeof(stringLength));

            if (err == noErr)
            {
                constexpr size_t pathCharSize = sizeof(boost::filesystem::path::value_type);

                // Check that the required byte buffer size can fit in a size_t.
                if (stringLength > (std::numeric_limits<size_t>::max() / pathCharSize))
                {
                    return ioErr;
                }

                std::vector<boost::filesystem::path::value_type> stringChars(stringLength);

                const size_t stringLengthInBytes = stringLength * pathCharSize;

                err = ReadFile(fileHandle, &stringChars[0], stringLengthInBytes);

                if (err == noErr)
                {
                    value.assign(stringChars.begin(), stringChars.end());
                }
            }
        }
        catch (const std::bad_alloc&)
        {
            err = memFullErr;
        }
        catch (...)
        {
            err = readErr;
        }

        return err;
    }

    OSErr WriteFilePath(const FileHandle* fileHandle, const boost::filesystem::path& value)
    {
        OSErr err = noErr;

        try
        {
            constexpr size_t pathCharSize = sizeof(boost::filesystem::path::value_type);

            // Check that the required byte buffer size can fit in a size_t.
            if (value.size() > (std::numeric_limits<size_t>::max() / pathCharSize))
            {
                return ioErr;
            }

            boost::endian::little_uint32_t stringLength = static_cast<uint32_t>(value.size());

            err = WriteFile(fileHandle, &stringLength, sizeof(stringLength));

            if (err == noErr)
            {
                const size_t stringLengthInBytes = value.size() * pathCharSize;

                err = WriteFile(fileHandle, value.c_str(), stringLengthInBytes);
            }
        }
        catch (const std::bad_alloc&)
        {
            err = memFullErr;
        }
        catch (...)
        {
            err = writErr;
        }

        return err;
    }
}

GmicIOSettings::GmicIOSettings() : defaultOutputPath()
{
}

GmicIOSettings::~GmicIOSettings()
{
}

boost::filesystem::path GmicIOSettings::GetDefaultOutputPath() const
{
    return defaultOutputPath;
}

void GmicIOSettings::SetDefaultOutputPath(const boost::filesystem::path& path)
{
    defaultOutputPath = path;
}

OSErr GmicIOSettings::Load(const boost::filesystem::path& path)
{
    OSErr err = noErr;

    if (boost::filesystem::exists(path))
    {
        std::unique_ptr<FileHandle> file;

        err = OpenFile(path, FileOpenMode::Read, file);

        if (err == noErr)
        {
            IOSettingsFileHeader header{};

            err = ReadFile(file.get(), &header, sizeof(header));

            if (err == noErr)
            {
                if (strncmp(header.signature, "G8IS", 4) != 0)
                {
                    // The setting file has an incorrect signature.
                    return readErr;
                }

                if (header.version != 1)
                {
                    // The setting file has an unknown version.
                    return readErr;
                }

                err = ReadFilePath(file.get(), defaultOutputPath);
            }
        }
    }

    return err;
}

OSErr GmicIOSettings::Save(const boost::filesystem::path& path)
{
    std::unique_ptr<FileHandle> file;

    OSErr err = noErr;

    try
    {
        err = OpenFile(path, FileOpenMode::Write, file);

        if (err == noErr)
        {
            IOSettingsFileHeader header;

            err = WriteFile(file.get(), &header, sizeof(header));

            if (err == noErr)
            {
                err = WriteFilePath(file.get(), defaultOutputPath);
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (...)
    {
        err = writErr;
    }

    return err;
}
