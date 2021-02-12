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

#ifndef FILEUTILWIN_H
#define FILEUTILWIN_H

#include <boost/filesystem.hpp>

constexpr inline const char ExecutableName[] = "gmic_8bf_qt.exe";

boost::filesystem::path GetPluginDataDirectoryNative();

boost::filesystem::path GetPluginInstallDirectoryNative();

std::unique_ptr<FileHandle> OpenFileNative(const boost::filesystem::path& path, FileOpenMode mode);

void ReadFileNative(const FileHandle* fileHandle, void* data, size_t dataSize);

void SetFileLengthNative(const FileHandle* fileHandle, int64 length);

void SetFilePositionNative(const FileHandle* fileHandle, int16 posMode, int64 posOffset);

void WriteFileNative(const FileHandle* fileHandle, const void* data, size_t dataSize);

#endif // !FILEUTILWIN_H
