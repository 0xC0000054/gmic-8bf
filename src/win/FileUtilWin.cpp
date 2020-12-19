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

#include "stdafx.h"
#include "GmicPlugin.h"
#include "FileUtil.h"
#include "FileUtilWin.h"
#include <objbase.h>
#include <ShlObj.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/win32_helpers.h>
#include <boost/filesystem.hpp>

class FileHandleWin : public FileHandle
{
public:
    FileHandleWin(const boost::filesystem::path& path, FileOpenMode mode) : hFile()
    {
        DWORD dwDesiredAccess;
        DWORD dwShareMode;
        DWORD dwCreationDisposition;
        DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

        switch (mode)
        {
        case FileOpenMode::Read:
            dwDesiredAccess = GENERIC_READ;
            dwShareMode = FILE_SHARE_READ;
            dwCreationDisposition = OPEN_EXISTING;
            break;
        case FileOpenMode::Write:
            dwDesiredAccess = GENERIC_WRITE;
            dwShareMode = 0;
            dwCreationDisposition = CREATE_ALWAYS;
            break;
        case FileOpenMode::ReadWrite:
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
            dwShareMode = 0;
            dwCreationDisposition = CREATE_ALWAYS;
            break;
        default:
            throw std::invalid_argument("Unknown FileOpenMode");
        }

        hFile.reset(CreateFileW(path.c_str(), dwDesiredAccess, dwShareMode, nullptr, dwCreationDisposition, dwFlagsAndAttributes, nullptr));

        if (!hFile)
        {
            THROW_LAST_ERROR();
        }
    }

    ~FileHandleWin() override
    {
        if (hFile)
        {
            hFile.reset();
        }
    }

    FileHandleWin(const FileHandleWin&) = delete;
    FileHandleWin& operator=(const FileHandleWin&) = delete;

    FileHandleWin(FileHandleWin&&) = delete;
    FileHandleWin& operator=(FileHandleWin&&) = delete;

    HANDLE get() const noexcept
    {
        return hFile.get();
    }

private:
    wil::unique_hfile hFile;
};

boost::filesystem::path GetPluginDataDirectoryNative()
{
    boost::filesystem::path path;

    try
    {
        wil::unique_cotaskmem_string appDataPath;

        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appDataPath)))
        {
            path = appDataPath.get();
            path /= L"Gmic8bfPlugin";
        }
    }
    catch (...)
    {
        path.clear();
    }

    return path;
}

OSErr GetPluginInstallDirectoryNative(boost::filesystem::path& path)
{
    try
    {
        wil::unique_cotaskmem_string dllPath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

        boost::filesystem::path temp(dllPath.get());

        path = temp.parent_path();
    }
    catch (...)
    {
        return ioErr;
    }

    return noErr;
}

OSErr OpenFileNative(const boost::filesystem::path& path, FileOpenMode mode, std::unique_ptr<FileHandle>& fileHandle)
{
    OSErr err = noErr;

    try
    {
        fileHandle = std::make_unique<FileHandleWin>(path, mode);
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (const std::invalid_argument&)
    {
        err = paramErr;
    }
    catch (const wil::ResultException&)
    {
        err = ioErr;
    }

    return err;
}

OSErr ReadFileNative(const FileHandle* fileHandle, void* data, size_t dataSize)
{
    if (!fileHandle)
    {
        return paramErr;
    }

    size_t bytesRemaining = dataSize;
    BYTE* buffer = static_cast<BYTE*>(data);

    HANDLE hFile = static_cast<const FileHandleWin*>(fileHandle)->get();

    while (bytesRemaining > 0)
    {
        DWORD numBytesToRead = 0x80000000UL;

        if (bytesRemaining < numBytesToRead)
        {
            numBytesToRead = static_cast<DWORD>(bytesRemaining);
        }

        DWORD bytesRead = 0;

        if (!ReadFile(hFile, buffer, numBytesToRead, &bytesRead, nullptr))
        {
            return ioErr;
        }

        if (bytesRead != numBytesToRead)
        {
            return bytesRead == 0 ? eofErr : ioErr;
        }

        buffer += bytesRead;
        bytesRemaining -= bytesRead;
    }

    return noErr;
}

OSErr SetFilePositionNative(const FileHandle* fileHandle, int16 posMode, int64 posOffset)
{
    if (!fileHandle)
    {
        return paramErr;
    }

    LARGE_INTEGER distanceToMove = {};

    distanceToMove.QuadPart = posOffset;

    if (!SetFilePointerEx(
        static_cast<const FileHandleWin*>(fileHandle)->get(),
        distanceToMove,
        nullptr,
        static_cast<DWORD>(posMode)))
    {
        return ioErr;
    }

    return noErr;
}

OSErr WriteFileNative(const FileHandle* fileHandle, const void* data, size_t dataSize)
{
    if (!fileHandle)
    {
        return paramErr;
    }

    size_t bytesRemaining = dataSize;
    const BYTE* buffer = static_cast<const BYTE*>(data);

    HANDLE hFile = static_cast<const FileHandleWin*>(fileHandle)->get();

    while (bytesRemaining > 0)
    {
        DWORD numBytesToWrite = 0x80000000UL;

        if (bytesRemaining < numBytesToWrite)
        {
            numBytesToWrite = static_cast<DWORD>(bytesRemaining);
        }

        DWORD bytesWritten = 0;

        if (!WriteFile(hFile, buffer, numBytesToWrite, &bytesWritten, nullptr))
        {
            return ioErr;
        }

        if (bytesWritten != numBytesToWrite)
        {
            return ioErr;
        }

        buffer += bytesWritten;
        bytesRemaining -= bytesWritten;
    }

    return noErr;
}
