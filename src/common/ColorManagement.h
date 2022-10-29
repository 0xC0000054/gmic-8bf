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

#ifndef COLORMANAGEMENT_H
#define COLORMANAGEMENT_H

#include "Common.h"

boost::filesystem::path GetDisplayColorProfilePath();

boost::filesystem::path WriteImageColorProfile(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& outputDir);

#endif // !COLORMANAGEMENT_H
