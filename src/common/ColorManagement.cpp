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

#include "ColorManagement.h"
#include "FileIO.h"
#include "FileUtil.h"
#include "ScopedHandleSuite.h"
#include "Utilities.h"

#if __PIWin__
#include "ColorManagementWin.h"
#else
#error "Missing a ColorManagement header for this platform."
#endif

boost::filesystem::path GetPrimaryDisplayColorProfilePath()
{
    return GetPrimaryDisplayColorProfilePathNative();
}

boost::filesystem::path WriteImageColorProfile(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& outputDir)
{
    boost::filesystem::path path;

    if (HandleSuiteIsAvailable(filterRecord) &&
        filterRecord->canUseICCProfiles &&
        filterRecord->iCCprofileData &&
        filterRecord->iCCprofileSize > 0)
    {
        path = GetTemporaryFileName(outputDir, ".icc");

        std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Write);

        const ScopedHandleSuiteLock lock(filterRecord->handleProcs, filterRecord->iCCprofileData);

        WriteFile(file.get(), lock.Data(), static_cast<size_t>(filterRecord->iCCprofileSize));
    }

    return path;
}
