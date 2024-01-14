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

::std::string FourCCToString(const uint32 fourCC)
{
    ::std::string value(4, '\0');

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

OSErr LaunderOSErrResult(OSErr err, const char* caption, const FilterRecordPtr filterRecord)
{
    // A positive error code indicates that a custom error message was already shown to the user.
    // The negative error codes are standard errors that plug-ins can use in place of a custom error message.
    // A value of zero is used to indicate that no error has occurred.
    //
    // When a plug-in exits with a standard error code the host is supposed to show an error message
    // to the user for all error codes except userCanceledErr, but a number of popular 3rd-party hosts
    // silently ignore all error codes that are returned from a plug-in.
    if (err < 0 && err != userCanceledErr)
    {
        const char* message = nullptr;

        switch (err)
        {
        case readErr:
        case writErr:
        case openErr:
        case ioErr:
            message = "A file I/O error occurred.";
            break;
        case eofErr:
            message = "Reached the end of the file.";
            break;
        case dskFulErr:
            message = "There is not enough space on the disk.";
            break;
        case fLckdErr:
            message = "The file is in use or locked by another process.";
            break;
        case vLckdErr:
            message = "The disk is in use or locked by another process.";
            break;
        case fnfErr:
            message = "The system cannot find the file specified.";
            break;
        case memFullErr:
        case memWZErr:
        case nilHandleErr:
            message = "Insufficient memory to continue execution of the plug-in.";
            break;
        case errPlugInHostInsufficient:
            message = "The plug-in requires services not provided by this host.";
            break;
        case filterBadMode:
            message = "This plug-in does not support the current image mode.";
            break;
        case filterBadParameters:
        case paramErr:
        default:
            message = "A problem with the filter module interface.";
            break;
        }

        return ShowErrorMessageNative(message, caption, filterRecord, err);
    }

    return err;
}

OSErr ShowErrorMessage(
    const char* message,
    const char* caption,
    const FilterRecordPtr filterRecord,
    OSErr fallbackErrorCode)
{
    return ShowErrorMessageNative(message, caption, filterRecord, fallbackErrorCode);
}
