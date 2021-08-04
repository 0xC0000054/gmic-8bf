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

#include "stdafx.h"
#include "GmicPlugin.h"
#include "FileUtilWin.h"
#include <objbase.h>
#include <ShlObj.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/win32_helpers.h>
#include <boost/filesystem.hpp>

namespace
{
    boost::filesystem::path GetPluginInstallDirectory()
    {
        boost::filesystem::path path;

        try
        {
            wil::unique_cotaskmem_string dllPath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

            boost::filesystem::path temp(dllPath.get());

            path = temp.parent_path();
        }
        catch (const wil::ResultException& e)
        {
            if (e.GetErrorCode() == E_OUTOFMEMORY)
            {
                throw std::bad_alloc();
            }
            else
            {
                throw std::runtime_error(e.what());
            }
        }

        return path;
    }
}

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

boost::filesystem::path GetGmicQtPathNative()
{
    boost::filesystem::path path = GetPluginInstallDirectory();

    path /= "gmic";
    path /= "gmic_8bf_qt.exe";

    return path;
}

boost::filesystem::path GetPluginSettingsDirectoryNative()
{
    boost::filesystem::path path;

    try
    {
        wil::unique_cotaskmem_string appDataPath;

        THROW_IF_FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appDataPath));

        path = appDataPath.get();
        path /= L"Gmic8bfPlugin";
        path /= L"settings";
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error(e.what());
        }
    }

    return path;
}

boost::filesystem::path GetSessionDirectoriesRootNative()
{
    boost::filesystem::path path;

    try
    {
        wil::unique_cotaskmem_string programDataPath;

        THROW_IF_FAILED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &programDataPath));

        path = programDataPath.get();
        path /= L"Gmic8bfPlugin";
        path /= L"SessionData";
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error(e.what());
        }
    }

    return path;
}

std::unique_ptr<FileHandle> OpenFileNative(const boost::filesystem::path& path, FileOpenMode mode)
{
    try
    {
        return std::make_unique<FileHandleWin>(path, mode);
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error(e.what());
        }
    }
}

void ReadFileNative(FileHandle* fileHandle, void* data, size_t dataSize)
{
    if (!fileHandle)
    {
        throw std::runtime_error("Null file handle");
    }

    try
    {
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

            THROW_IF_WIN32_BOOL_FALSE(ReadFile(hFile, buffer, numBytesToRead, &bytesRead, nullptr));

            if (bytesRead == 0)
            {
                throw std::runtime_error("Attempted to read beyond the end of the file.");
            }

            buffer += bytesRead;
            bytesRemaining -= bytesRead;
        }
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error(e.what());
        }
    }
}

void SetFileLengthNative(FileHandle* fileHandle, int64 length)
{
    try
    {
        FILE_END_OF_FILE_INFO endOfFileInfo;
        endOfFileInfo.EndOfFile.QuadPart = length;

        THROW_IF_WIN32_BOOL_FALSE(SetFileInformationByHandle(
            static_cast<const FileHandleWin*>(fileHandle)->get(),
            FileEndOfFileInfo,
            &endOfFileInfo,
            sizeof(endOfFileInfo)));
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error(e.what());
        }
    }
}

void SetFilePositionNative(FileHandle* fileHandle, int16 posMode, int64 posOffset)
{
    if (!fileHandle)
    {
        throw std::runtime_error("Null file handle");
    }

    try
    {
        LARGE_INTEGER distanceToMove = {};

        distanceToMove.QuadPart = posOffset;

        THROW_IF_WIN32_BOOL_FALSE(SetFilePointerEx(
            static_cast<const FileHandleWin*>(fileHandle)->get(),
            distanceToMove,
            nullptr,
            static_cast<DWORD>(posMode)));
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error(e.what());
        }
    }
}

void WriteFileNative(FileHandle* fileHandle, const void* data, size_t dataSize)
{
    if (!fileHandle)
    {
        throw std::runtime_error("Null file handle");
    }

    size_t bytesRemaining = dataSize;
    const BYTE* buffer = static_cast<const BYTE*>(data);

    HANDLE hFile = static_cast<const FileHandleWin*>(fileHandle)->get();

    try
    {
        while (bytesRemaining > 0)
        {
            DWORD numBytesToWrite = 0x80000000UL;

            if (bytesRemaining < numBytesToWrite)
            {
                numBytesToWrite = static_cast<DWORD>(bytesRemaining);
            }

            DWORD bytesWritten = 0;

            THROW_IF_WIN32_BOOL_FALSE(WriteFile(hFile, buffer, numBytesToWrite, &bytesWritten, nullptr));

            buffer += bytesWritten;
            bytesRemaining -= bytesWritten;
        }
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error(e.what());
        }
    }
}
