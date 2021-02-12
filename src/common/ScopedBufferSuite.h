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

#ifndef SCOPEDBUFFERSUITE_H
#define SCOPEDBUFFERSUITE_H

#include "Common.h"

class unique_buffer_suite_buffer
{
public:
    explicit unique_buffer_suite_buffer(FilterRecordPtr filterRecord, int32 bufferSize)
        : bufferID(), filterRecord(filterRecord), bufferIDValid(false)
    {
        OSErrException::ThrowIfError(filterRecord->bufferProcs->allocateProc(bufferSize, &bufferID));
        bufferIDValid = true;
    }

    ~unique_buffer_suite_buffer()
    {
        if (bufferIDValid)
        {
            bufferIDValid = false;
            filterRecord->bufferProcs->unlockProc(bufferID);
            filterRecord->bufferProcs->freeProc(bufferID);
        }
    }

    void* Lock()
    {
        return filterRecord->bufferProcs->lockProc(bufferID, false);
    }

private:
    BufferID bufferID;
    FilterRecordPtr filterRecord;
    bool bufferIDValid;
};

#endif // !SCOPEDBUFFERSUITE_H
