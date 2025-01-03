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

#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <PITypes.h>
#include <boost/filesystem.hpp>

boost::filesystem::path GetGmicQtPath();

boost::filesystem::path GetInputDirectory();

boost::filesystem::path GetOutputDirectory();

boost::filesystem::path GetIOSettingsPath();

boost::filesystem::path GetTemporaryFileName(const boost::filesystem::path& dir, const char* const fileExtension);

#endif // !FILEUTIL_H
