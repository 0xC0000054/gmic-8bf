////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2026 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GmicPlugin.h"
#include "GmicProcess.h"
#include "CommonUIWin.h"
#include <stdio.h>
#include "resource.h"
#include "version.h"
#include <CommCtrl.h>
#include <process.h>
#include <shellapi.h>
#include <objbase.h> // Required by wil/win32_helpers.h
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <libpng16/png.h>

namespace
{
    void InitAboutDialog(HWND dp) noexcept
    {
        char s[384]{}, format[384]{};

        GetDlgItemTextA(dp, ABOUTFORMAT, format, 384);
        sprintf_s(s, format, VI_VERSION_STR);
        SetDlgItemTextA(dp, ABOUTFORMAT, s);

        memset(&s, 0, sizeof(s));
        memset(&format, 0, sizeof(format));

        GetDlgItemTextA(dp, IDC_LIBPNG, format, 384);
        png_const_charp str = png_get_copyright(nullptr);
        sprintf_s(s, format, str);
        SetDlgItemTextA(dp, IDC_LIBPNG, s);
    }

    INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam) noexcept
    {
        UNREFERENCED_PARAMETER(lParam);

        int cmd;
        switch (wMsg)
        {
        case WM_INITDIALOG:
            CenterDialog(hDlg);
            InitAboutDialog(hDlg);
            return TRUE;
        case WM_LBUTTONUP:
            EndDialog(hDlg, IDOK);
            break;
        case WM_COMMAND:
            cmd = HIWORD(wParam);

            if (cmd == BN_CLICKED)
            {
                EndDialog(hDlg, IDOK);
            }
            break;
        case WM_NOTIFY:
            switch (reinterpret_cast<LPNMHDR>(lParam)->code)
            {
            case NM_CLICK:          // Fall through to the next case.
            case NM_RETURN:
            {
                const PNMLINK pNMLink = reinterpret_cast<const PNMLINK>(lParam);

                if (pNMLink->hdr.idFrom == IDC_GMICQT && (pNMLink->item.iLink == 0))
                {
                    // The item.szUrl field is always empty, so the link has to be hard-coded.
                    ShellExecuteW(nullptr, L"open", L"https://github.com/c-koi/gmic-qt", nullptr, nullptr, SW_SHOW);
                }
                else if (pNMLink->hdr.idFrom == IDC_GMICCORE && (pNMLink->item.iLink == 0))
                {
                    // The item.szUrl field is always empty, so the link has to be hard-coded.
                    ShellExecuteW(nullptr, L"open", L"https://github.com/GreycLab/gmic", nullptr, nullptr, SW_SHOW);
                }
                else if (pNMLink->hdr.idFrom == IDC_LIBPNG && (pNMLink->item.iLink == 0))
                {
                    // The item.szUrl field is always empty, so the link has to be hard-coded.
                    ShellExecuteW(nullptr, L"open", L"http://www.libpng.org/pub/png/libpng.html", nullptr, nullptr, SW_SHOW);
                }
                else if (pNMLink->hdr.idFrom == IDC_ZLIB && (pNMLink->item.iLink == 0))
                {
                    // The item.szUrl field is always empty, so the link has to be hard-coded.
                    ShellExecuteW(nullptr, L"open", L"http://zlib.net", nullptr, nullptr, SW_SHOW);
                }
                break;
            }
            }
            break;
        }

        return FALSE;
    }

    class GmicDialog
    {
    public:
        GmicDialog(
            const boost::filesystem::path& indexFilePath,
            const boost::filesystem::path& outputDir,
            const boost::filesystem::path& gmicParametersFilePath,
            bool showFullUI)
            : dialogHandle(), threadHandle(nullptr), errorCode(noErr), errorInfo(),
            indexFilePath(indexFilePath), outputDir(outputDir),
            gmicParametersFilePath(gmicParametersFilePath), showFullUI(showFullUI), dialogShown(false)
        {
        }

        ~GmicDialog()
        {
        }

        OSErr GetErrorCode() const
        {
            return errorCode;
        }

        const char* GetErrorMessage() const
        {
            return errorInfo.GetErrorMessage();
        }

        bool HasErrorMessage() const
        {
            return errorInfo.HasErrorMessage();
        }

        static INT_PTR CALLBACK StaticDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
        {
            GmicDialog* dialog;

            if (wMsg == WM_INITDIALOG)
            {
                dialog = reinterpret_cast<GmicDialog*>(lParam);

                SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(dialog));
            }
            else
            {
                dialog = reinterpret_cast<GmicDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
            }

            if (dialog != nullptr)
            {
                return dialog->DlgProc(hDlg, wMsg, wParam, lParam);
            }
            else
            {
                return FALSE;
            }
        }

    private:

        static unsigned int __stdcall StaticThreadProc(LPVOID lpParameter)
        {
            GmicDialog* dialog = static_cast<GmicDialog*>(lpParameter);

            dialog->ThreadProc();
            return 0;
        }

        static constexpr UINT StartGmicProcessThreadMessage = WM_APP;
        static constexpr UINT EndGmicProcessThreadMessage = StartGmicProcessThreadMessage + 1;

        void ThreadProc()
        {
            errorCode = ExecuteGmicQt(
                indexFilePath,
                outputDir,
                gmicParametersFilePath,
                showFullUI,
                errorInfo);

            PostMessage(dialogHandle, EndGmicProcessThreadMessage, 0, 0);
        }

        bool InitializeDialog(HWND hDlg)
        {
            bool result = false;

            constexpr int BufferSize = 256;
            WCHAR buffer[BufferSize] = {0};

            const UINT stringResourceID = showFullUI ? GMICDIALOG_FULLUI_TEXT : GMICDIALOG_REPEATFILTER_TEXT;

            try
            {
                THROW_LAST_ERROR_IF(LoadStringW(wil::GetModuleInstanceHandle(), stringResourceID, buffer, BufferSize) == 0);

                THROW_IF_WIN32_BOOL_FALSE(SetDlgItemTextW(hDlg, IDC_GMICINFO, buffer));

                result = true;
            }
            catch (const wil::ResultException& e)
            {
                switch (e.GetErrorCode())
                {
                case E_OUTOFMEMORY:
                    errorCode = memFullErr;
                    break;
                default:
                    errorCode = ioErr;
                    errorInfo.SetErrorMesage(e.what());
                    break;
                }
            }

            return result;
        }

        INT_PTR DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
        {
            UNREFERENCED_PARAMETER(wParam);
            UNREFERENCED_PARAMETER(lParam);

            switch (wMsg)
            {
            case WM_INITDIALOG:
                CenterDialog(hDlg);
                dialogHandle = hDlg;
                if (!InitializeDialog(hDlg))
                {
                    EndDialog(hDlg, IDABORT);
                }
                return TRUE;
            case WM_WINDOWPOSCHANGED:
                // Start the G'MIC-Qt process worker thread after the dialog has been displayed to the user.
                //
                // This code was adapted from Raymond Chen's blog post "Waiting until the dialog box is displayed before doing something"
                // https://devblogs.microsoft.com/oldnewthing/20060925-02/?p=29603
                if ((reinterpret_cast<WINDOWPOS*>(lParam)->flags & SWP_SHOWWINDOW) != 0 && !dialogShown)
                {
                    dialogShown = true;
                    PostMessage(hDlg, StartGmicProcessThreadMessage, 0, 0);
                }
                break;
            case StartGmicProcessThreadMessage:
                threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, StaticThreadProc, static_cast<void*>(this), 0, nullptr));

                if (threadHandle == nullptr)
                {
                    errorCode = ioErr;
                    errorInfo.SetErrorMesage("Unable to start the G'MIC-Qt process worker thread.");
                    EndDialog(hDlg, IDABORT);
                }
                break;
            case EndGmicProcessThreadMessage:
                WaitForSingleObject(threadHandle, INFINITE);
                CloseHandle(threadHandle);
                EndDialog(hDlg, IDOK);
                break;
            }

            return FALSE;
        }

        HWND dialogHandle;
        HANDLE threadHandle;
        OSErr errorCode;
        GmicProcessErrorInfo errorInfo;
        boost::filesystem::path indexFilePath;
        boost::filesystem::path outputDir;
        boost::filesystem::path gmicParametersFilePath;
        bool showFullUI;
        bool dialogShown;
    };
}

