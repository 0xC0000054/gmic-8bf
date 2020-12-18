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

#pragma once

#ifndef GMICIOSETTINGS_H
#define GMICIOSETTINGS_H

#include "Common.h"
#include <string>
#include <boost/filesystem.hpp>

class GmicIOSettings
{
public:
    GmicIOSettings();

    ~GmicIOSettings();

    boost::filesystem::path GetDefaultOutputPath() const;

    void SetDefaultOutputPath(const boost::filesystem::path& path);

    OSErr Load(const boost::filesystem::path& path);

    OSErr Save(const boost::filesystem::path& path);

private:

    boost::filesystem::path defaultOutputPath;
};

#endif // !GMICOUTPUTSETTINGS_H
