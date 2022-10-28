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

#ifndef INPUTLAYERINDEX_H
#define INPUTLAYERINDEX_H

#include "GmicPlugin.h"
#include "FileUtil.h"
#include "InputLayerInfo.h"
#include <boost/filesystem.hpp>
#include <string>
#include <vector>

class InputLayerIndex
{
public:
    InputLayerIndex(uint8 imageBitDepth, bool grayScale);

    ~InputLayerIndex();

    void AddFile(
        const boost::filesystem::path& path,
        int32 width,
        int32 height,
        bool visible,
        ::std::string utf8Name);

    void AddFile(const InputLayerInfo& info);

    int32 GetLayerCount() const;

    void SetActiveLayerIndex(int32 index);

    void Write(const boost::filesystem::path& path);

private:

    ::std::vector<InputLayerInfo> inputFiles;
    int32 activeLayerIndex;
    uint8 imageBitDepth;
    bool grayScale;
};

#endif // !INPUTLAYERINDEX_H
