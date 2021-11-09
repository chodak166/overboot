// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObContext.h"
#include "ob/ObLogging.h"
#include "ObOsUtils.h"
#include "ObBlkid.h"
#include <sds.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char* ROOTMNT_ENV_VAR = "rootmnt";
static const char* DEFAULT_ROOTMNT = "/root";
static const char* DEFAULT_TMPFS_SIZE = "50%";
static const char* DEFAULT_DEVICE_PATH = "/var/obdev";
static const char* DEFAULT_HEAD_LAYER = "root";
static const char* DEFAULT_REPO_NAME = "overboot";
static const char* DEFAULT_CONFIG_DIR = "overboot.d";


// --------- public API ---------- //

void obInitializeObContext(ObContext* context, const char* prefix)
{
  ObConfig* config = &context->config;
  strcpy(config->prefix, prefix);
  strcpy(config->devicePath, DEFAULT_DEVICE_PATH);
  strcpy(config->headLayer, DEFAULT_HEAD_LAYER);
  strcpy(config->repository, DEFAULT_REPO_NAME);
  strcpy(config->configDir, DEFAULT_CONFIG_DIR);
  strcpy(config->tmpfsSize, DEFAULT_TMPFS_SIZE);

  config->enabled = true;
  config->bindLayers = true;
  config->useTmpfs = true;
  config->clearUpper = false;
  config->rollback = false;

  config->durable = NULL;

  context->foundDevicePath = sdsempty();

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

  context->deviceType = OB_DEV_UNKNOWN;
  context->reloadConfig = false;
}

ObContext* obCreateObContext(const char* prefix)
{
  ObContext* context = malloc(sizeof(ObContext));
  obInitializeObContext(context, prefix);
  return context;
}

void obFreeObContext(ObContext** context)
{
  sdsfree((*context)->foundDevicePath);
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

  // UUID
  if (obIsUuid(config->devicePath)
      && !obGetPathByUuid(config->devicePath, OB_PATH_MAX)) {
    context->deviceType = OB_DEV_BLK;
    return false;
  }

  if (strlen(config->devicePath) && config->devicePath[0] != '/') {
    obLogE("%s does not look like path nor valid UUID, aborting", config->devicePath);
    return false;
  }

  // block device
  if (obExists(config->devicePath)) {
    if (!obIsBlockDevice(config->devicePath)) {
      obLogE("Device is not a block device: %s", config->devicePath);
      return false;
    }
    context->deviceType = OB_DEV_BLK;
    context->foundDevicePath = sdscpy(context->foundDevicePath, config->devicePath);
    return true;
  }

  // image file
  bool imgFound = true;
  sds newPath = sdsnew(config->prefix);
  newPath = sdscatfmt(newPath, "/%s", config->devicePath);
  if (!obIsFile(newPath)) {
    newPath = sdscpy(newPath, context->root);
    newPath = sdscatfmt(newPath, "/%s", config->devicePath);
    if (!obIsFile(newPath)) {
      imgFound = false;
    }
  }

  sdsfree(context->foundDevicePath);
  context->foundDevicePath = newPath;

  if (imgFound) {
    context->deviceType = OB_DEV_IMG;
  }
  else {
    obLogI("Device not found, using %s as an embedded repository ", newPath);
    context->deviceType = OB_DEV_DIR;
  }

  return true;
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
  obLogI("config dir: %s", config->configDir);

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

