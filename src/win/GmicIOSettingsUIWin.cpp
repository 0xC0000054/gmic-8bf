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

#include "GmicIOSettingsPlugin.h"
#include "CommonUIWin.h"
#include "FolderBrowser.h"
#include "resource.h"
#include <objbase.h>
#include <ShlObj.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/win32_helpers.h>
#include <windowsx.h>
#include <vector>

namespace
{
    struct DialogData
    {
        boost::filesystem::path defaultOutputFolder;
        OSErr dialogError;

        DialogData(const boost::filesystem::path& path) : defaultOutputFolder(path), dialogError(noErr)
        {
        }
    };

    void InitIOSettingsDialog(HWND hDlg, const boost::filesystem::path& defaultOutputFolder)
    {
        const HWND defaultOutputFolderCheckBox = GetDlgItem(hDlg, IDC_DEFAULTOUTDIRCB);
        const HWND editBoxHWnd = GetDlgItem(hDlg, IDC_DEFAULTOUTDIREDIT);
        const HWND browseButtonHWnd = GetDlgItem(hDlg, IDC_DEFAULTOUTFOLDERBROWSE);

        if (defaultOutputFolder.empty())
        {
            Button_SetCheck(defaultOutputFolderCheckBox, BST_UNCHECKED);
            EnableWindow(editBoxHWnd, FALSE);
            EnableWindow(browseButtonHWnd, FALSE);

            try
            {
                wil::unique_cotaskmem_string picturesFolderPath;

                THROW_IF_FAILED(SHGetKnownFolderPath(FOLDERID_Pictures, 0, nullptr, &picturesFolderPath));

                boost::filesystem::path defaultPath(picturesFolderPath.get());
                defaultPath /= "G'MIC-Qt";

                SetWindowTextW(editBoxHWnd, defaultPath.c_str());
            }
            catch (...)
            {
                // Ignore any exceptions that occur when setting the default value.
            }
        }
        else
        {
            Button_SetCheck(defaultOutputFolderCheckBox, BST_CHECKED);
            EnableWindow(editBoxHWnd, TRUE);
            EnableWindow(browseButtonHWnd, TRUE);
            SetWindowTextW(editBoxHWnd, defaultOutputFolder.c_str());
        }
    }

    void WriteSettings(HWND hDlg, DialogData* data)
    {
        bool defaultFolderChecked = Button_GetCheck(GetDlgItem(hDlg, IDC_DEFAULTOUTDIRCB)) == BST_CHECKED;

        if (defaultFolderChecked)
        {
            const HWND editBoxHWnd = GetDlgItem(hDlg, IDC_DEFAULTOUTDIREDIT);

            const int pathLength = GetWindowTextLengthW(editBoxHWnd);

            if (pathLength == 0)
            {
                return;
            }

            try
            {
                // Ensure that adding the NUL terminator will not cause an integer overflow.
                if (pathLength < std::numeric_limits<int>::max())
                {
                    const int lengthWithTerminator = pathLength + 1;

                    std::vector<wchar_t> temp(lengthWithTerminator);

                    if (GetWindowTextW(editBoxHWnd, &temp[0], lengthWithTerminator) > 0)
                    {
                        data->defaultOutputFolder = temp.data();
                    }
                }
                else
                {
                    data->dialogError = memFullErr;
                }
            }
            catch (const std::bad_alloc&)
            {
                data->dialogError = memFullErr;
            }
            catch (...)
            {
                data->dialogError = ioErr;
            }
        }
    }

    void EnableOuputDirItems(HWND hDlg, bool enable)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_DEFAULTOUTDIREDIT), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_DEFAULTOUTFOLDERBROWSE), enable);
    }

    INT_PTR CALLBACK IOSettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam) noexcept
    {
        DialogData* dialogParams = nullptr;

        if (wMsg == WM_INITDIALOG)
        {
            dialogParams = reinterpret_cast<DialogData*>(lParam);
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialogParams));
        }
        else
        {
            dialogParams = reinterpret_cast<DialogData*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
        }

        int item;
        int cmd;
        switch (wMsg)
        {
        case WM_INITDIALOG:
            CenterDialog(hDlg);
            InitIOSettingsDialog(hDlg, dialogParams->defaultOutputFolder);
            break;
        case WM_COMMAND:
            item = LOWORD(wParam);
            cmd = HIWORD(wParam);

            if (cmd == BN_CLICKED)
            {
                boost::filesystem::path newFolderPath;

                switch (item)
                {
                case IDOK:
                    WriteSettings(hDlg, dialogParams);
                    EndDialog(hDlg, item);
                    break;
                case IDCANCEL:
                    EndDialog(hDlg, item);
                    break;
                case IDC_DEFAULTOUTDIRCB:
                    EnableOuputDirItems(hDlg, Button_GetCheck(GetDlgItem(hDlg, IDC_DEFAULTOUTDIRCB)) == BST_CHECKED);
                    break;
                case IDC_DEFAULTOUTFOLDERBROWSE:

                    if (GetDefaultGmicOutputFolder(reinterpret_cast<intptr_t>(hDlg), newFolderPath) == noErr)
                    {
                        SetWindowTextW(GetDlgItem(hDlg, IDC_DEFAULTOUTDIREDIT), newFolderPath.c_str());
                    }

                    break;

                default:
                    return FALSE;
                }
            }
            break;
        default:
            return FALSE;
        }

        return TRUE;
    }
}

OSErr DoIOSettingsUI(const FilterRecordPtr filterRecord, GmicIOSettings& settings)
{
    OSErr err = noErr;

    PlatformData* platform = static_cast<PlatformData*>(filterRecord->platformData);

    HWND parent = platform != nullptr ? reinterpret_cast<HWND>(platform->hwnd) : nullptr;

    DialogData dialogData(settings.GetDefaultOutputPath());

    try
    {
        LPARAM lParams = reinterpret_cast<LPARAM>(&dialogData);
        if (DialogBoxParam(
            wil::GetModuleInstanceHandle(),
            MAKEINTRESOURCE(IDD_OUTPUTSETTINGS),
            parent,
            IOSettingsDlgProc,
            lParams) == IDOK)
        {
            if (dialogData.dialogError == noErr)
            {
                settings.SetDefaultOutputPath(dialogData.defaultOutputFolder);
            }
            else
            {
                err = dialogData.dialogError;
            }
        }
        else
        {
            err = userCanceledErr;
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
