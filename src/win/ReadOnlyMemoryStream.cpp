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

/////////////////////////////////////////////////////////////////////////////////
// Paint.NET                                                                   //
// Copyright (C) Rick Brewster, Tom Jackson, and past contributors.            //
// Portions Copyright (C) Microsoft Corporation. All Rights Reserved.          //
// See src/Resources/Files/License.txt for full licensing and attribution      //
// details.                                                                    //
// .                                                                           //
/////////////////////////////////////////////////////////////////////////////////

#pragma strict_gs_check(on)
#include "ReadOnlyMemoryStream.h"
#include <limits.h>
#include <algorithm>

#pragma warning(disable: 4100)

ReadOnlyMemoryStream::ReadOnlyMemoryStream(const void* pBuffer, size_t nSize)
    : m_pbBuffer(static_cast<const BYTE*>(pBuffer)),
    m_nSize(nSize),
    m_nPos(0)

{
     m_lRefCount = 1;

     SYSTEMTIME st;
     GetSystemTime(&st);
     SystemTimeToFileTime(&st, &m_ftCreation);
     SystemTimeToFileTime(&st, &m_ftModified);
     SystemTimeToFileTime(&st, &m_ftAccessed);
}

ReadOnlyMemoryStream::~ReadOnlyMemoryStream()
{
}

// IUnknown methods
STDMETHODIMP ReadOnlyMemoryStream::QueryInterface(REFIID iid, void **ppvObject)
{
    if (NULL == ppvObject)
    {
        return E_INVALIDARG;
    }

    if (IsEqualCLSID(iid, IID_IStream))
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(DWORD) ReadOnlyMemoryStream::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(DWORD) ReadOnlyMemoryStream::Release()
{
    DWORD dwNewRefCount = InterlockedDecrement(&m_lRefCount);

    if (0 == dwNewRefCount)
    {
        delete this;
    }

    return dwNewRefCount;
}

// IStream members
STDMETHODIMP ReadOnlyMemoryStream::Clone(IStream **ppstm)
{
    return E_NOTIMPL;
}

STDMETHODIMP ReadOnlyMemoryStream::Commit(DWORD grfCommitFlags)
{
    return S_OK;
}

STDMETHODIMP ReadOnlyMemoryStream::CopyTo(IStream *pstm,
                                   ULARGE_INTEGER cb,
                                   ULARGE_INTEGER *pcbRead,
                                   ULARGE_INTEGER *pcbWritten)
{
    return E_NOTIMPL;
}

STDMETHODIMP ReadOnlyMemoryStream::LockRegion(ULARGE_INTEGER libOffset,
                                       ULARGE_INTEGER cb,
                                       DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP ReadOnlyMemoryStream::Read(void *pv,
                                 ULONG cb,
                                 ULONG *pcbRead)
{
    if (NULL == pv)
    {
        return STG_E_INVALIDPOINTER;
    }

    const BYTE *pbEnd = m_pbBuffer + m_nSize;
    const BYTE *pbReqStart = m_pbBuffer + m_nPos;
    const BYTE *pbReqEnd = pbReqStart + cb;
    ULONG nBytesToGet = (ULONG)(::std::min(pbEnd - pbReqStart, pbReqEnd - pbReqStart));

    memcpy(pv, pbReqStart, nBytesToGet);

    if (NULL != pcbRead)
    {
        *pcbRead = nBytesToGet;
    }

    m_nPos += nBytesToGet;

    return S_OK;
}

STDMETHODIMP ReadOnlyMemoryStream::Write(const void *pv,
                                  ULONG cb,
                                  ULONG *pcbWritten)
{
    return E_NOTIMPL;
}

STDMETHODIMP ReadOnlyMemoryStream::Revert()
{
    return S_OK;
}

STDMETHODIMP ReadOnlyMemoryStream::Seek(LARGE_INTEGER dlibMove,
                                 DWORD dwOrigin,
                                 ULARGE_INTEGER *plibNewPosition)
{
    HRESULT hr = S_OK;

    LONGLONG nMove = dlibMove.QuadPart;
    LONGLONG nNewPos = 0;

    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        if (nMove >= 0 && static_cast<ULONGLONG>(nMove) < m_nSize)
        {
            m_nPos = nMove;

            if (NULL != plibNewPosition)
            {
                plibNewPosition->QuadPart = m_nPos;
            }
        }
        else
        {
            hr = STG_E_INVALIDFUNCTION;
        }

        break;

    case STREAM_SEEK_CUR:
        nNewPos = m_nPos + nMove;

        if (nNewPos >= 0 && static_cast<ULONGLONG>(nNewPos) < m_nSize)
        {
            m_nPos = nNewPos;

            if (NULL != plibNewPosition)
            {
                plibNewPosition->QuadPart = m_nPos;
            }
        }
        else
        {
            hr = STG_E_INVALIDFUNCTION;
        }

        break;

    case STREAM_SEEK_END:
        nNewPos = m_nSize - 1 + nMove;

        if (nNewPos >= 0 && static_cast<ULONGLONG>(nNewPos) < m_nSize)
        {
            m_nPos = nNewPos;

            if (NULL != plibNewPosition)
            {
                plibNewPosition->QuadPart = m_nPos;
            }
        }
        else
        {
            hr = STG_E_INVALIDFUNCTION;
        }

        break;

    default:
        hr = STG_E_INVALIDFUNCTION;
        break;
    }

    return hr;
}

STDMETHODIMP ReadOnlyMemoryStream::SetSize(ULARGE_INTEGER libNewSize)
{
    return E_NOTIMPL;
}

STDMETHODIMP ReadOnlyMemoryStream::Stat(STATSTG *pstatstg,
                                 DWORD grfStatFlag)
{
    if (NULL == pstatstg)
    {
        return STG_E_INVALIDPOINTER;
    }

    ZeroMemory(pstatstg, sizeof(*pstatstg));

    if (grfStatFlag != STATFLAG_NONAME)
    {
        WCHAR wszName[64];
        wsprintfW(wszName, L"%p", m_pbBuffer);
        size_t nSize = 1 + wcslen(wszName);
        pstatstg->pwcsName = (LPOLESTR)CoTaskMemAlloc(nSize);

        if (NULL == pstatstg->pwcsName)
        {
            return E_OUTOFMEMORY;
        }

        wcscpy_s(pstatstg->pwcsName, nSize - 1, wszName);
    }

    pstatstg->cbSize.QuadPart = (ULONGLONG)m_nSize;
    pstatstg->type = STGTY_STREAM;
    pstatstg->ctime = m_ftCreation;
    pstatstg->mtime = m_ftModified;
    pstatstg->atime = m_ftAccessed;
    pstatstg->grfMode = STGM_READ;
    pstatstg->grfLocksSupported = 0;
    pstatstg->clsid = CLSID_NULL;
    pstatstg->grfStateBits = 0;
    pstatstg->reserved = 0;

    return S_OK;
}

STDMETHODIMP ReadOnlyMemoryStream::UnlockRegion(ULARGE_INTEGER libOffset,
                                         ULARGE_INTEGER cb,
                                         DWORD dwLockType)
{
    return E_NOTIMPL;
}

#pragma warning(default: 4100)
