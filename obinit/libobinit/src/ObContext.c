// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObContext.h"

#include <string.h>
#include <stdio.h>

void obSetPrefix(ObContext* context, const char* prefix)
{
  strcpy(context->prefix, prefix);
  sprintf(context->devMountPoint, "%s%s", prefix, OB_DEV_MOUNT_POINT);
}

void obSetDevicePath(ObContext* context, const char* path)
{
  sprintf(context->devicePath, "%s%s", context->prefix, path);
}
