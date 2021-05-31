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

#ifndef CLIPBOARDUTILWIN_H
#define CLIPBOARDUTILWIN_H

#include "Common.h"

void ConvertClipboardImageToGmicInputNative(const boost::filesystem::path& gmicInputPath);

#endif // !CLIPBOARDUTILWIN_H
