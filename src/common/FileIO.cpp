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

#include "FileIO.h"
#include "PIDefines.h"

#ifdef __PIWin__
#include "FileIOWin.h"
#else
#error "Missing a FileIO header for this platform."
#endif

::std::unique_ptr<FileHandle> OpenFile(const boost::filesystem::path& path, FileOpenMode mode)
{
    return OpenFileNative(path, mode);
}

void ReadFile(FileHandle* fileHandle, void* data, size_t dataSize)
{
    ReadFileNative(fileHandle, data, dataSize);
}

void SetFileLength(FileHandle* fileHandle, int64 length)
{
    SetFileLengthNative(fileHandle, length);
}

int64 GetFilePosition(FileHandle* fileHandle)
{
    return GetFilePositionNative(fileHandle);
}

void SetFilePosition(FileHandle* fileHandle, int64 posOffset)
{
    SetFilePositionNative(fileHandle, posOffset);
}

void WriteFile(FileHandle* fileHandle, const void* data, size_t dataSize)
{
    WriteFileNative(fileHandle, data, dataSize);
}
