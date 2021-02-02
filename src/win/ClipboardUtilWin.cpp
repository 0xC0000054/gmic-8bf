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

#include "ClipboardUtilWin.h"
#include "FileUtil.h"
#include "ImageConversionWin.h"
#include <vector>
#include <boost/algorithm/string.hpp>

namespace
{
    std::vector<UINT> GetAvailableClipboardFormats()
    {
        std::vector<UINT> formats;

        UINT format = EnumClipboardFormats(0);

        while (format != 0)
        {
            formats.push_back(format);

            format = EnumClipboardFormats(format);
        }

        return formats;
    }

    OSErr TryGetFileDropPath(std::wstring& path)
    {
        OSErr err = noErr;

        HANDLE hGlobal = GetClipboardData(CF_HDROP);

        if (hGlobal != nullptr)
        {
            HDROP hDrop = static_cast<HDROP>(GlobalLock(hGlobal));

            if (hDrop != nullptr)
            {
                try
                {
                    UINT requiredLength = DragQueryFileW(hDrop, 0, nullptr, 0) + 1;

                    if (requiredLength > 1)
                    {
                        path = std::wstring(static_cast<size_t>(requiredLength), '\0');

                        if (DragQueryFileW(hDrop, 0, &path[0], requiredLength) > 0)
                        {
                            // Remove the NUL-terminator from the end of the string.
                            path.resize(static_cast<size_t>(requiredLength) - 1);
                        }
                        else
                        {
                            err = ioErr;

                        }
                    }
                    else
                    {
                        err = ioErr;
                    }
                }
                catch (const std::bad_alloc&)
                {
                    err = memFullErr;
                }
                catch (...)
                {
                    err = ioErr;
                }

                GlobalUnlock(hGlobal);
            }
            else
            {
                err = ioErr;
            }
        }
        else
        {
            err = ioErr;
        }

        return err;
    }

    bool FileDropIsImage(const std::wstring& path)
    {
        static const std::vector<std::wstring> fileExtensions
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

    OSErr ProcessFileDrop(
        const FilterRecordPtr filterRecord,
        const std::wstring& path,
        const boost::filesystem::path& gmicInputPath)
    {
        return ConvertImageToGmicInputFormatNative(filterRecord, path, gmicInputPath);
    }

    OSErr ProcessDib(
        const FilterRecordPtr filterRecord,
        UINT format,
        const boost::filesystem::path& gmicInputPath)
    {
        HANDLE hGlobal = GetClipboardData(format);

        if (hGlobal == nullptr)
        {
            return ioErr;
        }

        const SIZE_T handleSize = GlobalSize(hGlobal);

        if (handleSize < sizeof(BITMAPINFOHEADER))
        {
            return ioErr;
        }

        OSErr err = noErr;

        PVOID data = GlobalLock(hGlobal);

        if (data != nullptr)
        {
            try
            {
                PBITMAPINFOHEADER pbih = static_cast<PBITMAPINFOHEADER>(data);

                uint64_t imageDataSize = static_cast<uint64_t>(pbih->biSizeImage);

                if (imageDataSize == 0)
                {
                    if (pbih->biCompression != BI_RGB)
                    {
                        err = ioErr;
                    }
                    else
                    {
                        uint64_t stride = ((static_cast<uint64_t>(pbih->biWidth) * pbih->biBitCount + 31) & ~31) / 8;
                        uint64_t height = pbih->biHeight < 0 ? -pbih->biHeight : pbih->biHeight;

                        imageDataSize = stride * height;
                    }
                }

                if (err == noErr)
                {
                    const uint64_t dibSize = static_cast<uint64_t>(pbih->biSize) +
                        static_cast<uint64_t>(pbih->biClrUsed) * sizeof(RGBQUAD) +
                        imageDataSize;

                    if (dibSize > handleSize)
                    {
                        err = ioErr;
                    }
                    else
                    {
                        // Compute the size of the entire file.
                        const uint64_t fileSize = sizeof(BITMAPFILEHEADER) + dibSize;

                        if (fileSize > std::numeric_limits<DWORD>::max())
                        {
                            err = ioErr;
                        }
                        else
                        {
                            std::vector<BYTE> memoryBmp(static_cast<size_t>(fileSize));

                            BITMAPFILEHEADER* bfh = reinterpret_cast<BITMAPFILEHEADER*>(memoryBmp.data());
                            bfh->bfType = 0x4d42; // 0x42 = "B" 0x4d = "M"
                            bfh->bfSize = static_cast<DWORD>(fileSize);
                            bfh->bfReserved1 = 0;
                            bfh->bfReserved2 = 0;
                            bfh->bfOffBits = sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD);

                            BYTE* dst = memoryBmp.data() + sizeof(BITMAPFILEHEADER);

                            std::memcpy(dst, data, static_cast<size_t>(dibSize));

                            err = ConvertImageToGmicInputFormatNative(
                                filterRecord,
                                memoryBmp.data(),
                                memoryBmp.size(),
                                gmicInputPath);
                        }
                    }
                }
            }
            catch (const std::bad_alloc&)
            {
                err = memFullErr;
            }
            catch (...)
            {
                err = ioErr;
            }

            GlobalUnlock(hGlobal);
        }
        else
        {
            err = ioErr;
        }

        return err;
    }

