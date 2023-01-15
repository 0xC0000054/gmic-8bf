////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022, 2023 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#ifndef FILEIOWIN_H
#define FILEIOWIN_H

#include "FileIO.h"

::std::unique_ptr<FileHandle> OpenFileNative(const boost::filesystem::path& path, FileOpenMode mode);

void ReadFileNative(FileHandle* fileHandle, void* data, size_t dataSize);

void SetFileLengthNative(FileHandle* fileHandle, int64 length);

int64 GetFilePositionNative(FileHandle* fileHandle);

void SetFilePositionNative(FileHandle* fileHandle, int64 posOffset);

void WriteFileNative(FileHandle* fileHandle, const void* data, size_t dataSize);

#endif // !FILEIOWIN_H
