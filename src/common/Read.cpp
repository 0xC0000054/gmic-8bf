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
#include "GmicIOSettings.h"
#include "FolderBrowser.h"
#include "ImageSaveDialog.h"
#include "Gmic8bfImageReader.h"
#include "GmicQtParameters.h"
#include "PngWriter.h"
#include "resource.h"
#include "Utilities.h"
#include <algorithm>
#include <memory>
#include <vector>
#include <wil\result.h>
#include <setjmp.h>
#include <png.h>

namespace
{
    std::vector<boost::filesystem::path> GetOutputFiles(const boost::filesystem::path& outputDir)
    {
        std::vector<boost::filesystem::path> filePaths;

        for (auto& file : boost::filesystem::directory_iterator(outputDir))
        {
            filePaths.push_back(file.path());
        }

        return filePaths;
    }

    bool TryGetDefaultOutputFolder(const GmicIOSettings& settings, boost::filesystem::path& defaultOutputFolder)
    {
        bool result = false;

        const boost::filesystem::path& savedOutputPath = settings.GetDefaultOutputPath();

        if (!savedOutputPath.empty())
        {
            try
            {
                defaultOutputFolder = savedOutputPath;
                result = true;
            }
            catch (...)
            {
                // Ignore any errors, the folder picker or save dialog will be shown.
            }
        }

        return result;
    }

    OSErr GetOutputFolder(
        FilterRecordPtr filterRecord,
        const GmicIOSettings& settings,
        boost::filesystem::path& outputFolder)
    {
        if (TryGetDefaultOutputFolder(settings, outputFolder))
        {
            return noErr;
        }
        else
        {
            return GetGmicOutputFolder(filterRecord, outputFolder);
        }
    }

    OSErr GetResizedImageOutputPath(
        const FilterRecordPtr filterRecord,
        const GmicIOSettings& settings,
        const boost::filesystem::path& originalFileName,
        boost::filesystem::path& outputFileName)
    {
        bool haveFilePathFromDefaultFolder = false;

        boost::filesystem::path defaultFolderPath;

        if (TryGetDefaultOutputFolder(settings, defaultFolderPath))
        {

            try
            {
                boost::filesystem::path temp = defaultFolderPath;
                temp /= originalFileName;

                outputFileName = temp;
                haveFilePathFromDefaultFolder = true;
            }
            catch (...)
            {
                // Ignore any errors, the save dialog will be shown.
            }
        }

        if(haveFilePathFromDefaultFolder)
        {
            boost::filesystem::create_directories(defaultFolderPath);
            return noErr;
        }
        else
        {
            return GetNewImageFileName(filterRecord, originalFileName, outputFileName);
        }
    }
}

OSErr ReadGmicOutput(
    const boost::filesystem::path& outputDir,
    const boost::filesystem::path& gmicParametersFilePath,
    bool fullUIWasShown,
    FilterRecord* filterRecord,
    const GmicIOSettings& settings)
{
    PrintFunctionName();

    OSErr err = noErr;

    try
    {
        std::vector<boost::filesystem::path> filePaths = GetOutputFiles(outputDir);

        if (filePaths.empty())
        {
            throw std::runtime_error("G'MIC did not produce any output images.");
        }
        else
        {
            const char* const outputFileExtension = ".png";

            GmicQtParameters parameters(gmicParametersFilePath);

            if (filePaths.size() == 1)
            {
                const boost::filesystem::path& filePath = filePaths[0];
                const VPoint& documentSize = GetImageSize(filterRecord);
                bool imageSizeMatchesDocument = ImageSizeMatchesDocument(filePath, documentSize);

                if (imageSizeMatchesDocument)
                {
                    CopyImageToActiveLayer(filePath, filterRecord);
                }
                else
                {
                    boost::filesystem::path outputFilePath;

                    OSErrException::ThrowIfError(GetResizedImageOutputPath(
                        filterRecord,
                        settings,
                        parameters.PrependGmicCommandName(filePath.filename()).replace_extension(outputFileExtension),
                        outputFilePath));

                    ConvertGmic8bfImageToPng(filterRecord, filePath, outputFilePath);
                }
            }
            else
            {
                boost::filesystem::path outputFolder;

                OSErrException::ThrowIfError(GetOutputFolder(filterRecord, settings, outputFolder));

                boost::filesystem::create_directories(outputFolder);

                for (size_t i = 0; i < filePaths.size(); i++)
                {
                    const boost::filesystem::path& inputFilePath = filePaths[i];

                    boost::filesystem::path outputFilePath = outputFolder;
                    outputFilePath /= parameters.PrependGmicCommandName(inputFilePath.filename()).replace_extension(outputFileExtension);

                    ConvertGmic8bfImageToPng(filterRecord, inputFilePath, outputFilePath);
                }
            }

            if (fullUIWasShown && parameters.IsValid())
            {
                parameters.SaveToDescriptor(filterRecord);
            }
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
        err = ShowErrorMessage("An unspecified error occurred when processing the G'MIC-Qt output.", filterRecord, readErr);
    }

    // Do not set the FilterRecord data pointers to NULL, some hosts
    // (e.g. XnView) will crash if they are set to NULL by a plug-in.
    SetOutputRect(filterRecord, 0, 0, 0, 0);
    SetMaskRect(filterRecord, 0, 0, 0, 0);

    return err;
}
