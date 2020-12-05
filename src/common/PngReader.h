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

#ifndef PNGREADER_H
#define PNGREADER_H

#include "GmicPlugin.h"

OSErr LoadPngImage(const boost::filesystem::path& path, FilterRecord* filterRecord);

#endif // !PNGREADER_H
