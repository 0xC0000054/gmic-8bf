////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2025 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#ifndef CLIPBOARDUTILWIN_H
#define CLIPBOARDUTILWIN_H

#include "Common.h"
#include "InputLayerInfo.h"
#include <memory>

void ConvertClipboardImageToGmicInputNative(::std::unique_ptr<InputLayerInfo>& layer);

#endif // !CLIPBOARDUTILWIN_H
