// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObYamlConfigReader.h"
#include "ObYamlParser.h"
#include "ob/ObContext.h"

#include <stdio.h>
#include <string.h>

static void onScalarValue(ObContext* context, const char* itemPath, const char* value)
{
  printf("onScalarValue: %s -> %s\n", itemPath, value);
  if (strcmp(itemPath, ".enabled") == 0) {
    context->enabled = strcmp(value, "true") == 0 ? true : false;
  }
  else if (strcmp(itemPath, ".layers.visible") == 0) {
    context->bindLayers = strcmp(value, "true") == 0 ? true : false;
  }
  else if (strcmp(itemPath, ".layers.device") == 0) {
    char buf[OB_DEV_PATH_MAX];
    strcpy(buf, value);
    obSetDevicePath(context, buf);
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
}


static void onSequenceEntryStart(ObContext* context, const char* itemPath)
{
  printf("onSequenceEntryStart: %s\n", itemPath);
}

void obLoadYamlConfig(ObContext* context, const char* path)
{
  obParseYamlFile(context, path,
                (ObYamlValueCallback)&onScalarValue,
                (ObYamlEntryCallback)&onSequenceEntryStart);
}
