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

#include "ImageUtil.h"

::std::vector<uint16> BuildSixteenBitToHostLUT()
{
    ::std::vector<uint16> sixteenBitToHostLUT;
    sixteenBitToHostLUT.reserve(65536);

    for (size_t i = 0; i < sixteenBitToHostLUT.capacity(); i++)
    {
        // According to the Photoshop SDK 16-bit image data is stored in the range of [0, 32768].
        sixteenBitToHostLUT.push_back(static_cast<uint16>(((i * 32768) + 32767) / 65535));
    }

    return sixteenBitToHostLUT;
}
