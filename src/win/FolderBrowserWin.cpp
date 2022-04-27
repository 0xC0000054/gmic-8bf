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

#include "FolderBrowserWin.h"
#include "resource.h"
#include <windows.h>
#include <Uxtheme.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <new>

namespace
{
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

    OSErr BrowseForFolderVista(
        HWND owner,
        UINT titleResourceId,
        const GUID& ClientGuid,
        boost::filesystem::path& outputFolderPath)
    {
        OSErr err = noErr;

        try
        {
            wchar_t titleBuffer[256] = {};

            if (LoadStringW(wil::GetModuleInstanceHandle(), titleResourceId, titleBuffer, _countof(titleBuffer)) > 0)
            {
                auto comCleanup = wil::CoInitializeEx(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

                auto pfd = wil::CoCreateInstance<IFileOpenDialog>(CLSID_FileOpenDialog);

                DWORD dwOptions;
                THROW_IF_FAILED(pfd->GetOptions(&dwOptions));

                THROW_IF_FAILED(pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_DONTADDTORECENT | FOS_FORCEFILESYSTEM));
                THROW_IF_FAILED(pfd->SetTitle(titleBuffer));
                THROW_IF_FAILED(pfd->SetClientGuid(ClientGuid));

                THROW_IF_FAILED(pfd->Show(owner));

                wil::com_ptr<IShellItem> psi;
                THROW_IF_FAILED(pfd->GetResult(&psi));

                wil::unique_cotaskmem_string pszPath;
                THROW_IF_FAILED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath));

                outputFolderPath = pszPath.get();
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

    using unique_oleuninitialize_call = wil::unique_call<decltype(&OleUninitialize), OleUninitialize>;

    unique_oleuninitialize_call InitializeOLE()
    {
        THROW_IF_FAILED(OleInitialize(nullptr));
        return unique_oleuninitialize_call();
    }

    OSErr BrowseForFolderClassic(HWND owner, UINT descriptionResourceId, boost::filesystem::path& outputFolderPath)
    {
        OSErr err = noErr;

        wchar_t titleBuffer[256] = {};

        if (LoadStringW(wil::GetModuleInstanceHandle(), descriptionResourceId, titleBuffer, _countof(titleBuffer)) > 0)
        {
            try
            {
                auto oleCleanup = InitializeOLE();

                BROWSEINFOW bi = {};
                bi.hwndOwner = owner;
                bi.lpszTitle = titleBuffer;
                bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;

                LPITEMIDLIST pIDL = SHBrowseForFolderW(&bi);

                if (pIDL != nullptr)
                {
                    auto idlCleanup = wil::scope_exit([&]
                    {
                        CoTaskMemFree(pIDL);
                    });

                    wchar_t pathBuffer[MAX_PATH + 1] = {};

                    if (SHGetPathFromIDListW(pIDL, pathBuffer))
                    {
                        outputFolderPath = pathBuffer;
                    }
                    else
                    {
                        err = ioErr;
                    }
                }
                else
                {
                    err = userCanceledErr;
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
                default:
                    err = ioErr;
                    break;
                }
            }
            catch (...)
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
}

OSErr GetGmicOutputFolderNative(FilterRecordPtr filterRecord, boost::filesystem::path& outputFolderPath)
{
    PlatformData* platformData = static_cast<PlatformData*>(filterRecord->platformData);

    HWND owner = platformData != nullptr ? reinterpret_cast<HWND>(platformData->hwnd) : nullptr;

    if (UseVistaStyleDialogs())
    {
        // The client GUID is used to allow this dialog to persist its state independently of the other file dialogs in
        // the host application.
        // {1F3E21BC-4678-404A-83FC-9442259DCF16}
        static const GUID ClientGuid = { 0x1f3e21bc, 0x4678, 0x404a, { 0x83, 0xfc, 0x94, 0x42, 0x25, 0x9d, 0xcf, 0x16 } };

        return BrowseForFolderVista(owner, OUTPUT_FOLDER_PICKER_TITLE, ClientGuid, outputFolderPath);
    }
    else
    {
        return BrowseForFolderClassic(owner, OUTPUT_FOLDER_PICKER_DESCRIPTION, outputFolderPath);
    }
}

OSErr GetDefaultGmicOutputFolderNative(intptr_t parentWindowHandle, boost::filesystem::path& outputFolderPath)
{
    HWND owner = reinterpret_cast<HWND>(parentWindowHandle);

    if (UseVistaStyleDialogs())
    {
        // The client GUID is used to allow this dialog to persist its state independently of the other file dialogs in
        // the host application.
        // {FE2705B8-0D02-45CF-A3FB-C227E0328C00}
        static const GUID ClientGuid = { 0xfe2705b8, 0xd02, 0x45cf, { 0xa3, 0xfb, 0xc2, 0x27, 0xe0, 0x32, 0x8c, 0x0 } };

        return BrowseForFolderVista(owner, DEFAULT_OUTPUT_FOLDER_PICKER_TITLE, ClientGuid, outputFolderPath);
    }
    else
    {
        return BrowseForFolderClassic(owner, DEFAULT_OUTPUT_FOLDER_PICKER_DESCRIPTION, outputFolderPath);
    }
}
