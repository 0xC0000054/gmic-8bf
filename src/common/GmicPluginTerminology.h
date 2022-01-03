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

#pragma once

#ifndef GMICPLUGINTERMINOLOGY_H
#define GMICPLUGINTERMINOLOGY_H

#include "PIActions.h"
#include "PITerminology.h"

#define plugInSuiteID		'gMic'
#define plugInClassID		plugInSuiteID
#define plugInEventID		plugInClassID

#define keyFilterName keyName
#define keyFilterInputMode 'gfiM'
// This is used to store the G'MIC command and G'MIC-Qt menu path data.
// There is no need to show that information to the user in the actions palette.
#define keyFilterOpaqueData keyDatum

#endif // GMICPLUGINTERMINOLOGY_H
