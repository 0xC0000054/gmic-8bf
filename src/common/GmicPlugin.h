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
#include "PIProperties.h"
#include "GmicIOSettings.h"

// A 4-byte Boolean used in the FilterParameters structure for alignment purposes.
typedef int32 GPBoolean;

struct FilterParameters
{
    GPBoolean lastSelectorWasParameters;
    GPBoolean showUI;
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

OSErr ReadGmicOutput(
    const boost::filesystem::path& outputDir,
    FilterRecord* filterRecord,
    const GmicIOSettings& settings);
OSErr WriteGmicFiles(
    const boost::filesystem::path& inputDir,
    boost::filesystem::path& indexFilePath,
    FilterRecord* filterRecord,
    const GmicIOSettings& settings);

constexpr Fixed int2fixed(int value)
{
    return value << 16;
}

OSErr NewPIHandle(const FilterRecordPtr filterRecord, int32 size, Handle* handle);
void DisposePIHandle(const FilterRecordPtr filterRecord, Handle handle);
Ptr LockPIHandle(const FilterRecordPtr filterRecord, Handle handle, Boolean moveHigh);
void UnlockPIHandle(const FilterRecordPtr filterRecord, Handle handle);

bool HandleSuiteIsAvailable(const FilterRecord* filterRecord);
bool TryGetLayerNameAsUTF8String(const FilterRecord* filterRecord, std::string& utf8LayerName);
bool HostMeetsRequirements(const FilterRecord* filterRecord) noexcept;
int32 GetImagePlaneCount(int16 imageMode, int32 layerPlanes, int32 transparencyPlanes);
VPoint GetImageSize(const FilterRecordPtr filterRecord);
int32 GetTileHeight(int16 suggestedTileHeight);
int32 GetTileWidth(int16 suggestedTileWidth);
void SetInputRect(FilterRecordPtr filterRecord, int32 top, int32 left, int32 bottom, int32 right);
void SetOutputRect(FilterRecordPtr filterRecord, int32 top, int32 left, int32 bottom, int32 right);
void SetMaskRect(FilterRecordPtr filterRecord, int32 top, int32 left, int32 bottom, int32 right);
bool TryMultiplyInt32(int32 a, int32 x, int32& result);

#if PSSDK_HAS_LAYER_SUPPORT
std::string ConvertLayerNameToUTF8(const uint16* layerName);
bool DocumentHasMultipleLayers(const FilterRecord* filterRecord);
bool HostSupportsReadingFromMultipleLayers(const FilterRecord* filterRecord);
bool TryGetTargetLayerIndex(const FilterRecord* filterRecord, int32& targetLayerIndex);
#endif // PSSDK_HAS_LAYER_SUPPORT

#endif // !GMICPLUGIN_H
