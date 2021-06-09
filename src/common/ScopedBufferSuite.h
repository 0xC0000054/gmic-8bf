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
        : bufferID(), bufferDataPtr(nullptr), filterRecord(filterRecord), bufferIDValid(false)
    {
        OSErrException::ThrowIfError(filterRecord->bufferProcs->allocateProc(bufferSize, &bufferID));
        bufferIDValid = true;
    }

    ~unique_buffer_suite_buffer()
    {
        if (bufferIDValid)
        {
            bufferIDValid = false;
            if (bufferDataPtr != nullptr)
            {
                filterRecord->bufferProcs->unlockProc(bufferID);
            }
            filterRecord->bufferProcs->freeProc(bufferID);
        }
    }

    void* Lock()
    {
        if (!bufferIDValid)
        {
            throw std::runtime_error("Cannot Lock an invalid buffer.");
        }

        if (bufferDataPtr == nullptr)
        {
            bufferDataPtr = filterRecord->bufferProcs->lockProc(bufferID, false);

            if (bufferDataPtr == nullptr)
            {
                throw std::runtime_error("Unable to lock the BufferSuite buffer.");
            }
        }

        return bufferDataPtr;
    }

private:
    BufferID bufferID;
    void* bufferDataPtr;
    FilterRecordPtr filterRecord;
    bool bufferIDValid;
};

#endif // !SCOPEDBUFFERSUITE_H
