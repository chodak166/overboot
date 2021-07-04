// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObContext.h"

#include "ob/ObOsUtils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

static void obFreeDurable(ObDurable* durable)
{
  if (durable == NULL) {
    return;
  }

  if (durable->next != NULL) {
    obFreeDurable(durable->next);
  }
  free(durable);
}

void obInitializeObContext(ObContext* context, const char* prefix)
{
  strcpy(context->prefix, prefix);
  strcpy(context->devicePath, "");
  strcpy(context->headLayer, "");
  strcpy(context->repository, "overboot");
  strcpy(context->tmpfsSize, "50%");

  context->enabled = true;
  context->bindLayers = true;
  context->useTmpfs = true;
  context->clearUpper = false;

  context->durable = NULL;

  sprintf(context->devMountPoint, "%s%s", prefix, OB_DEV_MOUNT_POINT);
  sprintf(context->overlayDir, "%s%s", prefix, OB_OVERLAY_DIR);
}

ObContext* obCreateObContext(const char* prefix)
{
  ObContext* context = malloc(sizeof(ObContext));
  obInitializeObContext(context, prefix);
  return context;
}

void obFreeObContext(ObContext** context)
{
  obFreeDurable((*context)->durable);
  free(*context);
  *context = NULL;
}


static bool obIsValidDevice(const char* path)
{
  //TODO: consider allowing using directory as device
  return obIsBlockDevice(path)
      || obIsFile(path);
}

bool obFindDevice(ObContext* context)
{
  bool result = true;
  //TODO: consider allowing using directory as device
  if (!obIsValidDevice(context->devicePath)) {
    char newPath[OB_DEV_PATH_MAX];
    strcpy(newPath, context->prefix);
    strncat(newPath, context->devicePath, OB_DEV_PATH_MAX);
    if (!obIsValidDevice(newPath)) {
      strcpy(newPath, context->prefix);
      strcat(newPath, "/rootmnt");
      strncat(newPath, context->devicePath, OB_DEV_PATH_MAX);
      if (!obIsValidDevice(newPath)) {
        result = false;
      }
    }
    if (result) {
      strcpy(context->devicePath, newPath);
    }
  }
  return result;
}

void obAddDurable(ObContext* context, const char* path, bool copyOrigin)
{
  ObDurable* durable = malloc(sizeof(ObDurable));
  strcpy(durable->path, path);
  durable->copyOrigin = copyOrigin;
  durable->next = context->durable;
  context->durable = durable;
}

static void obCountDurablesRecursive(ObDurable* durable, int* count)
{
  if (durable != NULL) {
    *count += 1;
    obCountDurablesRecursive(durable->next, count);
  }
}

int obCountDurables(ObContext* context)
{
  int count = 0;
  obCountDurablesRecursive(context->durable, &count);
  return count;
}
