////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020 Nicholas Hayes
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

    OSErr BrowseForFolderVista(HWND owner, boost::filesystem::path& outputFolderPath)
    {
        // The client GUID is used to allow this dialog to persist its state independently of the other file dialogs in
        // the host application.
        // {1F3E21BC-4678-404A-83FC-9442259DCF16}
        static const GUID ClientGuid = { 0x1f3e21bc, 0x4678, 0x404a, { 0x83, 0xfc, 0x94, 0x42, 0x25, 0x9d, 0xcf, 0x16 } };

        OSErr err = noErr;

        try
        {
            wchar_t titleBuffer[256] = {};

            if (LoadStringW(wil::GetModuleInstanceHandle(), VISTA_FOLDER_PICKER_TITLE, titleBuffer, _countof(titleBuffer)) > 0)
            {
                if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
                {
                    wil::com_ptr_nothrow<IFileDialog> pfd;
                    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
                    {
                        DWORD dwOptions;
                        if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
                        {
                            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_DONTADDTORECENT | FOS_FORCEFILESYSTEM);
                            pfd->SetTitle(titleBuffer);
                            pfd->SetClientGuid(ClientGuid);

                            HRESULT hr = pfd->Show(owner);
                            if (SUCCEEDED(hr))
                            {
                                wil::com_ptr_nothrow<IShellItem> psi;
                                if (SUCCEEDED(pfd->GetResult(&psi)))
                                {
                                    wil::unique_cotaskmem_string pszPath;
                                    if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
                                    {
                                        outputFolderPath = pszPath.get();
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
                            else
                            {
                                err = hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) ? userCanceledErr : ioErr;
                            }
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

                    CoUninitialize();
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

        return err;
    }

    OSErr BrowseForFolderClassic(HWND owner, boost::filesystem::path& outputFolderPath)
    {
        OSErr err = noErr;

        wchar_t titleBuffer[256] = {};

        if (LoadStringW(wil::GetModuleInstanceHandle(), CLASSIC_FOLDER_PICKER_DESCRIPTION, titleBuffer, _countof(titleBuffer)) > 0)
        {
            if (SUCCEEDED(OleInitialize(nullptr)))
            {
                BROWSEINFOW bi = {};
                bi.hwndOwner = owner;
                bi.lpszTitle = titleBuffer;
                bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;

                LPITEMIDLIST pIDL = SHBrowseForFolderW(&bi);

                if (pIDL != nullptr)
                {
                    wchar_t pathBuffer[MAX_PATH + 1] = {};

                    if (SHGetPathFromIDListW(pIDL, pathBuffer))
                    {
                        outputFolderPath = pathBuffer;
                    }
                    else
                    {
                        err = ioErr;
                    }

                    CoTaskMemFree(pIDL);
                }
                else
                {
                    err = userCanceledErr;
                }


                OleUninitialize();
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
}

OSErr GetGmicOutputFolderNative(FilterRecordPtr filterRecord, boost::filesystem::path& outputFolderPath)
{
    PlatformData* platformData = static_cast<PlatformData*>(filterRecord->platformData);

    HWND owner = platformData != nullptr ? reinterpret_cast<HWND>(platformData->hwnd) : nullptr;

    if (UseVistaStyleDialogs())
    {
        return BrowseForFolderVista(owner, outputFolderPath);
    }
    else
    {
        return BrowseForFolderClassic(owner, outputFolderPath);
    }
}
