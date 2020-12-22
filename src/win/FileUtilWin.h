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

#ifndef FILEUTILWIN_H
#define FILEUTILWIN_H

#include <boost/filesystem.hpp>

constexpr inline const char ExecutableName[] = "gmic_8bf_qt.exe";

boost::filesystem::path GetPluginDataDirectoryNative();

OSErr GetPluginInstallDirectoryNative(boost::filesystem::path& path);

OSErr OpenFileNative(const boost::filesystem::path& path, FileOpenMode mode, std::unique_ptr<FileHandle>& fileHandle);

OSErr ReadFileNative(const FileHandle* fileHandle, void* data, size_t dataSize);

OSErr SetFileLengthNative(const FileHandle* fileHandle, int64 length);

OSErr SetFilePositionNative(const FileHandle* fileHandle, int16 posMode, int64 posOffset);

OSErr WriteFileNative(const FileHandle* fileHandle, const void* data, size_t dataSize);

#endif // !FILEUTILWIN_H
