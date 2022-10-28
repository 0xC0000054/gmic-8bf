////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#ifndef GMICPLUGIN_H
#define GMICPLUGIN_H

#include "Common.h" // Common definitions that are shared between plug-ins
#include "PIAbout.h"
#include "GmicIOSettings.h"

// A 4-byte Boolean used in the FilterParameters structure for alignment purposes.
typedef int32 GPBoolean;

struct FilterParameters
{
    GPBoolean lastSelectorWasParameters;
    GPBoolean showUI;
};

FilterParameters* LockParameters(FilterRecordPtr filterRecord);
void UnlockParameters(FilterRecordPtr filterRecord);

OSErr DoAbout(const AboutRecord* aboutRecord) noexcept;
OSErr ShowGmicUI(
    const boost::filesystem::path& indexFilePath,
    const boost::filesystem::path& outputDir,
    const boost::filesystem::path& gmicParametersFilePath,
    bool showFullUI,
    const FilterRecordPtr filterRecord);

OSErr ReadGmicOutput(
    const boost::filesystem::path& outputDir,
    const boost::filesystem::path& gmicParametersFilePath,
    bool fullUIWasShown,
    FilterRecord* filterRecord,
    const int32& hostBitDepth,
    const GmicIOSettings& settings);
OSErr ShowErrorMessage(const char* message, const FilterRecordPtr filterRecord, OSErr fallbackErrorCode);
OSErr WriteGmicFiles(
    const boost::filesystem::path& inputDir,
    boost::filesystem::path& indexFilePath,
    boost::filesystem::path& gmicParametersFilePath,
    FilterRecord* filterRecord,
    const int32& hostBitDepth,
    const GmicIOSettings& settings);

constexpr Fixed int2fixed(int value)
{
    return value << 16;
}

#endif // !GMICPLUGIN_H
