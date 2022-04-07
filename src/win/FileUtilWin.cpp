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
#include "GmicPlugin.h"
#include "FileUtilWin.h"
#include <objbase.h>
#include <ShlObj.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/win32_helpers.h>
#include <boost/filesystem.hpp>

namespace
{
    boost::filesystem::path GetPluginInstallDirectory()
    {
        boost::filesystem::path path;

        try
        {
            wil::unique_cotaskmem_string dllPath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

            boost::filesystem::path temp(dllPath.get());

            path = temp.parent_path();
        }
        catch (const wil::ResultException& e)
        {
            if (e.GetErrorCode() == E_OUTOFMEMORY)
            {
                throw std::bad_alloc();
            }
            else
            {
                throw std::runtime_error(e.what());
            }
        }

        return path;
    }
}

boost::filesystem::path GetGmicQtPathNative()
{
    boost::filesystem::path path = GetPluginInstallDirectory();

    path /= "gmic";
    path /= "gmic_8bf_qt.exe";

    return path;
}

boost::filesystem::path GetPluginSettingsDirectoryNative()
{
    boost::filesystem::path path;

    try
    {
        wil::unique_cotaskmem_string appDataPath;

        THROW_IF_FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appDataPath));

        path = appDataPath.get();
        path /= L"Gmic8bfPlugin";
        path /= L"settings";
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error(e.what());
        }
    }

    return path;
}

boost::filesystem::path GetSessionDirectoriesRootNative()
{
    boost::filesystem::path path;

    try
    {
        wil::unique_cotaskmem_string programDataPath;

        THROW_IF_FAILED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &programDataPath));

        path = programDataPath.get();
        path /= L"Gmic8bfPlugin";
        path /= L"SessionData";
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error(e.what());
        }
    }

    return path;
}
