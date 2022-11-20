// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ObYamlLayerReader.h"
#include "ObOsUtils.h"
#include "ObYamlParser.h"
#include "ob/ObLogging.h"

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


// --------- public API ---------- //

//TODO: error should cause rollback (write test case)
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
  size_t len =  strlen(yamlPath) - strlen(OB_LAYER_INFO_PATH);
  memccpy(info->rootPath, yamlPath, 0, len);
  return info;
}


