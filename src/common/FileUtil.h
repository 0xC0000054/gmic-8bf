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

#pragma once

#ifndef FILEUTIL_H
#define FILEUTIL_H

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

boost::filesystem::path GetGmicQtPath();

boost::filesystem::path GetInputDirectory();

boost::filesystem::path GetOutputDirectory();

boost::filesystem::path GetIOSettingsPath();

boost::filesystem::path GetTemporaryFileName(const boost::filesystem::path& dir, const char* const fileExtension);

std::unique_ptr<FileHandle> OpenFile(const boost::filesystem::path& path, FileOpenMode mode);

void ReadFile(FileHandle* fileHandle, void* data, size_t dataSize);

void SetFileLength(FileHandle* fileHandle, int64 length);

void SetFilePosition(FileHandle* fileHandle, int16 posMode, int64 posOffset);

void WriteFile(FileHandle* fileHandle, const void* data, size_t dataSize);

#endif // !FILEUTIL_H
