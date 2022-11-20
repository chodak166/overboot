// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBCONFIG_H
#define OBCONFIG_H

#include "ob/ObDefs.h"

#include <stdbool.h>

typedef struct ObDurable
{
  char path[OB_PATH_MAX];
  bool copyOrigin;
  bool forceFileType;
  struct ObDurable* next;
} ObDurable;

typedef struct ObConfig
{
  char prefix[OB_PREFIX_MAX];
  char devicePath[OB_PATH_MAX];

  char headLayer[OB_NAME_MAX];
  char repository[OB_PATH_MAX];
  char configDir[OB_PATH_MAX];
  char tmpfsSize[16];
  const char* configPath;

  bool enabled;
  bool bindLayers;
  bool useTmpfs;
  bool clearUpper;
  bool rollback;
  bool upperAsLower;
  bool safeMode;
  ObDurable* durable;

} ObConfig;


void obAddDurable(ObConfig* config, const char* path);

int obCountDurables(const ObConfig* config);

void obFreeDurable(ObDurable* durable);


#endif // OBCONFIG_H