    OSErr ProcessPng(
        const FilterRecordPtr filterRecord,
        UINT format,
        const boost::filesystem::path& gmicInputPath)
    {
        HANDLE hGlobal = GetClipboardData(format);

        if (hGlobal == nullptr)
        {
            return ioErr;
        }

        const SIZE_T handleSize = GlobalSize(hGlobal);

        OSErr err = noErr;

        PVOID data = GlobalLock(hGlobal);

        if (data != nullptr)
        {
            try
            {
                err = ConvertImageToGmicInputFormatNative(
                    filterRecord,
                    data,
                    handleSize,
                    gmicInputPath);
            }
            catch (const std::bad_alloc&)
            {
                err = memFullErr;
            }
            catch (...)
            {
                err = ioErr;
            }

            GlobalUnlock(hGlobal);
        }
        else
        {
            err = ioErr;
        }

        return err;
    }

    OSErr TryProcessClipboardImage(
        const FilterRecordPtr filterRecord,
        const std::vector<UINT>& availableFormats,
        const boost::filesystem::path& gmicInputPath)
    {
        static const UINT pngFormatId = RegisterClipboardFormatW(L"PNG");
        static const UINT pngMimeFormatId = RegisterClipboardFormatW(L"image/png"); // Used by Qt-based applications

        OSErr err = noErr;

        for (const auto& format : availableFormats)
        {
            // Pick the first format in the list that we support.
            if (format == CF_HDROP)
            {
                // Web Browsers often download the image and place a link on the clipboard.

                std::wstring path;
                if (TryGetFileDropPath(path) == noErr && FileDropIsImage(path))
                {
                    err = ProcessFileDrop(filterRecord, path, gmicInputPath);

                    if (err == noErr)
                    {
                        break;
                    }
                }
            }
            else if (format == CF_DIB || format == CF_DIBV5)
            {
                err = ProcessDib(filterRecord, format, gmicInputPath);

                if (err == noErr)
                {
                    break;
                }
            }
            else if (format == pngFormatId || format == pngMimeFormatId)
            {
                err = ProcessPng(filterRecord, format, gmicInputPath);

                if (err == noErr)
                {
                    break;
                }
            }
        }

        return err;
    }

#if DEBUG_BUILD
#include <map>

    void DumpClipboardFormats(const std::vector<UINT>& availableFormats)
    {
        static std::map<UINT, std::string> predefinedFormats
        {
            { CF_TEXT, "CF_TEXT" },
            { CF_BITMAP, "CF_BITMAP" },
            { CF_METAFILEPICT, "CF_METAFILEPICT" },
            { CF_SYLK, "CF_SYLK" },
            { CF_DIF, "CF_DIF" },
            { CF_TIFF, "CF_TIFF" },
            { CF_OEMTEXT, "CF_OEMTEXT" },
            { CF_DIB, "CF_DIB" },
            { CF_PALETTE, "CF_PALETTE" },
            { CF_PENDATA, "CF_PENDATA" },
            { CF_RIFF, "CF_RIFF" },
            { CF_WAVE, "CF_WAVE" },
            { CF_UNICODETEXT, "CF_UNICODETEXT" },
            { CF_ENHMETAFILE, "CF_ENHMETAFILE" },
            { CF_HDROP, "CF_HDROP" },
            { CF_LOCALE, "CF_LOCALE" },
            { CF_DIBV5, "CF_DIBV5" }
        };

        DebugOut("The clipboard contains %zd formats.", availableFormats.size());

        constexpr int formatNameBufferLength = 256;
        char formatNameBuffer[formatNameBufferLength]{};

        for (auto& format : availableFormats)
        {
            const auto& predefinedItem = predefinedFormats.find(format);

            if (predefinedItem != predefinedFormats.end())
            {
                DebugOut("Predefined format %u (%s)", format, predefinedItem->second.c_str());
            }
            else
            {
                if (GetClipboardFormatNameA(format, formatNameBuffer, formatNameBufferLength) > 0)
                {
                    DebugOut("Registered format %u (%s)", format, formatNameBuffer);
                }
                else
                {
                    if (format >= CF_PRIVATEFIRST && format <= CF_PRIVATELAST)
                    {
                        DebugOut("Private format %u", format);
                    }
                    else
                    {
                        DebugOut("Unknown format %u", format);
                    }
                }
            }
        }
    }
#endif // DEBUG_BUILD
}

OSErr ConvertClipboardImageToGmicInputNative(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& gmicInputPath)
{
    OSErr err = noErr;

    // Failure to open the clipboard is not a fatal error, if it cannot be opened
    // G'MIC will only get one input image.
    // The same thing that would happen if the clipboard does not contain an image.
    if (OpenClipboard(nullptr))
    {
        try
        {
            const std::vector<UINT>& availableFormats = GetAvailableClipboardFormats();

#if DEBUG_BUILD
            DumpClipboardFormats(availableFormats);
#endif // DEBUG_BUILD

            err = TryProcessClipboardImage(filterRecord, availableFormats, gmicInputPath);
        }
        catch (const std::bad_alloc&)
        {
            err = memFullErr;
        }
        catch (...)
        {
            err = ioErr;
        }

        CloseClipboard();
    }

    return err;
}
