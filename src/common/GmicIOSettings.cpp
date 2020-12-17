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

#include "GmicIOSettings.h"

GmicIOSettings::GmicIOSettings() : defaultOutputPath()
{
}

GmicIOSettings::~GmicIOSettings()
{
}

boost::filesystem::path GmicIOSettings::GetDefaultOutputPath() const
{
    return defaultOutputPath;
}

void GmicIOSettings::SetDefaultOutputPath(const boost::filesystem::path& path)
{
    defaultOutputPath = path;
}
