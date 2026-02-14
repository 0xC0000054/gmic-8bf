////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2026 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GmicProcess.h"
#include "FileUtil.h"
#include <string>
#include <boost/process.hpp>

GmicProcessErrorInfo::GmicProcessErrorInfo()
    : hasErrorMessage(false), errorMessage{}
{
}

const char* GmicProcessErrorInfo::GetErrorMessage() const
{
    return errorMessage;
}

bool GmicProcessErrorInfo::HasErrorMessage() const
{
    return hasErrorMessage;
}

void GmicProcessErrorInfo::SetErrorMesage(const char* message)
{
    constexpr size_t MaxErrorStringLength = sizeof(errorMessage) - 1;

    hasErrorMessage = true;
    ::std::strncpy(
        errorMessage,
        message,
        MaxErrorStringLength);
}

void GmicProcessErrorInfo::SetErrorMesageFormat(const char* format, ...)
{
    va_list args;

    va_start(args, format);

    hasErrorMessage = ::std::vsnprintf(errorMessage, sizeof(errorMessage), format, args) > 0;

    va_end(args);
}

OSErr ExecuteGmicQt(
    const boost::filesystem::path& indexFilePath,
    const boost::filesystem::path& outputDir,
    const boost::filesystem::path& gmicParametersFilePath,
    bool showFullUI,
    GmicProcessErrorInfo& errorInfo)
{
    OSErr err = noErr;

    try
    {
        boost::filesystem::path gmicExecutablePath = GetGmicQtPath();

        boost::filesystem::path reapply = showFullUI ? "" : "reapply";

        boost::process::child child(gmicExecutablePath, indexFilePath, outputDir, gmicParametersFilePath, reapply);

        child.wait();

        int exitCode = child.exit_code();

        switch (exitCode)
        {
        case 0:
            // No error
            break;
        case 1:
        case 2:
        case 3:
            err = ioErr;
            errorInfo.SetErrorMesage("A G'MIC-Qt argument is invalid.");
            break;
        case 4:
            err = ioErr;
            errorInfo.SetErrorMesage("An unspecified error occurred when reading the G'MIC-Qt input images.");
            break;
        case 5:
            err = userCanceledErr;
            break;
        case 6:
            err = ioErr;
            errorInfo.SetErrorMesage("Unable to open one of the G'MIC-Qt input images.");
            break;
        case 7:
            err = ioErr;
            errorInfo.SetErrorMesage("The G'MIC-Qt input images use an unknown format.");
            break;
        case 8:
            err = ioErr;
            errorInfo.SetErrorMesage("The G'MIC-Qt input images have an unsupported file version.");
            break;
        case 9:
            err = memFullErr;
            break;
        case 10:
            err = eofErr;
            break;
        case 11:
            err = ioErr;
            errorInfo.SetErrorMesage("The Qt platform byte order does not match the plug-in.");
            break;
        case 12:
            err = ioErr;
            errorInfo.SetErrorMesage("An error occurred when reading from one of the input files.");
            break;
        case 13:
            err = ioErr;
            errorInfo.SetErrorMesage("An error occurred when loading the image color profiles.");
            break;
        default:
            err = ioErr;
            errorInfo.SetErrorMesageFormat(
                "An unspecified error occurred when running G'MIC-Qt, exit code=0x%x.",
                exitCode);
            break;
        }
    }
    catch (const ::std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (const ::std::exception& e)
    {
        err = ioErr;
        errorInfo.SetErrorMesage(e.what());
    }
    catch (...)
    {
        err = ioErr;
        errorInfo.SetErrorMesage("An unspecified error occurred when running G'MIC-Qt.");
    }

    return err;
}
