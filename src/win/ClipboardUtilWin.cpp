////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClipboardUtilWin.h"
#include "FileUtil.h"
#include "ImageConversionWin.h"
#include <boost/algorithm/string.hpp>
#include <boost/core/noncopyable.hpp>
#include <shellapi.h>

namespace
{
    struct unique_hglobal_lock : private boost::noncopyable
    {
        explicit unique_hglobal_lock(HGLOBAL global) noexcept
            : lockedMemory(GlobalLock(global))
        {
        }

        ~unique_hglobal_lock()
        {
            if (lockedMemory != nullptr)
            {
                GlobalUnlock(lockedMemory);
                lockedMemory = nullptr;
            }
        }

        void* get() const noexcept
        {
            return lockedMemory;
        }

        operator bool() const
        {
            return lockedMemory != nullptr;
        }

    private:
        void* lockedMemory;
    };

    ::std::wstring GetFileDropPath()
    {
        ::std::wstring path;

        HANDLE hGlobal = GetClipboardData(CF_HDROP);

        if (hGlobal == nullptr)
        {
            throw ::std::runtime_error("GetClipboardData returned NULL.");
        }

        unique_hglobal_lock lockedData(hGlobal);

        if (!lockedData)
        {
            throw ::std::runtime_error("Unable to lock the clipboard data handle.");
        }

        HDROP hDrop = static_cast<HDROP>(lockedData.get());

        UINT requiredLength = DragQueryFileW(hDrop, 0, nullptr, 0) + 1;

        if (requiredLength > 1)
        {
            path = ::std::wstring(static_cast<size_t>(requiredLength), '\0');

            if (DragQueryFileW(hDrop, 0, &path[0], requiredLength) == 0)
            {
                throw ::std::runtime_error("Unable to get the file path from the clipboard.");
            }

            // Remove the NUL-terminator from the end of the string.
            path.resize(static_cast<size_t>(requiredLength) - 1);
        }

        return path;
    }

    bool FileDropIsImage(const ::std::wstring& path)
    {
        static const ::std::vector<::std::wstring> fileExtensions
        {
            L".bmp",
            L".png",
            L".jpg",
            L".jpe",
            L".jpeg",
            L".jfif",
            L".gif",
            L".tif",
            L".tiff"
        };

        for (const auto& ext : fileExtensions)
        {
            if (boost::algorithm::iends_with(path, ext))
            {
                return true;
            }
        }

        return false;
    }

    void ProcessFileDrop(
        const ::std::wstring& path,
        ::std::unique_ptr<InputLayerInfo>& layer)
    {
        ConvertImageToGmicInputFormatNative(path, layer);
    }

