// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBCONTEXT_H
#define OBCONTEXT_H

#include "ob/ObDefs.h"
#include "ob/ObConfig.h"

#include <stdbool.h>
#include <inttypes.h>

typedef struct ObContext
{
  struct ObConfig config;

  char* devMountPoint;
  char* overbootDir;
  char* root;
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

/**
 * @brief Dump given context to stdout/kmsg
 */
void logObContext(const ObContext* context);

#endif // OBCONTEXT_H
