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

#include "GmicIOSettingsPlugin.h"
#include "CommonUIWin.h"
#include "FolderBrowser.h"
#include "ImageLoadDialog.h"
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
        SecondInputImageSource secondImageSource;
        boost::filesystem::path secondImageFilePath;

        OSErr GetDialogError() const
        {
            return dialogError;
        }

        void SetDialogError(OSErr err)
        {
            if (err != noErr)
            {
                dialogError = err;
            }
        }

        DialogData(const GmicIOSettings& settings)
            : defaultOutputFolder(settings.GetDefaultOutputPath()),
              secondImageSource(settings.GetSecondInputImageSource()),
              secondImageFilePath(settings.GetSecondInputImagePath()),
              dialogError(noErr)
        {
        }
    private:

        OSErr dialogError;
    };

    void InitIOSettingsDialog(HWND hDlg, const DialogData* const data)
    {
        const HWND defaultOutputFolderCheckBox = GetDlgItem(hDlg, IDC_DEFAULTOUTDIRCB);
        const HWND outputFolderEditBox = GetDlgItem(hDlg, IDC_DEFAULTOUTDIREDIT);
        const HWND outputFolderBrowseButton = GetDlgItem(hDlg, IDC_DEFAULTOUTFOLDERBROWSE);

        if (data->defaultOutputFolder.empty())
        {
            Button_SetCheck(defaultOutputFolderCheckBox, BST_UNCHECKED);
            EnableWindow(outputFolderEditBox, FALSE);
            EnableWindow(outputFolderBrowseButton, FALSE);

            try
            {
                wil::unique_cotaskmem_string picturesFolderPath;

                THROW_IF_FAILED(SHGetKnownFolderPath(FOLDERID_Pictures, 0, nullptr, &picturesFolderPath));

                boost::filesystem::path defaultPath(picturesFolderPath.get());
                defaultPath /= "G'MIC-Qt";

                SetWindowTextW(outputFolderEditBox, defaultPath.c_str());
            }
            catch (...)
            {
                // Ignore any exceptions that occur when setting the default value.
            }
        }
        else
        {
            Button_SetCheck(defaultOutputFolderCheckBox, BST_CHECKED);
            EnableWindow(outputFolderEditBox, TRUE);
            EnableWindow(outputFolderBrowseButton, TRUE);
            SetWindowTextW(outputFolderEditBox, data->defaultOutputFolder.c_str());
        }

        int checkedRadioButtonId;

        switch (data->secondImageSource)
        {
        case SecondInputImageSource::Clipboard:
            checkedRadioButtonId = IDC_SECONDIMAGESOURCE_CLIPBOARD_RADIO;
            break;
        case SecondInputImageSource::File:
            checkedRadioButtonId = IDC_SECONDIMAGESOURCE_FILE_RADIO;
        break;
        case SecondInputImageSource::None:
        default:
            checkedRadioButtonId = IDC_SECONDIMAGESOURCE_NONE_RADIO;
            break;
        }

        if (checkedRadioButtonId == IDC_SECONDIMAGESOURCE_FILE_RADIO)
        {
            if (data->secondImageFilePath.empty() ||
                !SetWindowTextW(GetDlgItem(hDlg, IDC_SECONDIMAGEPATHEDIT), data->secondImageFilePath.c_str()))
            {
                checkedRadioButtonId = IDC_SECONDIMAGESOURCE_NONE_RADIO;
            }
        }

        CheckRadioButton(
            hDlg,
            IDC_SECONDIMAGESOURCE_NONE_RADIO,
            IDC_SECONDIMAGESOURCE_FILE_RADIO,
            checkedRadioButtonId);
    }

    OSErr GetPathFromTextBox(const HWND editBoxHWnd, boost::filesystem::path& path)
    {
        OSErr err = noErr;

        try
        {
            path = boost::filesystem::path();

            const int pathLength = GetWindowTextLengthW(editBoxHWnd);

            if (pathLength == 0)
            {
                return noErr;
            }

            // Ensure that adding the NUL terminator will not cause an integer overflow.
            if (pathLength < std::numeric_limits<int>::max())
            {
                const int lengthWithTerminator = pathLength + 1;

                std::vector<wchar_t> temp(lengthWithTerminator);

                if (GetWindowTextW(editBoxHWnd, &temp[0], lengthWithTerminator) > 0)
                {
                    path = temp.data();
                }
                else
                {
                    err = ioErr;
                }
            }
            else
            {
                err = memFullErr;
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

    void WriteOutputFolderSettings(HWND hDlg, DialogData* data)
    {
        bool defaultFolderChecked = Button_GetCheck(GetDlgItem(hDlg, IDC_DEFAULTOUTDIRCB)) == BST_CHECKED;

        if (defaultFolderChecked)
        {
            const HWND editBoxHWnd = GetDlgItem(hDlg, IDC_DEFAULTOUTDIREDIT);

            data->SetDialogError(GetPathFromTextBox(editBoxHWnd, data->defaultOutputFolder));
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
            SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(dialogParams));
        }
        else
        {
            dialogParams = reinterpret_cast<DialogData*>(GetWindowLongPtrW(hDlg, DWLP_USER));
        }

        int item;
        int cmd;
        switch (wMsg)
        {
        case WM_INITDIALOG:
            CenterDialog(hDlg);
            InitIOSettingsDialog(hDlg, dialogParams);
            return TRUE;
        case WM_COMMAND:
            item = LOWORD(wParam);
            cmd = HIWORD(wParam);

            if (cmd == BN_CLICKED)
            {
                boost::filesystem::path newPath;

                switch (item)
                {
                case IDOK:
                    WriteOutputFolderSettings(hDlg, dialogParams);
                    EndDialog(hDlg, item);
                    break;
                case IDCANCEL:
                    EndDialog(hDlg, item);
                    break;
                case IDC_DEFAULTOUTDIRCB:
                    EnableOuputDirItems(hDlg, Button_GetCheck(GetDlgItem(hDlg, IDC_DEFAULTOUTDIRCB)) == BST_CHECKED);
                    break;
                case IDC_DEFAULTOUTFOLDERBROWSE:

                    if (GetDefaultGmicOutputFolder(reinterpret_cast<intptr_t>(hDlg), newPath) == noErr)
                    {
                        SetWindowTextW(GetDlgItem(hDlg, IDC_DEFAULTOUTDIREDIT), newPath.c_str());
                    }

                    break;
                case IDC_SECONDIMAGESOURCE_NONE_RADIO:

                    if (Button_GetCheck(GetDlgItem(hDlg, IDC_SECONDIMAGESOURCE_NONE_RADIO)) == BST_CHECKED)
                    {
                        CheckRadioButton(
                            hDlg,
                            IDC_SECONDIMAGESOURCE_NONE_RADIO,
                            IDC_SECONDIMAGESOURCE_FILE_RADIO,
                            item);
                        dialogParams->secondImageSource = SecondInputImageSource::None;
                    }

                    break;
                case IDC_SECONDIMAGESOURCE_CLIPBOARD_RADIO:

                    if (Button_GetCheck(GetDlgItem(hDlg, IDC_SECONDIMAGESOURCE_CLIPBOARD_RADIO)) == BST_CHECKED)
                    {
                        CheckRadioButton(
                            hDlg,
                            IDC_SECONDIMAGESOURCE_NONE_RADIO,
                            IDC_SECONDIMAGESOURCE_FILE_RADIO,
                            item);
                        dialogParams->secondImageSource = SecondInputImageSource::Clipboard;
                    }

                    break;
                case IDC_SECONDIMAGESOURCE_FILE_RADIO:

                    if (Button_GetCheck(GetDlgItem(hDlg, IDC_SECONDIMAGESOURCE_FILE_RADIO)) == BST_CHECKED)
                    {
                        CheckRadioButton(
                            hDlg,
                            IDC_SECONDIMAGESOURCE_NONE_RADIO,
                            IDC_SECONDIMAGESOURCE_FILE_RADIO,
                            item);
                        dialogParams->secondImageSource = SecondInputImageSource::File;
                    }

                    break;

                case IDC_SECONDIMAGEPATHBROWSE:

                    if (GetImageFileName(reinterpret_cast<intptr_t>(hDlg), newPath) == noErr)
                    {
                        SetWindowTextW(GetDlgItem(hDlg, IDC_SECONDIMAGEPATHEDIT), newPath.c_str());
                    }

                    break;
                }
            }
            else if (item == IDC_SECONDIMAGEPATHEDIT)
            {
                if (cmd == EN_CHANGE)
                {
                    const HWND editBoxHwnd = reinterpret_cast<HWND>(lParam);

                    if (GetWindowTextLengthW(editBoxHwnd) > 0)
                    {
                        dialogParams->SetDialogError(GetPathFromTextBox(editBoxHwnd, dialogParams->secondImageFilePath));

                        if (Button_GetCheck(GetDlgItem(hDlg, IDC_SECONDIMAGESOURCE_FILE_RADIO)) == BST_UNCHECKED)
                        {
                            CheckRadioButton(
                                hDlg,
                                IDC_SECONDIMAGESOURCE_NONE_RADIO,
                                IDC_SECONDIMAGESOURCE_FILE_RADIO,
                                IDC_SECONDIMAGESOURCE_FILE_RADIO);
                            dialogParams->secondImageSource = SecondInputImageSource::File;
                        }
                    }
                    else
                    {
                        dialogParams->secondImageFilePath.clear();
                    }
                }
            }
            break;
        }

        return FALSE;
    }
}

OSErr DoIOSettingsUI(const FilterRecordPtr filterRecord, GmicIOSettings& settings)
{
    OSErr err = noErr;

    PlatformData* platform = static_cast<PlatformData*>(filterRecord->platformData);

    HWND parent = platform != nullptr ? reinterpret_cast<HWND>(platform->hwnd) : nullptr;

    DialogData dialogData(settings);

    try
    {
        if (DialogBoxParam(
            wil::GetModuleInstanceHandle(),
            MAKEINTRESOURCE(IDD_OUTPUTSETTINGS),
            parent,
            IOSettingsDlgProc,
            reinterpret_cast<LPARAM>(&dialogData)) == IDOK)
        {
            OSErr dialogError = dialogData.GetDialogError();

            if (dialogError == noErr)
            {
                settings.SetDefaultOutputPath(dialogData.defaultOutputFolder);
                settings.SetSecondInputImageSource(dialogData.secondImageSource);
                settings.SetSecondInputImagePath(dialogData.secondImageFilePath);
            }
            else
            {
                err = dialogError;
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
