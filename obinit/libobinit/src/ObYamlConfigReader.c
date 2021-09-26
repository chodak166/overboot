// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObYamlConfigReader.h"
#include "ob/ObConfig.h"
#include "ObYamlParser.h"

#include <stdio.h>
#include <string.h>

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
}


static void onSequenceEntryStart(ObConfig* config, const char* itemPath)
{
  if (strcmp(itemPath, ".durables") == 0) {
    obAddDurable(config, "", false);
  }
}


// --------- public API ---------- //


void obLoadYamlConfig(ObConfig* config, const char* path)
{
  obParseYamlFile(config, path,
                (ObYamlValueCallback)&onScalarValue,
                (ObYamlEntryCallback)&onSequenceEntryStart);
}
