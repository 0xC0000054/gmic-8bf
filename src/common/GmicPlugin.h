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

#pragma once

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

OSErr ReadGmicOutput(
    const boost::filesystem::path& outputDir,
    FilterRecord* filterRecord,
    const GmicIOSettings& settings);
OSErr ShowErrorMessage(const char* message, const FilterRecordPtr filterRecord, OSErr fallbackErrorCode);
OSErr WriteGmicFiles(
    const boost::filesystem::path& inputDir,
    boost::filesystem::path& indexFilePath,
    FilterRecord* filterRecord,
    const GmicIOSettings& settings);

constexpr Fixed int2fixed(int value)
{
    return value << 16;
}

#endif // !GMICPLUGIN_H
