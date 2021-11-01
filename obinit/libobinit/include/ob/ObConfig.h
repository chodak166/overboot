// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBCONFIG_H
#define OBCONFIG_H

#include "ob/ObDefs.h"

#include <stdbool.h>

typedef struct ObDurable
{
  char path[OB_PATH_MAX];
  bool copyOrigin;
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
  ObDurable* durable;

} ObConfig;


void obAddDurable(ObConfig* config, const char* path, bool copyOrigin);

int obCountDurables(const ObConfig* config);

void obFreeDurable(ObDurable* durable);


#endif // OBCONFIG_H
