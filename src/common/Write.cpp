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

#include "stdafx.h"
#include "GmicPlugin.h"
#include "FileUtil.h"
#include "InputLayerIndex.h"
#include "GmicInputWriter.h"
#include "resource.h"
#include <string>
#include <wil\result.h>

OSErr WriteGmicFiles(
    const boost::filesystem::path& inputDir,
    boost::filesystem::path& indexFilePath,
    FilterRecord* filterRecord,
    const GmicIOSettings& settings)
{
    PrintFunctionName();

    OSErr err = noErr;

    try
    {
        std::unique_ptr<InputLayerIndex> inputLayerIndex = std::make_unique<InputLayerIndex>(filterRecord->imageMode);

#if PSSDK_HAS_LAYER_SUPPORT
        int32 targetLayerIndex = 0;

        if (DocumentHasMultipleLayers(filterRecord) &&
            HostSupportsReadingFromMultipleLayers(filterRecord) &&
            TryGetTargetLayerIndex(filterRecord, targetLayerIndex))
        {
            SaveAllLayers(inputDir, inputLayerIndex.get(), targetLayerIndex, filterRecord);
        }
        else
#endif

        {
            SaveActiveLayer(inputDir, inputLayerIndex.get(), filterRecord);
        }

        indexFilePath = GetTemporaryFileName(inputDir, ".idx");

        inputLayerIndex->Write(indexFilePath, settings);
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
        err = ShowErrorMessage(e.what(), filterRecord, writErr);
    }
    catch (...)
    {
        err = ShowErrorMessage("An unspecified error occurred when writing the G'MIC-Qt input.", filterRecord, writErr);
    }

    // Do not set the FilterRecord data pointers to NULL, some hosts
    // (e.g. XnView) will crash if they are set to NULL by a plug-in.
    SetInputRect(filterRecord, 0, 0, 0, 0);

    return err;
}
