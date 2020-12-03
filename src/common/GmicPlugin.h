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

#pragma once

// Suppress C4121: 'FilterRecord': alignment of a member was sensitive to packing
#pragma warning(push)
#pragma warning(disable: 4121)

#include "PIDefines.h"
#include "PITypes.h"
#include "PIAbout.h"
#include "PIFilter.h"
#include "PIUSuites.h"
#include "PIProperties.h"
#include <stdlib.h>
#include <string>
#include <boost/filesystem.hpp>

#pragma warning(pop)

// A 4-byte Boolean used in the FilterParameters structure for alignment purposes.
typedef int32 GPBoolean;

struct FilterParameters
{
    GPBoolean lastSelectorWasParameters;
    GPBoolean showUI;
    Handle lastOutputFolder;
};

// Support compiling with the 7.0 and earlier SDKs.
#if defined(kCurrentMaxVersReadLayerDesc) && defined(propNumberOfLayers) && defined(propTargetLayerIndex)
#define PSSDK_HAS_LAYER_SUPPORT 1
#else
#define PSSDK_HAS_LAYER_SUPPORT 0
#endif

FilterParameters* LockParameters(FilterRecordPtr filterRecord);
void UnlockParameters(FilterRecordPtr filterRecord);

OSErr DoAbout(const AboutRecord* aboutRecord) noexcept;
OSErr ShowErrorMessage(const char* message, const FilterRecordPtr filterRecord, OSErr fallbackErrorCode);

OSErr ReadGmicOutput(const boost::filesystem::path& outputDir, FilterRecord* filterRecord);
OSErr WriteGmicFiles(
    const boost::filesystem::path& inputDir,
    boost::filesystem::path& indexFilePath,
    FilterRecord* filterRecord);

bool TryGetLayerNameAsUTF8String(const FilterRecord* filterRecord, std::string& utf8LayerName);
bool HostMeetsRequirements(const FilterRecord* filterRecord) noexcept;
Fixed int2fixed(int value);
VPoint GetImageSize(FilterRecordPtr filterRecord);
void SetInputRect(FilterRecordPtr filterRecord, int32 top, int32 left, int32 bottom, int32 right);
void SetOutputRect(FilterRecordPtr filterRecord, int32 top, int32 left, int32 bottom, int32 right);
void SetMaskRect(FilterRecordPtr filterRecord, int32 top, int32 left, int32 bottom, int32 right);

#if PSSDK_HAS_LAYER_SUPPORT
std::string ConvertLayerNameToUTF8(const uint16* layerName);
bool DocumentHasMultipleLayers(const FilterRecord* filterRecord);
bool HostSupportsReadingFromMultipleLayers(const FilterRecord* filterRecord);
bool TryGetTargetLayerIndex(const FilterRecord* filterRecord, int32& targetLayerIndex);
#endif // PSSDK_HAS_LAYER_SUPPORT


#if _DEBUG
void DebugOut(const char* fmt, ...) noexcept;
std::string FourCCToString(const uint32 fourCC);
#else
#define DebugOut(fmt, ...)
#define FourCCToString(fourCC)
#endif // _DEBUG

#define PrintFunctionName() DebugOut("%s", __FUNCTION__)
