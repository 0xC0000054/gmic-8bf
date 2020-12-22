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
            err = SaveAllLayers(inputDir, inputLayerIndex.get(), targetLayerIndex, filterRecord);
        }
        else
#endif

        {
            err = SaveActiveLayer(inputDir, inputLayerIndex.get(), filterRecord);
        }

        if (err == noErr)
        {
            err = GetTemporaryFileName(inputDir, indexFilePath, ".idx");

            if (err == noErr)
            {
                err = inputLayerIndex->Write(indexFilePath, filterRecord, settings);
            }
        }

    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }

    return err;
}
