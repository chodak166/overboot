// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBCONTEXT_H
#define OBCONTEXT_H

#include "ob/ObDefs.h"

#include <stdbool.h>
#include <inttypes.h>

typedef struct ObContext
{
  char prefix[OB_PREFIX_MAX];
  char devicePath[OB_PATH_MAX];
  char devMountPoint[OB_PATH_MAX];

  char headLayer[OB_NAME_MAX];
  char repository[OB_PATH_MAX];
  char tmpfsSize[16];

  bool enabled;
  bool bindLayers;
  bool useTmpfs;
  bool clearUpper;

  //TODO durables
} ObContext;

void obSetPrefix(ObContext* context, const char* prefix);

void obSetDevicePath(ObContext* context, const char* path);


#endif // OBCONTEXT_H
