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

#ifndef CLIPBOARDUTIL_H
#define CLIPBOARDUTIL_H

#include "Common.h"
#include "InputLayerInfo.h"
#include <memory>

void ConvertClipboardImageToGmicInput(::std::unique_ptr<InputLayerInfo>& layer);

#endif // !CLIPBOARDUTIL_H
