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

#ifndef GMICQTPARAMETERS_H
#define GMICQTPARAMETERS_H

#include "Common.h"
#include "GmicPluginTerminology.h"
#include "ASZStringSuite.h"
#include <string>
#include <boost/filesystem.hpp>

class GmicQtParameters
{
public:

    GmicQtParameters(const FilterRecordPtr filterRecord);

    GmicQtParameters(const boost::filesystem::path& path);

    ~GmicQtParameters();

    bool IsValid() const;

    void SaveToDescriptor(FilterRecordPtr filterRecord) const;

    void SaveToFile(const boost::filesystem::path& path) const;

private:

    OSErr ReadFilterInputMode(
        ASZStringSuite* zstringSuite,
        PSActionDescriptorProcs* descriptorProcs,
        PIActionDescriptor descriptor);

    OSErr ReadFilterOpaqueData(
        const FilterRecordPtr filterRecord,
        PSActionDescriptorProcs* suite,
        PIActionDescriptor descriptor);

    OSErr WriteFilterOpaqueData(
        const FilterRecordPtr filterRecord,
        PSActionDescriptorProcs* suite,
        PIActionDescriptor descriptor) const;

    std::string command;
    std::string filterMenuPath;
    std::string inputMode;
    std::string filterName;
};

#endif

