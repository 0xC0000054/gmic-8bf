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

#ifndef UTILITIES_H
#define UTILITIES_H

#include "Common.h"
#include "PIProperties.h"

// Support compiling with the 7.0 and earlier SDKs.
#if defined(kCurrentMaxVersReadLayerDesc) && defined(propNumberOfLayers) && defined(propTargetLayerIndex)
#define PSSDK_HAS_LAYER_SUPPORT 1
#else
#define PSSDK_HAS_LAYER_SUPPORT 0
#endif

bool HandleSuiteIsAvailable(const FilterRecord* filterRecord);
bool SPBasicSuiteIsAvailable(const FilterRecord* filterRecord);
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

#endif // !UTILITIES_H
