// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBYAMLLAYERREADER_H
#define OBYAMLLAYERREADER_H

#include "ObLayerInfo.h"

ObLayerInfo* obLoadLayerInfo(const char* repoPath, const char* layerName, ObLayerInfo* info);

ObLayerInfo* obLoadLayerInfoYaml(const char* yamlPath, ObLayerInfo* info);

#endif // OBYAMLLAYERREADER_H
