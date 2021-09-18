// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObYamlLayerReader.h"
#include "ob/ObOsUtils.h"
#include "ob/ObLogging.h"
#include "ObYamlParser.h"

#include <stdio.h>
#include <string.h>

static void onScalarValue(ObLayerInfo* info, const char* itemPath, const char* value)
{
  if (strcmp(itemPath, ".name") == 0) {
    strcpy(info->name, value);
  }
  else if (strcmp(itemPath, ".author") == 0) {
    strcpy(info->author, value);
  }
  else if (strcmp(itemPath, ".create_ts") == 0) {
    strcpy(info->createTs, value);
  }
  else if (strcmp(itemPath, ".description") == 0) {
    strcpy(info->description, value);
  }
  else if (strcmp(itemPath, ".underlayer") == 0) {
    strcpy(info->underlayer, value);
  }
}

ObLayerInfo* obLoadLayerInfo(const char* repoPath, const char* layerName, ObLayerInfo* info)
{
  char path[OB_PATH_MAX];
  sprintf(path, "%s/%s", repoPath, layerName);
  if (!obExists(path)) {
    sprintf(path, "%s/%s.%s", repoPath, layerName, OB_LAYER_DIR_EXT);
    if (!obExists(path)) {
      obLogE("Layer %s[.%s] not found", layerName, OB_LAYER_DIR_EXT);
      return NULL;
    }
  }

  strcat(path, OB_LAYER_ROOT_DIR);
  strcat(path, OB_LAYER_INFO_PATH);
  if (!obExists(path)) {
    obLogE("Layer info file not found: %s", path);
    return NULL;
  }

  return obLoadLayerInfoYaml(path, info);
}

ObLayerInfo* obLoadLayerInfoYaml(const char* yamlPath, ObLayerInfo* info)
{
  memset(info, 0, sizeof(ObLayerInfo));
  obParseYamlFile(info, yamlPath,
                (ObYamlValueCallback)&onScalarValue,
                NULL);
  strncpy(info->rootPath, yamlPath, strlen(yamlPath) - strlen(OB_LAYER_INFO_PATH));
  return info;
}


