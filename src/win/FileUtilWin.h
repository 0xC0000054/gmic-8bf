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

#ifndef FILEUTILWIN_H
#define FILEUTILWIN_H

#include "FileUtil.h"

boost::filesystem::path GetGmicQtPathNative();

boost::filesystem::path GetPluginSettingsDirectoryNative();

boost::filesystem::path GetSessionDirectoriesRootNative();

#endif // !FILEUTILWIN_H
