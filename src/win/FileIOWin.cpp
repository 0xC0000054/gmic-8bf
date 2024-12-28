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

#include "FileIOWin.h"
#include <vector>
#include <wil/resource.h>

namespace
{
    DWORD ReadCore(HANDLE hFile, BYTE* buffer, size_t count)
    {
        try
        {
            DWORD numBytesToRead = 0x80000000UL;

            if (count < numBytesToRead)
            {
                numBytesToRead = static_cast<DWORD>(count);
            }

            DWORD bytesRead = 0;

            THROW_IF_WIN32_BOOL_FALSE(ReadFile(hFile, buffer, numBytesToRead, &bytesRead, nullptr));

            return bytesRead;
        }
        catch (const wil::ResultException& e)
        {
            if (e.GetErrorCode() == E_OUTOFMEMORY)
            {
                throw ::std::bad_alloc();
            }
            else
            {
                throw ::std::runtime_error(e.what());
            }
        }
    }

    void ReadCoreBlocking(HANDLE hFile, BYTE* buffer, size_t count)
    {
        size_t offset = 0;
        size_t bytesRemaining = count;

        while (bytesRemaining > 0)
        {
            DWORD bytesRead = ReadCore(hFile, buffer + offset, bytesRemaining);

            if (bytesRead == 0)
            {
                throw ::std::runtime_error("Attempted to read beyond the end of the file.");
            }

            offset += bytesRead;
            bytesRemaining -= bytesRead;
        }
    }

    void WriteCore(HANDLE hFile, const BYTE* buffer, size_t count)
    {
        try
        {
            size_t bytesRemaining = count;

            while (bytesRemaining > 0)
            {
                DWORD numBytesToWrite = 0x80000000UL;

                if (bytesRemaining < numBytesToWrite)
                {
                    numBytesToWrite = static_cast<DWORD>(bytesRemaining);
                }

                DWORD bytesWritten = 0;

                THROW_IF_WIN32_BOOL_FALSE(WriteFile(hFile, buffer, numBytesToWrite, &bytesWritten, nullptr));

                buffer += bytesWritten;
                bytesRemaining -= bytesWritten;
            }
        }
        catch (const wil::ResultException& e)
        {
            if (e.GetErrorCode() == E_OUTOFMEMORY)
            {
                throw ::std::bad_alloc();
            }
            else
            {
                throw ::std::runtime_error(e.what());
            }
        }
    }
}

struct FileBuffer
{
    ::std::vector<BYTE> buffer;
    size_t readOffset;
    size_t readLength;
    size_t writeOffset;

    FileBuffer()
        : buffer(4096), readOffset(0), readLength(0), writeOffset(0)
    {
    }
};

class FileHandleWin : public FileHandle
{
public:
    FileHandleWin(const boost::filesystem::path& path, FileOpenMode mode, int64 preallocationSize)
        : buffer(::std::make_unique<FileBuffer>()), uncaughtExceptionCount(::std::uncaught_exceptions()), hFile()
    {
        DWORD dwDesiredAccess;
        DWORD dwShareMode;
        DWORD dwCreationDisposition;
        DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

        switch (mode)
        {
        case FileOpenMode::Read:
            dwDesiredAccess = GENERIC_READ;
            dwShareMode = FILE_SHARE_READ;
            dwCreationDisposition = OPEN_EXISTING;
            break;
        case FileOpenMode::Write:
            dwDesiredAccess = GENERIC_WRITE;
            dwShareMode = 0;
            dwCreationDisposition = CREATE_ALWAYS;
            break;
        default:
            throw ::std::invalid_argument("Unknown FileOpenMode");
        }

        hFile.reset(CreateFileW(path.c_str(), dwDesiredAccess, dwShareMode, nullptr, dwCreationDisposition, dwFlagsAndAttributes, nullptr));

        THROW_LAST_ERROR_IF(!hFile);

        if (mode == FileOpenMode::Write && preallocationSize > 0)
        {
            FILE_ALLOCATION_INFO allocationInfo = {};
            allocationInfo.AllocationSize.QuadPart = preallocationSize;

            if (!SetFileInformationByHandle(
                hFile.get(),
                FileAllocationInfo,
                &allocationInfo,
                sizeof(allocationInfo)))
            {
                DWORD lastError = GetLastError();

                // As preallocating the file is a performance optimization, we only throw
                // an exception for errors that indicate there is not enough space.
                // If SetFileInformationByHandle fails for any other reason, we write the
                // file without preallocating it.
                if (lastError == ERROR_DISK_FULL || lastError == ERROR_FILE_TOO_LARGE)
                {
                    // Throw the exception after closing the file handle.
                    hFile.reset();
                    THROW_WIN32(lastError);
                }
            }
        }
    }

