////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2026 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "Memory.h"
#include "Utilities.h"

#if __PIWin__
#include "MemoryWin.h"
#else
#include "LowMem.h"
#endif // __PIWin__

// The following methods were adapted from the Host*Handle methods in the PS6 SDK:

OSErr NewPIHandle(const FilterRecordPtr filterRecord, int32 size, Handle* handle)
{
    if (handle == nullptr)
    {
        return nilHandleErr;
    }

    if (HandleSuiteIsAvailable(filterRecord))
    {
        *handle = filterRecord->handleProcs->newProc(size);
    }
    else
    {
        *handle = NewHandle(size);
    }

    return *handle != nullptr ? noErr : memFullErr;
}

void DisposePIHandle(const FilterRecordPtr filterRecord, Handle handle)
{
    if (HandleSuiteIsAvailable(filterRecord))
    {
        filterRecord->handleProcs->disposeProc(handle);
    }
    else
    {
        DisposeHandle(handle);
    }
}

Ptr LockPIHandle(const FilterRecordPtr filterRecord, Handle handle, Boolean moveHigh)
{
    if (HandleSuiteIsAvailable(filterRecord))
    {
        return filterRecord->handleProcs->lockProc(handle, moveHigh);
    }
    else
    {
        // Use OS routines:

#ifdef __PIMac__
        if (moveHigh)
            MoveHHi(handle);
        HLock(handle);

        return *handle; // dereference and return pointer

#else // WIndows

        return (Ptr)GlobalLock(handle);

#endif
    }
}

void UnlockPIHandle(const FilterRecordPtr filterRecord, Handle handle)
{
    if (HandleSuiteIsAvailable(filterRecord))
    {
        filterRecord->handleProcs->unlockProc(handle);
    }
    else
    {
        // Use OS routines:
#ifdef __PIMac__

        HUnlock(handle);

#else // Windows

        GlobalUnlock(handle);

#endif
    }
}

