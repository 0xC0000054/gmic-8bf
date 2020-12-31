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

#include "Common.h"
#include <string>
#include <boost/predef.h>

#if __PIWin__
#include "CommonUIWin.h"
#else
#error "Missing a CommonUI header for this platform"
#endif

#if DEBUG_BUILD
void DebugOut(const char* fmt, ...) noexcept
{
#if __PIWin__
    va_list argp;
    char dbg_out[4096] = {};

    va_start(argp, fmt);
    vsprintf_s(dbg_out, fmt, argp);
    va_end(argp);

    OutputDebugStringA(dbg_out);
    OutputDebugStringA("\n");
#else
#error "Debug output has not been configured for this platform."
#endif // __PIWin__
}

std::string FourCCToString(const uint32 fourCC)
{
    std::string value(4, '\0');

    const char* sigPtr = reinterpret_cast<const char*>(&fourCC);

#if BOOST_ENDIAN_BIG_BYTE
    value.insert(0, 1, sigPtr[0]);
    value.insert(1, 1, sigPtr[1]);
    value.insert(2, 1, sigPtr[2]);
    value.insert(3, 1, sigPtr[3]);
#elif BOOST_ENDIAN_LITTLE_BYTE
    value.insert(0, 1, sigPtr[3]);
    value.insert(1, 1, sigPtr[2]);
    value.insert(2, 1, sigPtr[1]);
    value.insert(3, 1, sigPtr[0]);
#else
#error "Unknown endianness on this platform."
#endif

    return value;
}
#endif // DEBUG_BUILD

OSErr ShowErrorMessage(const char* message, const FilterRecordPtr filterRecord, OSErr fallbackErrorCode)
{
    return ShowErrorMessageNative(message, filterRecord, fallbackErrorCode);
}
