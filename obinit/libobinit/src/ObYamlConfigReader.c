// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObYamlConfigReader.h"
#include "ob/ObContext.h"
#include "ObYamlParser.h"

#include <stdio.h>
#include <string.h>

static void onScalarValue(ObContext* context, const char* itemPath, const char* value)
{
  if (strcmp(itemPath, ".enabled") == 0) {
    context->enabled = strcmp(value, "true") == 0 ? true : false;
  }
  else if (strcmp(itemPath, ".layers.visible") == 0) {
    context->bindLayers = strcmp(value, "true") == 0 ? true : false;
  }
  else if (strcmp(itemPath, ".layers.device") == 0) {
    strcpy(context->devicePath, value);
  }
  else if (strcmp(itemPath, ".layers.repository") == 0) {
    strcpy(context->repository, value);
  }
  else if (strcmp(itemPath, ".layers.head") == 0) {
    strcpy(context->headLayer, value);
  }
  else if (strcmp(itemPath, ".upper.type") == 0) {
    context->useTmpfs = strcmp(value, "tmpfs") == 0 ? true : false;
    context->clearUpper = strcmp(value, "volatile") == 0 ? true : false;
  }
  else if (strcmp(itemPath, ".upper.size") == 0) {
    strcpy(context->tmpfsSize, value);
  }
  else if (strcmp(itemPath, ".durables..path") == 0
           && context->durable != NULL) {
    strcpy(context->durable->path, value);
  }
  else if (strcmp(itemPath, ".durables..copy_origin") == 0
           && context->durable != NULL) {
    context->durable->copyOrigin = strcmp(value, "true") == 0 ? true : false;
  }
}


static void onSequenceEntryStart(ObContext* context, const char* itemPath)
{
  if (strcmp(itemPath, ".durables") == 0) {
    obAddDurable(context, "", false);
  }
}

void obLoadYamlConfig(ObContext* context, const char* path)
{
  obParseYamlFile(context, path,
                (ObYamlValueCallback)&onScalarValue,
                (ObYamlEntryCallback)&onSequenceEntryStart);
}