    ~FileHandleWin() override
    {
        // Flush the write buffer to disk, unless the stack is being unwound due to an uncaught exception.
        if (uncaughtExceptionCount == ::std::uncaught_exceptions())
        {
            if (buffer->writeOffset > 0)
            {
                WriteCore(hFile.get(), buffer->buffer.data(), buffer->writeOffset);
                buffer->writeOffset = 0;
            }
        }
    }

    FileHandleWin(const FileHandleWin&) = delete;
    FileHandleWin& operator=(const FileHandleWin&) = delete;

    FileHandleWin(FileHandleWin&&) = delete;
    FileHandleWin& operator=(FileHandleWin&&) = delete;

    HANDLE get_native_handle() const noexcept
    {
        return hFile.get();
    }

    FileBuffer* get_buffer() const noexcept
    {
        return buffer.get();
    }

private:
    ::std::unique_ptr<FileBuffer> buffer;
    const int uncaughtExceptionCount;
    wil::unique_hfile hFile;
};

::std::unique_ptr<FileHandle> OpenFileNative(
    const boost::filesystem::path& path,
    FileOpenMode mode,
    int64 preallocationSize)
{
    try
    {
        return ::std::make_unique<FileHandleWin>(path, mode, preallocationSize);
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw ::std::bad_alloc();
        }
        else
        {
            throw ::std::runtime_error(e.what());
        }
    }
}

void ReadFileNative(FileHandle* fileHandle, void* data, size_t dataSize)
{
    if (!fileHandle)
    {
        throw ::std::runtime_error("Null file handle");
    }

    FileHandleWin* fileHandleWin = static_cast<FileHandleWin*>(fileHandle);

    HANDLE hFile = fileHandleWin->get_native_handle();
    FileBuffer* buffer = fileHandleWin->get_buffer();

    if (buffer->writeOffset > 0)
    {
        // Flush the buffered data.
        WriteCore(hFile, buffer->buffer.data(), buffer->writeOffset);
        buffer->writeOffset = 0;
    }

    BYTE* output = static_cast<BYTE*>(data);
    size_t outputOffset = 0;
    size_t outputBytesRemaining = dataSize;

    if (buffer->readLength == 0)
    {
        if (dataSize >= buffer->buffer.size())
        {
            ReadCoreBlocking(hFile, output, dataSize);
            // Invalidate the existing buffer.
            buffer->readOffset = 0;
            buffer->readLength = 0;
            return;
        }
        else
        {
            DWORD bytesRead = ReadCore(hFile, buffer->buffer.data(), buffer->buffer.size());

            if (bytesRead == 0)
            {
                throw ::std::runtime_error("Attempted to read beyond the end of the file.");
            }

            buffer->readOffset = 0;
            buffer->readLength = bytesRead;
        }
    }

    size_t bytesToRead = buffer->readLength - buffer->readOffset;

    if (bytesToRead > outputBytesRemaining)
    {
        bytesToRead = outputBytesRemaining;
    }

    memcpy(output + outputOffset, buffer->buffer.data() + buffer->readOffset, bytesToRead);
    buffer->readOffset += bytesToRead;
    outputOffset += bytesToRead;
    outputBytesRemaining -= bytesToRead;

    if (outputBytesRemaining > 0)
    {
        // If we have read all of the data in the buffer, read the remaining bytes directly from the file.
        ReadCoreBlocking(hFile, output + outputOffset, outputBytesRemaining);
        // Invalidate the existing buffer.
        buffer->readOffset = 0;
        buffer->readLength = 0;
    }
}

