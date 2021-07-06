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

#include "stdafx.h"
#include "GmicProcess.h"
#include "FileUtil.h"
#include <boost/process.hpp>

OSErr ExecuteGmicQt(
    const boost::filesystem::path& indexFilePath,
    const boost::filesystem::path& outputDir,
    const boost::filesystem::path& gmicParametersFilePath,
    bool showFullUI,
    GmicProcessErrorInfo& errorInfo)
{
    OSErr err = noErr;
    errorInfo.hasErrorMessage = false;
    memset(errorInfo.errorMessage, 0, sizeof(errorInfo.errorMessage));
    constexpr size_t MaxErrorStringLength = sizeof(errorInfo.errorMessage) - 1;

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
            errorInfo.hasErrorMessage = true;
            std::strncpy(
                errorInfo.errorMessage,
                "A G'MIC-Qt argument is invalid.",
                MaxErrorStringLength);
            break;
        case 4:
            err = ioErr;
            errorInfo.hasErrorMessage = true;
            std::strncpy(
                errorInfo.errorMessage,
                "An unspecified error occurred when reading the G'MIC-Qt input images.",
                MaxErrorStringLength);
            break;
        case 5:
            err = userCanceledErr;
            break;
        case 6:
            err = ioErr;
            errorInfo.hasErrorMessage = true;
            std::strncpy(
                errorInfo.errorMessage,
                "Unable to open one of the G'MIC-Qt input images.",
                MaxErrorStringLength);
            break;
        case 7:
            err = ioErr;
            errorInfo.hasErrorMessage = true;
            std::strncpy(
                errorInfo.errorMessage,
                "The G'MIC-Qt input images use an unknown format.",
                MaxErrorStringLength);
            break;
        case 8:
            err = ioErr;
            errorInfo.hasErrorMessage = true;
            std::strncpy(
                errorInfo.errorMessage,
                "The G'MIC-Qt input images have an unsupported file version.",
                MaxErrorStringLength);
            break;
        case 9:
            err = memFullErr;
            break;
        case 10:
            err = eofErr;
            break;
        case 11:
            err = ioErr;
            errorInfo.hasErrorMessage = true;
            std::strncpy(
                errorInfo.errorMessage,
                "The Qt platform byte order does not match the plug-in.",
                MaxErrorStringLength);
            break;
        default:
            err = ioErr;
            errorInfo.hasErrorMessage = std::snprintf(
                errorInfo.errorMessage,
                MaxErrorStringLength,
                "An unspecified error occurred when running G'MIC-Qt, exit code=%d.",
                exitCode) > 0;
            break;
        }
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (const std::exception& e)
    {
        err = ioErr;
        errorInfo.hasErrorMessage = true;
        std::strncpy(
            errorInfo.errorMessage,
            e.what(),
            MaxErrorStringLength);
    }
    catch (...)
    {
        err = ioErr;
        errorInfo.hasErrorMessage = true;
        std::strncpy(
            errorInfo.errorMessage,
            "An unspecified error occurred when running G'MIC-Qt.",
            MaxErrorStringLength);
    }

    return err;
}
