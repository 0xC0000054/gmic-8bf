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

#ifndef MEMORYWIN_H
#define MEMORYWIN_H

#include "GmicPlugin.h"

Handle	NewHandle(int32 size);
void	DisposeHandle(Handle handle);

#endif // MEMORYWIN_H
