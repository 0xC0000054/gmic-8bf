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

#ifndef GMICPROCESS_H
#define GMICPROCESS_H

#include "Common.h"

class GmicProcessErrorInfo
{
public:

    GmicProcessErrorInfo();

    const char* GetErrorMessage() const;

    bool HasErrorMessage() const;

    void SetErrorMesage(const char* message);

    void SetErrorMesageFormat(const char* format, ...);

private:

    bool hasErrorMessage;
    char errorMessage[2048];
};

OSErr ExecuteGmicQt(
    const boost::filesystem::path& indexFilePath,
    const boost::filesystem::path& outputDir,
    const boost::filesystem::path& gmicParametersFilePath,
    bool showFullUI,
    GmicProcessErrorInfo& errorInfo);

#endif // !GMICPROCESS_H
