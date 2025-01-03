////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2025 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "StringIO.h"

void ReadUtf8String(FileHandle* fileHandle, ::std::string& value)
{
    int32_t stringLength = 0;

    ReadFile(fileHandle, &stringLength, sizeof(stringLength));

    if (stringLength == 0)
    {
        value = ::std::string();
    }
    else
    {
        ::std::vector<char> stringChars(stringLength);

        ReadFile(fileHandle, &stringChars[0], stringLength);

        value.assign(stringChars.begin(), stringChars.end());
    }
}

void WriteUtf8String(FileHandle* fileHandle, const ::std::string& value)
{
    if (value.size() > static_cast<size_t>(::std::numeric_limits<int32>::max()))
    {
        throw ::std::runtime_error("The string length exceeds 2GB.");
    }

    int32_t stringLength = static_cast<int32>(value.size());

    WriteFile(fileHandle, &stringLength, sizeof(stringLength));

    WriteFile(fileHandle, value.data(), value.size());
}
