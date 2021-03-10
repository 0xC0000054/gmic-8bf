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

#include "GmicPlugin.h"
#include "FileUtil.h"

#ifdef __PIWin__
#include "FileUtilWin.h"
#else
#error "Missing a FileUtil header for this platform."
#endif

namespace
{
    class TempDirectory
    {
    public:

        TempDirectory(boost::filesystem::path sessionDirectoriesRoot) : path()
        {
            if (sessionDirectoriesRoot.empty())
            {
                throw std::runtime_error("The session directories root path is empty.");
            }

            path = sessionDirectoriesRoot;
            path /= boost::filesystem::unique_path();

            boost::filesystem::create_directories(path);
        }

        TempDirectory(const TempDirectory&) = delete;
        TempDirectory& operator=(const TempDirectory&) = delete;

        TempDirectory(TempDirectory&&) = delete;
        TempDirectory operator=(TempDirectory&&) = delete;

        ~TempDirectory()
        {
            boost::system::error_code ec;
            boost::filesystem::remove_all(path, ec);
        }

        boost::filesystem::path Get() const
        {
            return path;
        }

    private:

        boost::filesystem::path path;
    };

    boost::filesystem::path GetSessionDirectory()
    {
        static std::unique_ptr<TempDirectory> sessionDir = std::make_unique<TempDirectory>(GetSessionDirectoriesRootNative());

        return sessionDir->Get();
    }

    boost::filesystem::path GetSettingsDirectory()
    {
        boost::filesystem::path path = GetPluginSettingsDirectoryNative();

        boost::filesystem::create_directories(path);

        return path;
    }
}

boost::filesystem::path GetGmicQtPath()
{
    static boost::filesystem::path path = GetGmicQtPathNative();

    return path;
}

boost::filesystem::path GetInputDirectory()
{
    return GetSessionDirectory();
}

boost::filesystem::path GetOutputDirectory()
{
    boost::filesystem::path path = GetSessionDirectory();
    path /= "out";

    boost::filesystem::create_directory(path);

    return path;
}

boost::filesystem::path GetIOSettingsPath()
{
    boost::filesystem::path path = GetSettingsDirectory();

    path /= "IOSettings.dat";

    return path;
}

boost::filesystem::path GetTemporaryFileName(const boost::filesystem::path& dir, const char* const fileExtension)
{
    boost::filesystem::path path;

    path = dir;
    path /= boost::filesystem::unique_path("%%%%%%%%");

    if (fileExtension)
    {
        path += fileExtension;
    }

    return path;
}

std::unique_ptr<FileHandle> OpenFile(const boost::filesystem::path& path, FileOpenMode mode)
{
    return OpenFileNative(path, mode);
}

void ReadFile(const FileHandle* fileHandle, void* data, size_t dataSize)
{
    ReadFileNative(fileHandle, data, dataSize);
}

void SetFileLength(const FileHandle* fileHandle, int64 length)
{
    SetFileLengthNative(fileHandle, length);
}

void SetFilePosition(const FileHandle* fileHandle, int16 posMode, int64 posOffset)
{
    SetFilePositionNative(fileHandle, posMode, posOffset);
}

void WriteFile(const FileHandle* fileHandle, const void* data, size_t dataSize)
{
    WriteFileNative(fileHandle, data, dataSize);
}
