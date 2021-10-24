// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObContext.h"
#include "ob/ObLogging.h"
#include "ObOsUtils.h"
#include "ObBlkid.h"
#include "sds.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char* ROOTMNT_ENV_VAR = "rootmnt";
static const char* DEFAULT_ROOTMNT = "/root";
static const char* DEFAULT_TMPFS_SIZE = "50%";
static const char* DEFAULT_REPO_NAME = "overboot";

static bool obIsValidDevice(const char* path)
{
  return obIsBlockDevice(path)
      || obIsFile(path);
}


// --------- public API ---------- //

void obInitializeObContext(ObContext* context, const char* prefix)
{
  ObConfig* config = &context->config;
  strcpy(config->prefix, prefix);
  strcpy(config->devicePath, "");
  strcpy(config->headLayer, "");
  strcpy(config->repository, DEFAULT_REPO_NAME);
  strcpy(config->tmpfsSize, DEFAULT_TMPFS_SIZE);

  config->enabled = true;
  config->bindLayers = true;
  config->useTmpfs = true;
  config->clearUpper = false;
  config->rollback = false;

  config->durable = NULL;

  context->devMountPoint = sdsnew(prefix);
  context->devMountPoint = sdscat(context->devMountPoint, OB_DEV_MOUNT_POINT);

  context->overbootDir = sdsnew(prefix);
  context->overbootDir = sdscat(context->overbootDir, OB_OVERLAY_DIR);

  context->root = sdsnew(prefix);
  char* rootmnt = getenv(ROOTMNT_ENV_VAR);
  if (rootmnt != NULL) {
    context->root = sdscat(context->root, rootmnt);
  }
  else {
    context->root = sdscat(context->root, DEFAULT_ROOTMNT);
    obLogI("The rootmnt environment variable not set, using %s", context->root);
  }

  context->dirAsDevice = false;
}

ObContext* obCreateObContext(const char* prefix)
{
  ObContext* context = malloc(sizeof(ObContext));
  obInitializeObContext(context, prefix);
  return context;
}

void obFreeObContext(ObContext** context)
{
  sdsfree((*context)->devMountPoint);
  sdsfree((*context)->overbootDir);
  sdsfree((*context)->root);
  obFreeDurable((*context)->config.durable);
  free(*context);
  *context = NULL;
}

bool obFindDevice(ObContext* context)
{
  ObConfig* config = &context->config;
  bool result = true;

  if (obIsUuid(config->devicePath)
      && !obGetPathByUuid(config->devicePath, OB_PATH_MAX)) {
    return false;
  }

  if (!obIsValidDevice(config->devicePath)) {
    char newPath[OB_DEV_PATH_MAX];
    strcpy(newPath, config->prefix);
    strncat(newPath, config->devicePath, OB_DEV_PATH_MAX);
    if (!obIsValidDevice(newPath)) {
      strcpy(newPath, config->prefix);
      strcat(newPath, context->root);
      strncat(newPath, config->devicePath, OB_DEV_PATH_MAX);
      if (!obIsValidDevice(newPath)) {
        result = false;
      }
    }
    if (result) {
      strcpy(config->devicePath, newPath);
    }
  }
  return result;
}

void obLogObContext(const ObContext* context)
{
  const ObConfig* config = &context->config;
  obLogI("enabled: %i", config->enabled);
  obLogI("use tmpfs: %i", config->useTmpfs);
  obLogI("tmpfs size: %s", config->tmpfsSize);
  obLogI("bind layers: %i", config->bindLayers);
  obLogI("Device path: %s", config->devicePath);
  obLogI("head layer: %s", config->headLayer);
  obLogI("repository: %s", config->repository);

  int durablesCount = obCountDurables(config);
  obLogI("durables (%i):", durablesCount);

  ObDurable* durable = config->durable;
  while (durable != NULL) {
    obLogI("path: %s, copy_origin: %i", durable->path, durable->copyOrigin);
    durable = durable->next;
  }

  obLogI("ramfs overlay dir: %s", context->overbootDir);
  obLogI("repository device mount point: %s", context->devMountPoint);
  obLogI("root path: %s", context->root);
}

