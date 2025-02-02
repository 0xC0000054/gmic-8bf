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

#ifndef TARGETVER_H
#define TARGETVER_H

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#ifdef _M_ARM64
#define _WIN32_WINNT 0x0A00 // ARM64 is only supported on Windows 10 and later.
#else
#define _WIN32_WINNT 0x0601 // Windows 7 or later.
#endif

#include <winsdkver.h>
#include <SDKDDKVer.h>

#endif // !TARGETVER_H

