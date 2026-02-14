////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2026 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#ifndef FILEIO_H
#define FILEIO_H

#include <PITypes.h>
#include <boost/filesystem.hpp>
#include <new>

enum class FileOpenMode
{
    Read = 0,
    Write = 1
};

class FileHandle
{
public:

    virtual ~FileHandle() noexcept(false)
    {
    }

protected:

    FileHandle()
    {
    }

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    FileHandle(FileHandle&&) = delete;
    FileHandle operator=(FileHandle&&) = delete;
};

::std::unique_ptr<FileHandle> OpenFile(const boost::filesystem::path& path, FileOpenMode mode, int64 preallocationSize = 0);

void ReadFile(FileHandle* fileHandle, void* data, size_t dataSize);

int64 GetFilePosition(FileHandle* fileHandle);

void SetFilePosition(FileHandle* fileHandle, int64 posOffset);

void WriteFile(FileHandle* fileHandle, const void* data, size_t dataSize);

#endif // !FILEIO_H