int64 GetFilePositionNative(FileHandle* fileHandle)
{
    int64 result;

    FileHandleWin* fileHandleWin = static_cast<FileHandleWin*>(fileHandle);

    HANDLE hFile = fileHandleWin->get_native_handle();
    FileBuffer* buffer = fileHandleWin->get_buffer();

    try
    {
        LARGE_INTEGER distanceToMove = {};

        distanceToMove.QuadPart = 0;

        LARGE_INTEGER currentFilePosition;

        THROW_IF_WIN32_BOOL_FALSE(SetFilePointerEx(
            hFile,
            distanceToMove,
            &currentFilePosition,
            FILE_CURRENT));

        // Adjust the OS file position to account for any buffered data.
        // When reading from a file we subtract the read buffer position from OS file position.
        // When writing to a file we add any data that is in the buffer.
        result = currentFilePosition.QuadPart + (buffer->readOffset - buffer->readLength + buffer->writeOffset);
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw ::std::bad_alloc();
        }
        else
        {
            throw ::std::runtime_error(e.what());
        }
    }

    return result;
}

void SetFilePositionNative(FileHandle* fileHandle, int64 posOffset)
{
    if (!fileHandle)
    {
        throw ::std::runtime_error("Null file handle");
    }

    FileHandleWin* fileHandleWin = static_cast<FileHandleWin*>(fileHandle);

    HANDLE hFile = fileHandleWin->get_native_handle();
    FileBuffer* buffer = fileHandleWin->get_buffer();

    // Invalidate the existing read buffer and flush the write buffer.
    buffer->readOffset = 0;
    buffer->readLength = 0;

    if (buffer->writeOffset > 0)
    {
        WriteCore(hFile, buffer->buffer.data(), buffer->writeOffset);
        buffer->writeOffset = 0;
    }

    try
    {
        LARGE_INTEGER distanceToMove = {};

        distanceToMove.QuadPart = posOffset;

        THROW_IF_WIN32_BOOL_FALSE(SetFilePointerEx(
            hFile,
            distanceToMove,
            nullptr,
            FILE_BEGIN));
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw ::std::bad_alloc();
        }
        else
        {
            throw ::std::runtime_error(e.what());
        }
    }
}

void WriteFileNative(FileHandle* fileHandle, const void* data, size_t dataSize)
{
    if (!fileHandle)
    {
        throw ::std::runtime_error("Null file handle");
    }

    const BYTE* input = static_cast<const BYTE*>(data);
    size_t inputOffset = 0;
    size_t inputLength = dataSize;

    FileHandleWin* fileHandleWin = static_cast<FileHandleWin*>(fileHandle);

    HANDLE hFile = fileHandleWin->get_native_handle();
    FileBuffer* buffer = fileHandleWin->get_buffer();

    if (buffer->readLength > 0)
    {
        // Invalidate the read buffer.
        buffer->readOffset = buffer->readLength = 0;
    }

    if (buffer->writeOffset > 0)
    {
        size_t remainingSpaceInBuffer = buffer->buffer.size() - buffer->writeOffset;

        if (remainingSpaceInBuffer > 0)
        {
            size_t bytesToCopy = remainingSpaceInBuffer;

            if (inputLength < bytesToCopy)
            {
                bytesToCopy = inputLength;
            }

            memcpy(buffer->buffer.data() + buffer->writeOffset, input, bytesToCopy);
            buffer->writeOffset += bytesToCopy;

            if (bytesToCopy == inputLength)
            {
                return;
            }

            inputOffset += bytesToCopy;
            inputLength -= bytesToCopy;
        }

        WriteCore(hFile, buffer->buffer.data(), buffer->buffer.size());
        buffer->writeOffset = 0;
    }

    if (inputLength >= buffer->buffer.size())
    {
        WriteCore(hFile, input + inputOffset, inputLength);
    }
    else
    {
        memcpy(buffer->buffer.data(), input + inputOffset, inputLength);
        buffer->writeOffset = inputLength;
    }
}
