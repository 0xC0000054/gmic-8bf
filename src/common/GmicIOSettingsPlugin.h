////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2026 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#ifndef GMICIOSETTINGSPLUGIN_H
#define GMICIOSETTINGSPLUGIN_H

#include "Common.h" // Common definitions that are shared between plug-ins
#include "GmicIOSettings.h"

OSErr DoIOSettingsUI(const FilterRecordPtr filterRecord, GmicIOSettings& settings);

#endif // !GMICIOSETTINGSPLUGIN_H
