// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBYAMLLAYERREADER_H
#define OBYAMLLAYERREADER_H

#include "ObLayerInfo.h"

ObLayerInfo* obLoadLayerInfo(const char* repoPath, const char* layerName, ObLayerInfo* info);

ObLayerInfo* obLoadLayerInfoYaml(const char* yamlPath, ObLayerInfo* info);

#endif // OBYAMLLAYERREADER_H
