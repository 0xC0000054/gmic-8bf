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

#include "stdafx.h"
#include "ImageLoadDialogWin.h"
#include "resource.h"
#include <string>
#include <vector>
#include <commdlg.h>
#include <Uxtheme.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>

namespace
{
    ::std::vector<::std::pair<::std::wstring, ::std::wstring>> GetOpenDialogFilters()
    {
        const ::std::vector<::std::pair<UINT, ::std::wstring>> resourceMap
        {
            { ALL_IMAGES_FILTER_NAME, L"*.bmp;*.dib;*.gif;*.jpg;*.jpeg;*.jpe;*.jfif;*.png;*.rle"},
            { BMP_FILTER_NAME, L"*.bmp;*.dib;*.rle" },
            { GIF_FILTER_NAME, L"*.gif" },
            { JPEG_FILTER_NAME, L"*.jpg;*.jpeg;*.jpe;*.jfif" },
            { PNG_FILTER_NAME, L"*.png" }
        };

        constexpr int filterNameBufferLength = 256;
        wchar_t filterNameBuffer[filterNameBufferLength]{};

        ::std::vector<::std::pair<::std::wstring, ::std::wstring>> filters;
        filters.reserve(resourceMap.size());

        for (auto& resource : resourceMap)
        {
            UINT resourceId = resource.first;
            const ::std::wstring& extFilter = resource.second;

            int stringLength = LoadStringW(wil::GetModuleInstanceHandle(),
                                           resourceId,
                                           filterNameBuffer,
                                           filterNameBufferLength);
            if (stringLength > 0)
            {
                ::std::wstring name(filterNameBuffer, static_cast<size_t>(stringLength));

                filters.push_back(::std::pair<::std::wstring, ::std::wstring>(name, extFilter));
            }
        }

        return filters;
    }

    bool UseVistaStyleDialogs()
    {
        DWORD resultFlags = GetThemeAppProperties();

        if ((resultFlags & STAP_ALLOW_CONTROLS) != 0)
        {
            // Use the classic dialog if the OS is in safe mode.
            return GetSystemMetrics(SM_CLEANBOOT) == 0;
        }

        return false;
    }

    ::std::vector<COMDLG_FILTERSPEC> BulidVistaOpenDialogFilter(const ::std::vector<::std::pair<::std::wstring, ::std::wstring>>& filterItems)
    {
        ::std::vector<COMDLG_FILTERSPEC> fileTypes(filterItems.size());

        for (size_t i = 0; i < fileTypes.size(); i++)
        {
            const ::std::pair<::std::wstring, ::std::wstring>& entry = filterItems[i];
            auto& filter = fileTypes[i];

            filter.pszName = entry.first.c_str();
            filter.pszSpec = entry.second.c_str();
        }

        return fileTypes;
    }

    OSErr GetOpenFileNameVista(
        HWND owner,
        boost::filesystem::path& saveFilePath)
    {
        // The client GUID is used to allow this dialog to persist its state independently of the other file dialogs in
        // the host application.
        // {C6B1C155-B51B-4D46-8D1C-EA0789E580BE}
        static const GUID ClientGuid = { 0xc6b1c155, 0xb51b, 0x4d46, { 0x8d, 0x1c, 0xea, 0x7, 0x89, 0xe5, 0x80, 0xbe } };

        OSErr err = noErr;

        try
        {
            wchar_t titleBuffer[256] = {};

            const int titleLength = LoadStringW(wil::GetModuleInstanceHandle(),
                IMAGE_OPEN_DIALOG_TITLE,
                titleBuffer,
                _countof(titleBuffer));

            if (titleLength > 0)
            {
                auto comCleanup = wil::CoInitializeEx(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

                auto pfd = wil::CoCreateInstance<IFileOpenDialog>(CLSID_FileOpenDialog);

                DWORD dwOptions;
                THROW_IF_FAILED(pfd->GetOptions(&dwOptions));

                THROW_IF_FAILED(pfd->SetOptions(dwOptions | FOS_DONTADDTORECENT | FOS_FORCEFILESYSTEM));
                THROW_IF_FAILED(pfd->SetTitle(titleBuffer));
                THROW_IF_FAILED(pfd->SetClientGuid(ClientGuid));

                const ::std::vector<::std::pair<::std::wstring, ::std::wstring>>& filterItems = GetOpenDialogFilters();
                const ::std::vector<COMDLG_FILTERSPEC>& fileTypes = BulidVistaOpenDialogFilter(filterItems);

                THROW_IF_FAILED(pfd->SetFileTypes(static_cast<UINT>(fileTypes.size()), fileTypes.data()));

                THROW_IF_FAILED(pfd->Show(owner));

                wil::com_ptr<IShellItem> psi;
                THROW_IF_FAILED(pfd->GetResult(&psi));

                wil::unique_cotaskmem_string pszPath;

                THROW_IF_FAILED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath));

                saveFilePath = pszPath.get();
            }
            else
            {
                err = ioErr;
            }
        }
        catch (const ::std::bad_alloc&)
        {
            err = memFullErr;
        }
        catch (const wil::ResultException& e)
        {
            switch (e.GetErrorCode())
            {
            case E_OUTOFMEMORY:
                err = memFullErr;
                break;
            case HRESULT_FROM_WIN32(ERROR_CANCELLED):
                err = userCanceledErr;
                break;
            default:
                err = ioErr;
                break;
            }
        }
        catch (...)
        {
            err = ioErr;
        }

