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

#pragma once

#ifndef ALPHA_H
#define ALPHA_H

#include "GmicPlugin.h"
#include "FileIO.h"

void PremultiplyAlpha(
    FileHandle* fileHandle,
    uint8* tileBuffer,
    int32 tileWidth,
    int32 tileHeight,
    FilterRecord* filterRecord,
    const VPoint& imageSize,
    int32 bitsPerChannel);

void SetAlphaChannelToOpaque(FilterRecord* filterRecord, const VPoint& imageSize, int32 bitsPerChannel);


#endif // !ALPHA_H
