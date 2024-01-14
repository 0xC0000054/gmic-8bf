////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022, 2023, 2024 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#ifndef OSERREXCEPTION_H
#define OSERREXCEPTION_H

#include "PITypes.h"
#include <stdexcept>

class OSErrException : public ::std::exception
{
public:
    explicit OSErrException(OSErr err) noexcept : error(err)
    {
    }

    OSErr GetErrorCode() const noexcept
    {
        return error;
    }

    static void ThrowIfError(OSErr err)
    {
        if (err != noErr)
        {
            throw OSErrException(err);
        }
    }

private:
    OSErr error;
};

#endif // !OSERREXCEPTION_H