        return err;
    }

    ::std::wstring BulidClassicOpenDialogFilterString(const ::std::vector<::std::pair<::std::wstring, ::std::wstring>>& filterItems)
    {
        ::std::wstring filter;

        for (auto& item : filterItems)
        {
            filter.append(item.first);
            filter.push_back('\0');
            filter.append(item.second);
            filter.push_back('\0');
        }

        filter.push_back('\0');

        return filter;
    }

    OSErr GetOpenFileNameClassic(
        HWND owner,
        boost::filesystem::path& outputFilePath)
    {
        OSErr err = noErr;

        try
        {
            wchar_t titleBuffer[256] = {};

            const int titleLength = LoadStringW(wil::GetModuleInstanceHandle(),
                IMAGE_OPEN_DIALOG_TITLE,
                titleBuffer,
                _countof(titleBuffer));

            if (titleLength > 0)
            {
                auto comCleanup = wil::CoInitializeEx(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

                const ::std::vector<::std::pair<::std::wstring, ::std::wstring>>& filterItems = GetOpenDialogFilters();
                const ::std::wstring& filterStr = BulidClassicOpenDialogFilterString(filterItems);

                constexpr int fileNameBufferLength = 8192;

                auto fileNameBuffer = wil::make_unique_cotaskmem<wchar_t[]>(fileNameBufferLength);

                memset(fileNameBuffer.get(), 0, fileNameBufferLength * sizeof(wchar_t));

                OPENFILENAMEW ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = owner;
                ofn.lpstrTitle = titleBuffer;
                ofn.lpstrFilter = filterStr.c_str();
                ofn.nFilterIndex = 1;
                ofn.lpstrFile = fileNameBuffer.get();
                ofn.nMaxFile = fileNameBufferLength;
                ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

                if (GetOpenFileNameW(&ofn))
                {
                    outputFilePath = fileNameBuffer.get();
                }
                else
                {
                    err = CommDlgExtendedError() == 0 ? userCanceledErr : ioErr;
                }
            }
            else
            {
                err = ioErr;
            }
        }
        catch (const ::std::bad_alloc&)
        {
            err = memFullErr;
        }
        catch (const wil::ResultException& e)
        {
            err = e.GetErrorCode() == E_OUTOFMEMORY ? memFullErr : ioErr;
        }
        catch (...)
        {
            err = ioErr;
        }

        return err;
    }
}

OSErr GetImageFileNameNative(intptr_t parentWindowHandle, boost::filesystem::path& imageFileName)
{
    HWND owner = reinterpret_cast<HWND>(parentWindowHandle);

    if (UseVistaStyleDialogs())
    {
        return GetOpenFileNameVista(owner, imageFileName);
    }
    else
    {
        return GetOpenFileNameClassic(owner, imageFileName);
    }
}
