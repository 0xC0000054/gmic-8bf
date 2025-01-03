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

#ifndef MEMORY_H
#define MEMORY_H

#include "Common.h"

OSErr NewPIHandle(const FilterRecordPtr filterRecord, int32 size, Handle* handle);
void DisposePIHandle(const FilterRecordPtr filterRecord, Handle handle);
Ptr LockPIHandle(const FilterRecordPtr filterRecord, Handle handle, Boolean moveHigh);
void UnlockPIHandle(const FilterRecordPtr filterRecord, Handle handle);

#endif // !MEMORY_H
