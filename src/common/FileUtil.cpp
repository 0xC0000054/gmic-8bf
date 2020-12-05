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

        TempDirectory(boost::filesystem::path sessionDirectoriesRoot) : path(), isValid(false)
        {
            if (sessionDirectoriesRoot.empty())
            {
                throw std::runtime_error("The session directories root path is empty.");
            }

            path = sessionDirectoriesRoot;
            path /= boost::filesystem::unique_path();

            boost::filesystem::create_directories(path);

            isValid = true;
        }

        TempDirectory(const TempDirectory&) = delete;
        TempDirectory& operator=(const TempDirectory&) = delete;

        TempDirectory(TempDirectory&&) = delete;
        TempDirectory operator=(TempDirectory&&) = delete;

        ~TempDirectory()
        {
            boost::system::error_code ec;
            boost::filesystem::remove_all(path, ec);
            isValid = false;
        }

        boost::filesystem::path Get() const
        {
            return path;
        }

        bool IsValid() const noexcept
        {
            return isValid;
        }

    private:

        boost::filesystem::path path;
        bool isValid;
    };

    OSErr GetSessionRootDirectory(boost::filesystem::path& path)
    {

        OSErr err = noErr;

        try
        {
            static std::unique_ptr<TempDirectory> sessionDir = std::make_unique<TempDirectory>(GetSessionDirectoriesRootNative());

            if (sessionDir->IsValid())
            {
                path = sessionDir->Get();
            }
            else
            {
                err = ioErr;
            }
        }
        catch (const std::bad_alloc&)
        {
            err = memFullErr;
        }
        catch (const std::length_error&)
        {
            err = memFullErr;
        }
        catch (...)
        {
            err = ioErr;
        }

        return err;
    }
}

OSErr GetGmicQtPath(boost::filesystem::path& path)
{
    OSErr err = noErr;

    try
    {
        boost::filesystem::path pluginInstallDir;

        err = GetPluginInstallDirectoryNative(pluginInstallDir);

        if (err == noErr)
        {
            path = pluginInstallDir;
            path /= "gmic";
            path /= ExecutableName;
        }
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (...)
    {
        err = ioErr;
    }

    return err;
}

OSErr GetInputDirectory(boost::filesystem::path& path)
{
    OSErr err = noErr;

    try
    {
        err = GetSessionRootDirectory(path);
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (...)
    {
        err = ioErr;
    }

    return err;
}

OSErr GetOutputDirectory(boost::filesystem::path& path)
{
    OSErr err = noErr;

    try
    {
        err = GetSessionRootDirectory(path);
        if (err == noErr)
        {
            path /= "out";

            boost::filesystem::create_directory(path);
        }
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (...)
    {
        err = ioErr;
    }

    return err;
}

OSErr GetTemporaryFileName(const boost::filesystem::path& dir, boost::filesystem::path& path, const char* const fileExtension)
{
    OSErr err = noErr;

    try
    {
        path = dir;
        path /= boost::filesystem::unique_path();

        if (fileExtension)
        {
            path += fileExtension;
        }
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (...)
    {
        err = ioErr;
    }

    return err;
}

OSErr OpenFile(const boost::filesystem::path& path, FileOpenMode mode, std::unique_ptr<FileHandle>& fileHandle)
{
    return OpenFileNative(path, mode, fileHandle);
}

OSErr ReadFile(const FileHandle* fileHandle, void* data, size_t dataSize)
{
    return ReadFileNative(fileHandle, data, dataSize);
}

OSErr SetFilePosition(const FileHandle* fileHandle, int16 posMode, int64 posOffset)
{
    return SetFilePositionNative(fileHandle, posMode, posOffset);
}

OSErr WriteFile(const FileHandle* fileHandle, const void* data, size_t dataSize)
{
    return WriteFileNative(fileHandle, data, dataSize);
}
