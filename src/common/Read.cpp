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
        OSErr err = noErr;

        FilterParameters* parameters = LockParameters(filterRecord);

        if (parameters->showUI || parameters->lastOutputFolder == nullptr)
        {
            err = GetGmicOutputFolder(filterRecord, outputFolder);

            if (err == noErr)
            {
                if (parameters->lastOutputFolder != nullptr)
                {
                    UnlockPIHandle(filterRecord, parameters->lastOutputFolder);
                    DisposePIHandle(filterRecord, parameters->lastOutputFolder);
                    parameters->lastOutputFolder = nullptr;
                }

                size_t pathSizeInBytes = outputFolder.size() * sizeof(boost::filesystem::path::value_type);

                if (pathSizeInBytes > static_cast<size_t>(std::numeric_limits<int32>::max()))
                {
                    err = memFullErr;
                }

                if (err == noErr)
                {
                    err = NewPIHandle(filterRecord, static_cast<int32>(pathSizeInBytes), &parameters->lastOutputFolder);

                    if (err == noErr)
                    {
                        uint8* destPtr = reinterpret_cast<uint8*>(LockPIHandle(filterRecord, parameters->lastOutputFolder, false));

                        std::memcpy(destPtr, outputFolder.c_str(), pathSizeInBytes);

                        UnlockPIHandle(filterRecord, parameters->lastOutputFolder);
                    }
                }
            }
        }
        else
        {
            Ptr destPtr = LockPIHandle(filterRecord, parameters->lastOutputFolder, false);

            try
            {
                const boost::filesystem::path::value_type* str = reinterpret_cast<const boost::filesystem::path::value_type*>(destPtr);

                outputFolder = str;
            }
            catch (const std::bad_alloc&)
            {
                err = memFullErr;
            }
            catch (...)
            {
                err = ioErr;
            }

            UnlockPIHandle(filterRecord, parameters->lastOutputFolder);
        }

        UnlockParameters(filterRecord);

        return err;
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
            err = LoadPngImage(filePaths[0], filterRecord);
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
