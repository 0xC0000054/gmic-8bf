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

#include "ColorManagementWin.h"
#include <wil/resource.h>
#include <memory>

boost::filesystem::path GetPrimaryDisplayColorProfilePathNative()
{
    boost::filesystem::path path;

    try
    {
        POINT ptZero = { 0 , 0 };

        HMONITOR primaryMonitor = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);

        MONITORINFOEXW monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);

        if (GetMonitorInfoW(primaryMonitor, &monitorInfo))
        {
            wil::unique_hdc hdc(CreateDCW(monitorInfo.szDevice, monitorInfo.szDevice, nullptr, nullptr));

            if (hdc)
            {
                DWORD size = 0;

                GetICMProfileW(hdc.get(), &size, nullptr);

                if (size > 0)
                {
                    std::unique_ptr<WCHAR[]> chars = std::make_unique<WCHAR[]>(static_cast<size_t>(size));

                    if (GetICMProfileW(hdc.get(), &size, chars.get()))
                    {
                        path = chars.get();
                    }
                }
            }
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

    return path;
}
