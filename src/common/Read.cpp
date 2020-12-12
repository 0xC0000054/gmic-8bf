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
#include "FolderBrowser.h"
#include "ImageSaveDialog.h"
#include "PngReader.h"
#include "resource.h"
#include <algorithm>
#include <memory>
#include <vector>
#include <wil\result.h>
#include <setjmp.h>
#include <png.h>

namespace
{
    OSErr GetOutputFiles(const boost::filesystem::path& outputDir, std::vector<boost::filesystem::path>& filePaths)
    {
        OSErr err = noErr;

        try
        {
            for (auto& file : boost::filesystem::directory_iterator(outputDir))
            {
                filePaths.push_back(file.path());
            }

            if (filePaths.size() > 1)
            {
                // Sort the returned file names.
                std::sort(filePaths.begin(), filePaths.end());
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

    OSErr GetOutputFolder(FilterRecordPtr filterRecord, boost::filesystem::path& outputFolder)
    {
        return GetGmicOutputFolder(filterRecord, outputFolder);
    }
}

OSErr ReadGmicOutput(const boost::filesystem::path& outputDir, FilterRecord* filterRecord)
{
    PrintFunctionName();

    OSErr err = noErr;

    std::vector<boost::filesystem::path> filePaths;

    err = GetOutputFiles(outputDir, filePaths);

    if (err == noErr)
    {
        if (filePaths.size() == 1)
        {
            const boost::filesystem::path& filePath = filePaths[0];
            const VPoint& documentSize = GetImageSize(filterRecord);
            bool imageSizeMatchesDocument = false;

            err = PngImageSizeMatchesDocument(filePath, documentSize, imageSizeMatchesDocument);

            if (err == noErr)
            {
                if (imageSizeMatchesDocument)
                {
                    err = LoadPngImage(filePath, filterRecord);
                }
                else
                {
                    boost::filesystem::path outputFileName;

                    err = GetNewImageFileName(filterRecord, outputFileName);

                    if (err == noErr)
                    {
                        try
                        {
                            boost::filesystem::copy_file(filePath, outputFileName, boost::filesystem::copy_options::overwrite_existing);
                        }
                        catch (const std::bad_alloc&)
                        {
                            err = memFullErr;
                        }
                        catch (const boost::filesystem::filesystem_error& e)
                        {
                            err = ShowErrorMessage(e.what(), filterRecord, ioErr);
                        }
                        catch (...)
                        {
                            err = ioErr;
                        }
                    }
                }
            }
        }
        else
        {
            boost::filesystem::path outputFolder;

            err = GetOutputFolder(filterRecord, outputFolder);

            if (err == noErr)
            {
                try
                {
                    boost::filesystem::create_directories(outputFolder);

                    for (size_t i = 0; i < filePaths.size(); i++)
                    {
                        const boost::filesystem::path& oldPath = filePaths[i];

                        boost::filesystem::path newPath = outputFolder;
                        newPath /= oldPath.filename();

                        boost::filesystem::copy_file(oldPath, newPath, boost::filesystem::copy_options::overwrite_existing);
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
            }
        }
    }

    return err;
}