OSErr DoAbout(const AboutRecord* about) noexcept
{
    PlatformData* platform = static_cast<PlatformData*>(about->platformData);

    HWND parent = platform != nullptr ? reinterpret_cast<HWND>(platform->hwnd) : nullptr;

    DialogBoxParam(wil::GetModuleInstanceHandle(), MAKEINTRESOURCE(IDD_ABOUT), parent, AboutDlgProc, 0);

    return noErr;
}

OSErr ShowGmicUI(
    const boost::filesystem::path& indexFilePath,
    const boost::filesystem::path& outputDir,
    const boost::filesystem::path& gmicParametersFilePath,
    bool showFullUI,
    const FilterRecordPtr filterRecord)
{
    OSErr err = noErr;

    PlatformData* platform = static_cast<PlatformData*>(filterRecord->platformData);

    HWND parent = platform != nullptr ? reinterpret_cast<HWND>(platform->hwnd) : nullptr;

    try
    {
        ::std::unique_ptr<GmicDialog> dialog = ::std::make_unique<GmicDialog>(indexFilePath, outputDir, gmicParametersFilePath, showFullUI);

        DialogBoxParam(
            wil::GetModuleInstanceHandle(),
            MAKEINTRESOURCE(IDD_GMICPLUGIN),
            parent,
            GmicDialog::StaticDlgProc,
            reinterpret_cast<LPARAM>(dialog.get()));

        err = dialog->GetErrorCode();

        if (err != noErr && dialog->HasErrorMessage())
        {
            err = ShowErrorMessage(dialog->GetErrorMessage(), filterRecord, err);
        }
    }
    catch (const ::std::bad_alloc&)
    {
        err = memFullErr;
    }

    return err;
}
