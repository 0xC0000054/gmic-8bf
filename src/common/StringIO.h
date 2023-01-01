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

#ifndef STRINGIO_H
#define STRINGIO_H

#include "FileIO.h"
#include <string>

void ReadUtf8String(FileHandle* fileHandle, ::std::string& value);
void WriteUtf8String(FileHandle* fileHandle, const ::std::string& value);

#endif // !STRINGIO_H
