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

#include "GmicIOSettingsPlugin.h"
#include "GmicIOSettings.h"
#include "FileUtil.h"

OSErr GmicIOSettingsDoParameters(FilterRecord* filterRecord);
OSErr GmicIOSettingsDoPrepare();
OSErr GmicIOSettingsDoStart();
OSErr GmicIOSettingsDoContinue();
OSErr GmicIOSettingsDoFinish();

namespace
{
    OSErr LaunderOSErrResult(OSErr err, const FilterRecordPtr filterRecord)
    {
        return LaunderOSErrResult(err, "Input/Output Settings for G'MIC-Qt", filterRecord);
    }

    OSErr ShowErrorMessage(const char* message, const FilterRecordPtr filterRecord, OSErr fallbackErrorCode)
    {
        return ShowErrorMessage(message, "Input/Output Settings for G'MIC-Qt", filterRecord, fallbackErrorCode);
    }
}

DLLExport MACPASCAL void Gmic_IO_Settings_Entry_Point(
    const int16 selector,
    FilterRecordPtr filterRecord,
    intptr_t* data,
    int16* result)
{
    (void)data;

    DebugOut("%s selector: %d", __FUNCTION__, selector);

    try
    {
        switch (selector)
        {
        case filterSelectorAbout:
            // The about box is handled by the G'MIC-Qt plug-in.
            *result = noErr;
            break;
        case filterSelectorParameters:
            *result = LaunderOSErrResult(GmicIOSettingsDoParameters(filterRecord), filterRecord);
            break;
        case filterSelectorPrepare:
            *result = LaunderOSErrResult(GmicIOSettingsDoPrepare(), filterRecord);
            break;
        case filterSelectorStart:
            *result = LaunderOSErrResult(GmicIOSettingsDoStart(), filterRecord);
            break;
        case filterSelectorContinue:
            *result = LaunderOSErrResult(GmicIOSettingsDoContinue(), filterRecord);
            break;
        case filterSelectorFinish:
            *result = LaunderOSErrResult(GmicIOSettingsDoFinish(), filterRecord);
            break;
        default:
            *result = filterBadParameters;
            break;
        }
    }
    catch (const OSErr e)
    {
        *result = e;
    }
    catch (const std::bad_alloc&)
    {
        *result = memFullErr;
    }
    catch (const std::length_error&)
    {
        // The std::*string classes can throw this if the string
        // exceeds the implementation-defined max size.
        *result = memFullErr;
    }
    catch (...)
    {
        *result = paramErr;
    }

    DebugOut("%s selector: %d, result: %d", __FUNCTION__, selector, *result);
}

OSErr GmicIOSettingsDoParameters(FilterRecord* filterRecord)
{
    PrintFunctionName();

#if DEBUG_BUILD
    std::string sig = FourCCToString(filterRecord->hostSig);

    DebugOut("Host signature: 0x%X (%s)", filterRecord->hostSig, sig.c_str());
#endif // DEBUG_BUILD

    OSErr err = noErr;

    try
    {
        boost::filesystem::path settingsPath = GetIOSettingsPath();

        GmicIOSettings settings;
        settings.Load(settingsPath);

        err = DoIOSettingsUI(filterRecord, settings);

        if (err == noErr)
        {
            settings.Save(settingsPath);

            // Prevent the settings plug-in from appearing in the "Last Filter" menu.
            err = userCanceledErr;
        }
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (const OSErrException& e)
    {
        err = e.GetErrorCode();
    }
    catch (const std::exception& e)
    {
        err = ShowErrorMessage(e.what(), filterRecord, readErr);
    }
    catch (...)
    {
        err = ShowErrorMessage("An unspecified error occurred.", filterRecord, ioErr);
    }

    return err;
}

OSErr GmicIOSettingsDoPrepare()
{
    PrintFunctionName();

    return noErr;
}

OSErr GmicIOSettingsDoStart()
{
    PrintFunctionName();

    return userCanceledErr;
}

OSErr GmicIOSettingsDoContinue()
{
    PrintFunctionName();

    return noErr;
}

OSErr GmicIOSettingsDoFinish()
{
    PrintFunctionName();

    return noErr;
}
