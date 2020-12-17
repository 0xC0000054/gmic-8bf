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

#include <string>
#include <boost/filesystem.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>

class GmicIOSettings
{
    friend class boost::serialization::access;

public:
    GmicIOSettings();

    ~GmicIOSettings();

    boost::filesystem::path GetDefaultOutputPath() const;

    void SetDefaultOutputPath(const boost::filesystem::path& path);

private:

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        (void)version;

#if BOOST_OS_WINDOWS
        std::wstring path;
#else
        std::string path;
#endif // BOOST_OS_WINDOWS

        if (Archive::is_saving::value)
        {
#if BOOST_OS_WINDOWS
            path = defaultOutputPath.wstring();
#else
            path = defaultOutputPath.string();
#endif // BOOST_OS_WINDOWS
        }

        ar& path;

        if (Archive::is_loading::value)
        {
            defaultOutputPath = path;
        }
    }

    boost::filesystem::path defaultOutputPath;
};

#endif // !GMICOUTPUTSETTINGS_H
