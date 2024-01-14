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

#include "ClipboardUtil.h"

#ifdef __PIWin__
#include "ClipboardUtilWin.h"
#else
#error "Missing a ClipboardUtil header for this platform."
#endif

void ConvertClipboardImageToGmicInput(::std::unique_ptr<InputLayerInfo>& layer)
{
    ConvertClipboardImageToGmicInputNative(layer);
}
