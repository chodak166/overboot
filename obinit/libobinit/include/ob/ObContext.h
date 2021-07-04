// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBCONTEXT_H
#define OBCONTEXT_H

#include "ob/ObDefs.h"

#include <stdbool.h>
#include <inttypes.h>

typedef struct ObDurable
{
  char path[OB_PATH_MAX];
  bool copyOrigin;
  struct ObDurable* next;
} ObDurable;

typedef struct ObContext
{
  char prefix[OB_PREFIX_MAX];
  char devicePath[OB_PATH_MAX];

  char headLayer[OB_NAME_MAX];
  char repository[OB_PATH_MAX];
  char tmpfsSize[16];

  bool enabled;
  bool bindLayers;
  bool useTmpfs;
  bool clearUpper;

  ObDurable* durable;

  char devMountPoint[OB_DEV_PATH_MAX];
  char overlayDir[OB_DEV_PATH_MAX];

} ObContext;

/**
 * @brief Create *initialized* OB context
 * @param prefix path to prepend, usually empty
 * @return Initialized OB context
 */
ObContext* obCreateObContext(const char* prefix);

/**
 * @brief Setup default and internal context fields
 * @param context OB context to setup
 * @param prefix path to prepend, usually empty
 */
void obInitializeObContext(ObContext* context, const char* prefix);

/**
 * @brief Free OB context
 * @param context OB context
 */
void obFreeObContext(ObContext** context);

/**
 * @brief Setup context's device path to match existing node
 * @param context OB context
 * @return true if existing node has been matched
 */
bool obFindDevice(ObContext* context);

void obAddDurable(ObContext* context, const char* path, bool copyOrigin);

int obCountDurables(ObContext* context);

#endif // OBCONTEXT_H
