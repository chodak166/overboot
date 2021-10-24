// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObYamlConfigReader.h"
#include "ob/ObOsUtils.h"
#include "ob/ObConfig.h"
#include "ob/ObLogging.h"
#include "ObYamlParser.h"

#include <sds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>

#define MAX_CONFIG_EXT_LEN 8

static void onScalarValue(ObConfig* config, const char* itemPath, const char* value)
{
  if (strcmp(itemPath, ".enabled") == 0) {
    config->enabled = strcmp(value, "true") == 0 ? true : false;
  }
  else if (strcmp(itemPath, ".layers.visible") == 0) {
    config->bindLayers = strcmp(value, "true") == 0 ? true : false;
  }
  else if (strcmp(itemPath, ".layers.device") == 0) {
    strcpy(config->devicePath, value);
  }
  else if (strcmp(itemPath, ".layers.repository") == 0) {
    strcpy(config->repository, value);
  }
  else if (strcmp(itemPath, ".layers.head") == 0) {
    strcpy(config->headLayer, value);
  }
  else if (strcmp(itemPath, ".upper.type") == 0) {
    config->useTmpfs = strcmp(value, "tmpfs") == 0 ? true : false;
    config->clearUpper = strcmp(value, "volatile") == 0 ? true : false;
  }
  else if (strcmp(itemPath, ".upper.size") == 0) {
    strcpy(config->tmpfsSize, value);
  }
  else if (strcmp(itemPath, ".durables..path") == 0
           && config->durable != NULL) {
    strcpy(config->durable->path, value);
  }
  else if (strcmp(itemPath, ".durables..copy_origin") == 0
           && config->durable != NULL) {
    config->durable->copyOrigin = strcmp(value, "true") == 0 ? true : false;
  }
  else if (strcmp(itemPath, ".config_dir") == 0) {
    strcpy(config->configDir, value);
  }
  else if (strcmp(itemPath, ".rollback") == 0) {
    config->rollback = strcmp(value, "true") == 0 ? true : false;
  }
}


static void onSequenceEntryStart(ObConfig* config, const char* itemPath)
{
  if (strcmp(itemPath, ".durables") == 0) {
    obAddDurable(config, "", false);
  }
}

static int configFilter(const struct dirent* entry)
{
  char extArray[][MAX_CONFIG_EXT_LEN] = {".yaml", ".yml", ".conf", ".cfg", ".config"};
  size_t len = strlen(entry->d_name);

  for (size_t e = 0; e < sizeof(extArray)/MAX_CONFIG_EXT_LEN; ++e) {
    if (strcmp(entry->d_name + (len - strlen(extArray[e])), extArray[e]) == 0) {
      return 1;
    }
  }

  return 0;
}

static bool loadYamlConfigDir(ObConfig* config, const char* configFilePath)
{
  sds buffer = sdsnew(configFilePath);
  char* configDirname = dirname(buffer);
  sds configDirPath = sdsempty();
  configDirPath = sdscatfmt(configDirPath, "%s/%s", configDirname, config->configDir);
  bool result = true;

  if (!obExists(configDirPath)) {
    obLogW("Config directory (%s) does not exist, skipping");
  }
  else {
    struct dirent **namelist;
    int n = scandir(configDirPath, &namelist, configFilter, alphasort);
    if (n == -1) {
      obLogE("Cannot open directory: %s", configDirPath);
      result = false;
    }
    else {
      for (int i = 0; i < n; ++i) {
          sds fullPath = sdsempty();
          fullPath = sdscatfmt(fullPath, "%s/%s", configDirPath, namelist[i]->d_name);
          obLogI("Loading partial config: %s", fullPath);
          obLoadYamlConfig(config, fullPath);
          sdsfree(fullPath);
          free(namelist[i]);
      }
      free(namelist);
    }
  }

  sdsfree(buffer);
  sdsfree(configDirPath);
  return result;
}


// --------- public API ---------- //


bool obLoadYamlConfig(ObConfig* config, const char* path)
{
  memset(config->configDir, 0, sizeof(config->configDir));
  bool result = obParseYamlFile(config, path,
                (ObYamlValueCallback)&onScalarValue,
                (ObYamlEntryCallback)&onSequenceEntryStart);

  if (result && strlen(config->configDir) > 0) {
    result = loadYamlConfigDir(config, path);
  }

  return result;
}
