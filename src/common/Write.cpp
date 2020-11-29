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
#include "PngWarningHandler.h"
#include "PngWriter.h"
#include "resource.h"
#include <string>
#include <wil\result.h>

OSErr WriteGmicFiles(
    const boost::filesystem::path& inputDir,
    boost::filesystem::path& indexFilePath,
    FilterRecord* filterRecord)
{
    PrintFunctionName();

    OSErr err = noErr;

    try
    {
        std::unique_ptr<InputLayerIndex> inputLayerIndex = std::make_unique<InputLayerIndex>();

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
            boost::filesystem::path activeLayerPath;

            err = GetTemporaryFileName(inputDir, activeLayerPath, ".png");

            if (err == noErr)
            {
                err = SaveActiveLayer(activeLayerPath, filterRecord);

                if (err == noErr)
                {
                    const VPoint imageSize = GetImageSize(filterRecord);

                    int32 layerWidth = imageSize.h;
                    int32 layerHeight = imageSize.v;
                    bool layerIsVisible = true;
                    std::string layerName;

                    if (!TryGetLayerNameAsUTF8String(filterRecord, layerName))
                    {
                        layerName = "Layer 0";
                    }

                    err = inputLayerIndex->AddFile(activeLayerPath, layerWidth, layerHeight, layerIsVisible, layerName);
                }
            }
        }

        if (err == noErr)
        {
            err = GetTemporaryFileName(inputDir, indexFilePath, ".idx");

            if (err == noErr)
            {
                err = inputLayerIndex->Write(indexFilePath);
            }
        }

    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }

    return err;
}
