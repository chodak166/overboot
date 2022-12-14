// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBCONTEXT_H
#define OBCONTEXT_H

#include "ob/ObDefs.h"
#include "ob/ObConfig.h"

#include <stdbool.h>
#include <inttypes.h>

typedef enum ObDeviceType
{
  OB_DEV_UNKNOWN = 0,
  OB_DEV_BLK, // block device
  OB_DEV_IMG, // embedded image
  OB_DEV_DIR  // embedded directory
} ObDeviceType;

typedef struct ObContext
{
  struct ObConfig config;

  char* foundDevicePath;
  char* devMountPoint;
  char* overbootDir;
  char* root;
  ObDeviceType deviceType;

  bool reloadConfig;
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
void obLogObContext(const ObContext* context);

#endif // OBCONTEXT_H
