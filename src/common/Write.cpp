////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2025 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GmicPlugin.h"
#include "FileUtil.h"
#include "ClipboardUtil.h"
#include "ColorManagement.h"
#include "ImageConversion.h"
#include "InputLayerIndex.h"
#include "Gmic8bfImageWriter.h"
#include "GmicQtParameters.h"
#include "resource.h"
#include "Utilities.h"
#include <string>
#include <wil\result.h>

namespace
{
    void WriteGmicParametersFile(
        const boost::filesystem::path& gmicParametersFilePath,
        FilterRecord* filterRecord)
    {
        GmicQtParameters parameters(filterRecord);

        if (parameters.IsValid())
        {
            parameters.SaveToFile(gmicParametersFilePath);
        }
    }

    void WriteAlternateInputImageData(
        const GmicIOSettings& settings,
        InputLayerIndex* layerIndex)
    {
        const SecondInputImageSource source = settings.GetSecondInputImageSource();

        if (source != SecondInputImageSource::None)
        {
            ::std::unique_ptr<InputLayerInfo> layer;

            if (source == SecondInputImageSource::Clipboard)
            {
                ConvertClipboardImageToGmicInput(layer);
            }
            else if (source == SecondInputImageSource::File)
            {
                ConvertImageToGmicInputFormat(
                    settings.GetSecondInputImagePath(),
                    layer);
            }

            if (layer)
            {
                layerIndex->AddFile(*layer.get());
            }
        }
    }

    bool IsGrayScale(const FilterRecord* filterRecord)
    {
        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeGray16:
        case plugInModeGray32:
            return true;
        case plugInModeRGBColor:
        case plugInModeRGB48:
        case plugInModeRGB96:
            return false;
        default:
            throw OSErrException(filterBadMode);
        }
    }

    void WriteLayerIndexFile(
        const boost::filesystem::path& inputDir,
        const boost::filesystem::path& indexFilePath,
        FilterRecord* filterRecord,
        const int32& bitsPerChannel,
        const GmicIOSettings& settings)
    {
        const bool grayScale = IsGrayScale(filterRecord);

        ::std::unique_ptr<InputLayerIndex> inputLayerIndex = ::std::make_unique<InputLayerIndex>(static_cast<uint8>(bitsPerChannel), grayScale);

        const boost::filesystem::path imageProfilePath = WriteImageColorProfile(filterRecord, inputDir);
        boost::filesystem::path displayProfilePath;

        if (!imageProfilePath.empty())
        {
            // Only fetch the display color profile if there is an image color profile.
            displayProfilePath = GetPrimaryDisplayColorProfilePath();
        }

        inputLayerIndex->SetColorProfiles(imageProfilePath, displayProfilePath);

#if PSSDK_HAS_LAYER_SUPPORT
        int32 targetLayerIndex = 0;

        if (DocumentHasMultipleLayers(filterRecord) &&
            HostSupportsReadingFromMultipleLayers(filterRecord) &&
            TryGetTargetLayerIndex(filterRecord, targetLayerIndex))
        {
            SaveAllLayers(inputDir, bitsPerChannel, grayScale, inputLayerIndex.get(), targetLayerIndex, filterRecord);
        }
        else
#endif
        {
            SaveActiveLayer(inputDir, bitsPerChannel, grayScale, inputLayerIndex.get(), filterRecord);

            WriteAlternateInputImageData(settings, inputLayerIndex.get());
        }

        inputLayerIndex->Write(indexFilePath);
    }
}

OSErr WriteGmicFiles(
    const boost::filesystem::path& inputDir,
    boost::filesystem::path& indexFilePath,
    boost::filesystem::path& gmicParametersFilePath,
    FilterRecord* filterRecord,
    const int32& hostBitDepth,
    const GmicIOSettings& settings)
{
    PrintFunctionName();

    OSErr err = noErr;

    try
    {
        indexFilePath = GetTemporaryFileName(inputDir, ".idx");

        WriteLayerIndexFile(inputDir, indexFilePath, filterRecord, hostBitDepth, settings);

        gmicParametersFilePath = GetTemporaryFileName(inputDir, ".g8p");

        WriteGmicParametersFile(gmicParametersFilePath, filterRecord);
    }
    catch (const ::std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (const OSErrException& e)
    {
        err = e.GetErrorCode();
    }
    catch (const ::std::exception& e)
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
