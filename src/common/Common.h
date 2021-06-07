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

#ifndef COMMON_H
#define COMMON_H

#ifdef _MSC_VER
#pragma warning(push)
// Suppress C4121: 'FilterRecord': alignment of a member was sensitive to packing
#pragma warning(disable: 4121)
// Suppress C26495: Variable '%variable%' is uninitialized.
#pragma warning(disable: 26495)
#endif // _MSC_VER

#include "PIDefines.h"
#include "PITypes.h"
#include "PIFilter.h"
#include "OSErrException.h"
#include <stdlib.h>
#include <string>
#include <boost/filesystem.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define DEBUG_BUILD 1
#else
#define DEBUG_BUILD 0
#endif

#if DEBUG_BUILD
void DebugOut(const char* fmt, ...) noexcept;
std::string FourCCToString(const uint32 fourCC);
#else
#define DebugOut(fmt, ...)
#define FourCCToString(fourCC)
#endif // DEBUG_BUILD

#define PrintFunctionName() DebugOut("%s", __FUNCTION__)

// Displays an error message to the user for the OSErr results that need it
// and tells the host that the plug-in has handled the error.
// This is necessary because a number of popular 3rd-party hosts silently
// ignore the error codes that a plug-in may return.
OSErr LaunderOSErrResult(OSErr err, const char* caption, const FilterRecordPtr filterRecord);
OSErr ShowErrorMessage(
    const char* message,
    const char* caption,
    const FilterRecordPtr filterRecord,
    OSErr fallbackErrorCode);

#endif // !COMMON_H
