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

/////////////////////////////////////////////////////////////////////////////////
// Paint.NET                                                                   //
// Copyright (C) Rick Brewster, Tom Jackson, and past contributors.            //
// Portions Copyright (C) Microsoft Corporation. All Rights Reserved.          //
// See src/Resources/Files/License.txt for full licensing and attribution      //
// details.                                                                    //
// .                                                                           //
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Shlobj.h>

class ReadOnlyMemoryStream
    : public IStream
{
public:
    ReadOnlyMemoryStream(const void *pBuffer, size_t nSize);
    ~ReadOnlyMemoryStream();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObject);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    // ISequentialStream members
    STDMETHODIMP Read(void *pv,
                      ULONG cb,
                      ULONG *pcbRead);

    STDMETHODIMP Write(const void *pv,
                       ULONG cb,
                       ULONG *pcbWritten);

    // IStream members
    STDMETHODIMP Clone(IStream **ppstm);
    STDMETHODIMP Commit(DWORD grfCommitFlags);

    STDMETHODIMP CopyTo(IStream *pstm,
                        ULARGE_INTEGER cb,
                        ULARGE_INTEGER *pcbRead,
                        ULARGE_INTEGER *pcbWritten);

    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
                            DWORD dwLockType);

    STDMETHODIMP Revert();

    STDMETHODIMP Seek(LARGE_INTEGER dlibMove,
                      DWORD dwOrigin,
                      ULARGE_INTEGER *plibNewPosition);

    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);

    STDMETHODIMP Stat(STATSTG *pstatstg,
                      DWORD grfStatFlag);

    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset,
                              ULARGE_INTEGER cb,
                              DWORD dwLockType);

protected:
    volatile LONG m_lRefCount;

private:
    const BYTE *m_pbBuffer;
    const ULONGLONG m_nSize;
    ULONGLONG m_nPos;
    FILETIME m_ftCreation;
    FILETIME m_ftModified;
    FILETIME m_ftAccessed;
};
