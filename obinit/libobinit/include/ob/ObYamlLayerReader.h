// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBYAMLLAYERREADER_H
#define OBYAMLLAYERREADER_H

#include "ob/ObDefs.h"

//TODO: move the struct
typedef struct ObLayerInfo
{
  char name[OB_NAME_MAX];
  char author[OB_LAYER_AUTHOR_MAX];
  char createTs[OB_TS_MAX];
  char description[OB_LAYER_DESC_MAX];
  char underlayer[OB_NAME_MAX];

  char rootPath[OB_PATH_MAX];
} ObLayerInfo;

ObLayerInfo* obLoadLayerInfo(const char* repoPath, const char* layerName, ObLayerInfo* info);

ObLayerInfo* obLoadLayerInfoYaml(const char* yamlPath, ObLayerInfo* info);

#endif // OBYAMLLAYERREADER_H