    void ProcessDib(
        UINT format,
        ::std::unique_ptr<InputLayerInfo>& layer)
    {
        HANDLE hGlobal = GetClipboardData(format);

        if (hGlobal == nullptr)
        {
            throw ::std::runtime_error("GetClipboardData returned NULL.");
        }

        const SIZE_T handleSize = GlobalSize(hGlobal);

        if (handleSize < sizeof(BITMAPINFOHEADER))
        {
            throw ::std::runtime_error("The clipboard handle size is too small for a DIB header.");
        }

        unique_hglobal_lock lockedData(hGlobal);

        if (!lockedData)
        {
            throw ::std::runtime_error("Unable to lock the clipboard data handle.");
        }

        const PBITMAPINFOHEADER pbih = static_cast<PBITMAPINFOHEADER>(lockedData.get());

        uint64_t imageDataSize = static_cast<uint64_t>(pbih->biSizeImage);

        if (imageDataSize == 0)
        {
            if (pbih->biCompression != BI_RGB)
            {
                throw ::std::runtime_error("The DIB compression must be BI_RGB when biSizeImage is 0.");
            }
            else
            {
                uint64_t stride = ((static_cast<uint64_t>(pbih->biWidth) * pbih->biBitCount + 31) & ~31) / 8;
                uint64_t height = pbih->biHeight < 0 ? -pbih->biHeight : pbih->biHeight;

                imageDataSize = stride * height;
            }
        }

        const uint64_t dibSize = static_cast<uint64_t>(pbih->biSize) +
            static_cast<uint64_t>(pbih->biClrUsed) * sizeof(RGBQUAD) +
            imageDataSize;

        if (dibSize > handleSize)
        {
            throw ::std::runtime_error("The calculated DIB size is larger than the clipboard data handle size.");
        }
        else
        {
            // Compute the size of the entire file.
            const uint64_t fileSize = sizeof(BITMAPFILEHEADER) + dibSize;

            if (fileSize > ::std::numeric_limits<DWORD>::max())
            {
                throw ::std::runtime_error("The bitmap file size is larger than 4 GB.");
            }
            else
            {
                ::std::vector<BYTE> memoryBmp(static_cast<size_t>(fileSize));

                BITMAPFILEHEADER* bfh = reinterpret_cast<BITMAPFILEHEADER*>(memoryBmp.data());
                bfh->bfType = 0x4d42; // 0x42 = "B" 0x4d = "M"
                bfh->bfSize = static_cast<DWORD>(fileSize);
                bfh->bfReserved1 = 0;
                bfh->bfReserved2 = 0;
                bfh->bfOffBits = sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD);

                BYTE* dst = memoryBmp.data() + sizeof(BITMAPFILEHEADER);

                ::std::memcpy(dst, lockedData.get(), static_cast<size_t>(dibSize));

                ConvertImageToGmicInputFormatNative(
                    memoryBmp.data(),
                    memoryBmp.size(),
                    layer);
            }
        }
    }

    void ProcessPng(
        UINT format,
        ::std::unique_ptr<InputLayerInfo>& layer)
    {
        HANDLE hGlobal = GetClipboardData(format);

        if (hGlobal == nullptr)
        {
            throw ::std::runtime_error("GetClipboardData returned NULL.");
        }

        const SIZE_T handleSize = GlobalSize(hGlobal);

        unique_hglobal_lock lockedData(hGlobal);

        if (!lockedData)
        {
            throw ::std::runtime_error("Unable to lock the clipboard data handle.");
        }

        ConvertImageToGmicInputFormatNative(
            lockedData.get(),
            handleSize,
            layer);
    }

    struct unique_clipboard : private boost::noncopyable
    {
        explicit unique_clipboard(HWND hwndNewOwner) noexcept
            : openedClipboard(OpenClipboard(hwndNewOwner))
        {
        }

        ~unique_clipboard()
        {
            if (openedClipboard)
            {
                CloseClipboard();
                openedClipboard = false;
            }
        }

        operator bool() const
        {
            return openedClipboard;
        }

    private:
        bool openedClipboard;
    };
}

void ConvertClipboardImageToGmicInputNative(::std::unique_ptr<InputLayerInfo>& layer)
{
    // Failure to open the clipboard is not a fatal error, if it cannot be opened
    // G'MIC will only get one input image.
    // The same thing that would happen if the clipboard does not contain an image.
    unique_clipboard clipboard(nullptr);

    if (clipboard)
    {
        static const UINT pngFormatId = RegisterClipboardFormatW(L"PNG");
        static const UINT pngMimeFormatId = RegisterClipboardFormatW(L"image/png"); // Used by Qt-based applications

        for (UINT format = EnumClipboardFormats(0); format != 0; format = EnumClipboardFormats(format))
        {
            // Pick the first format in the list that we support.
            if (format == CF_HDROP)
            {
                // Web Browsers often download the image and place a link on the clipboard.
                ::std::wstring path = GetFileDropPath();
                if (!path.empty() && FileDropIsImage(path))
                {
                    ProcessFileDrop(path, layer);
                    break;
                }
            }
            else if (format == CF_DIB || format == CF_DIBV5)
            {
                ProcessDib(format, layer);
                break;
            }
            else if (format == pngFormatId || format == pngMimeFormatId)
            {
                ProcessPng(format, layer);
                break;
            }
        }
    }
}
