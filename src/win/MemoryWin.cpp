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

#include "MemoryWin.h"
#include <Windows.h>
#include <windowsx.h>

// The following methods were adapted from WinUtilities.cpp in the PS6 SDK:

#define signatureSize	4
static char cSig[signatureSize] = { 'O', 'T', 'O', 'F' };

#pragma warning(disable: 6387)
#pragma warning(disable: 28183)

Handle NewHandle(int32 size)
{
    Handle mHand;
    char* p;

    mHand = (Handle)GlobalAllocPtr(GHND, (sizeof(Handle) + signatureSize));

    if (mHand)
        *mHand = (Ptr)GlobalAllocPtr(GHND, size);

    if (!mHand || !(*mHand))
    {
        return NULL;
    }

    // put the signature after the pointer
    p = (char*)mHand;
    p += sizeof(Handle);
    memcpy(p, cSig, signatureSize);

    return mHand;

}

void DisposeHandle(Handle handle)
{
    if (handle)
    {
        Ptr p;

        p = *handle;

        if (p)
            GlobalFreePtr(p);

        GlobalFreePtr((Ptr)handle);
    }
}

#pragma warning(default: 6387)
#pragma warning(default: 28183)
