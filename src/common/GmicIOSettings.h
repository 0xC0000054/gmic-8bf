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

#pragma once

#ifndef GMICIOSETTINGS_H
#define GMICIOSETTINGS_H

#include "Common.h"
#include <string>
#include <boost/filesystem.hpp>

enum class SecondInputImageSource : uint32_t
{
    None = 0,
    Clipboard = 1,
    File = 2
};

class GmicIOSettings
{
public:
    GmicIOSettings();

    ~GmicIOSettings();

    boost::filesystem::path GetDefaultOutputPath() const;

    SecondInputImageSource GetSecondInputImageSource() const;

    boost::filesystem::path GetSecondInputImagePath() const;

    void SetDefaultOutputPath(const boost::filesystem::path& path);

    void SetSecondInputImageSource(SecondInputImageSource source);

    void SetSecondInputImagePath(const boost::filesystem::path& filePath);

    OSErr Load(const boost::filesystem::path& path);

    OSErr Save(const boost::filesystem::path& path);

private:

    boost::filesystem::path defaultOutputPath;
    SecondInputImageSource secondInputImageSource;
    boost::filesystem::path secondInputImagePath;
};

#endif // !GMICOUTPUTSETTINGS_H
