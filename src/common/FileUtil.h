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

#pragma once

#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <boost/filesystem.hpp>
#include <new>

enum class FileOpenMode
{
    Read = 0,
    Write = 1,
    ReadWrite = 2
};

class FileHandle
{
public:

    virtual ~FileHandle()
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

OSErr GetGmicQtPath(boost::filesystem::path& path);

OSErr GetInputDirectory(boost::filesystem::path& path);

OSErr GetOutputDirectory(boost::filesystem::path& path);

OSErr GetTemporaryFileName(const boost::filesystem::path& dir, boost::filesystem::path& path, const char* const fileExtension);

OSErr OpenFile(const boost::filesystem::path& path, FileOpenMode mode, std::unique_ptr<FileHandle>& fileHandle);

OSErr ReadFile(const FileHandle* fileHandle, void* data, size_t dataSize);

OSErr SetFilePosition(const FileHandle* fileHandle, int16 posMode, int64 posOffset);

OSErr WriteFile(const FileHandle* fileHandle, const void* data, size_t dataSize);

#endif // !FILEUTIL_H
