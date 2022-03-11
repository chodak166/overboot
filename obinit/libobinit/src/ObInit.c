// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObInit.h"
#include "ob/ObLogging.h"
#include "ob/ObHash.h"
#include "ObMount.h"
#include "ObOsUtils.h"
#include "ObFstab.h"
#include "ObPaths.h"
#include "ObLayerCollector.h"
#include "sds.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

typedef struct OverlayPaths
{
  sds workPath;
  sds lowerPath;
  sds upperPath;
  sds repoPath;
  sds layersPath;
} OverlayPaths;

static OverlayPaths newOverlayPaths(const ObContext* context)
{
  OverlayPaths paths;
  paths.workPath = obGetOverlayWorkPath(context);

  paths.repoPath = sdsempty();
  paths.repoPath = sdscatfmt(paths.repoPath, ""
                                "%s/%s", context->devMountPoint,
                             context->config.repository);

  paths.layersPath = obGetLayersPath(context);

  paths.lowerPath = obGetLowerRootPath(context);
  paths.upperPath = obGetUpperPath(context);
  return paths;
}

static void freeOverlayPaths(OverlayPaths* paths)
{
  sdsfree(paths->workPath);
  sdsfree(paths->repoPath);
  sdsfree(paths->layersPath);
  sdsfree(paths->lowerPath);
  sdsfree(paths->upperPath);
}


static bool obPreparePersistentUpperDir(const ObContext* context, sds upperPath)
{
  obLogI("Preparing persistent upper layer dir: %s", upperPath);
  bool result = true;
  const ObConfig* config = &context->config;

  if (config->clearUpper) {
    obLogI("Clearing upper directory (%s)", upperPath);
    if (!obRemoveDirR(upperPath)) {
      obLogE("Clearing upper directory failed");
      result = false;
    }
    else if (!obMkpath(upperPath, OB_MKPATH_MODE)) {
      result = false;
    }
  }
  else if (!obExists(upperPath) && !obMkpath(upperPath, OB_MKPATH_MODE)) {
    result = false;
  }

  if (!result) {
    obLogE("Preparing persistent upper dir failed");
  }
  return result;
}


static bool obPrepareOverlay(const ObContext* context)
{
  const ObConfig* config = &context->config;
  if (!obMountTmpfs(context->overbootDir, config->tmpfsSize)) {
    return false;
  }

  sds path = obGetOverlayWorkPath(context);
  if (!obMkpath(path, OB_MKPATH_MODE)) {
    sdsfree(path);
    return false;
  }

  sdsfree(path);
  path = obGetLowerRootPath(context);
  if (!obMkpath(path, OB_MKPATH_MODE)) {
    sdsfree(path);
    return false;
  }

  sdsfree(path);
  path = obGetUpperPath(context);
  if (!obMkpath(path, OB_MKPATH_MODE)) {
    sdsfree(path);
    return false;
  }

  sdsfree(path);
  return true;
}

static bool obBindJobsDir(const ObContext* context, const char* bindedOverlay)
{
  sds jobsDir = obGetJobsPath(context);
  if (!obMkpath(jobsDir, OB_MKPATH_MODE)) {
    sdsfree(jobsDir);
    return false;
  }

  sds bindedJobsDir = obGetBindedJobsPath(bindedOverlay);
  bool result = true;

  if (!obMkpath(jobsDir, OB_MKPATH_MODE)) {
    result = false;
  }
  else {
    result = obRbind(jobsDir, bindedJobsDir);
  }

  sdsfree(jobsDir);
  sdsfree(bindedJobsDir);
  return result;
}


// --------- public API ---------- //


bool obInitPersistentDevice(ObContext* context)
{
  ObConfig* config = &context->config;
  if (!obFindDevice(context)) {
    obLogE("Cannot find or identify device: %s", config->devicePath);
    return false;
  }

  switch(context->deviceType) {
  case OB_DEV_BLK:
    return obMountBlockDevice(context->foundDevicePath, context->devMountPoint);
  case OB_DEV_IMG:
    obLogW("Using embedded image as a repository device requires RW mount of the lower layer");
    obRemountRw(context->root, NULL);
    return obMountImageFile(context->foundDevicePath, context->devMountPoint);
  case OB_DEV_DIR:
    obLogW("Using embedded directory as a repository device requires RW mount of the lower layer");
    obRemountRw(context->root, NULL);
    if (!config->useTmpfs) {
      obLogE("Using repository from the same device as rootfs is not supported due to the overlayfs limitations. Please use embeddd image instead or tmpfs as the upper layer.");
      return false;
    }
    return obMountEmbeddedRepository(context->foundDevicePath, context->devMountPoint);
  default:
    obLogE("Unknown device type");
  }

  return false;
}

bool obInitOverbootDir(ObContext* context)
{
  obLogI("Initializing overboot working directory");

  ObConfig* config = &context->config;
  if (!obPrepareOverlay(context)) {
    obLogE("Cannot prepare overboot dir (%s)", context->overbootDir);
    return false;
  }

  bool result = true;
  sds upperPath = obGetUpperPath(context);

  if (!config->useTmpfs && !obPreparePersistentUpperDir(context, upperPath)) {
    result = false;
  }

  sdsfree(upperPath);
  return result;
}


bool obInitLowerRoot(ObContext* context)
{
  bool result = true;
  sds lowerPath = obGetLowerRootPath(context);

  if (!obMove(context->root, lowerPath)) {
    result = false;
  }

  sdsfree(lowerPath);
  return result;
}


bool obInitOverlayfs(ObContext* context)
{
  bool result = true;
  ObConfig* config = &context->config;

  OverlayPaths paths = newOverlayPaths(context);

  uint8_t count = 0;
  ObLayerItem* topLayer = obCollectLayers(paths.layersPath, config->headLayer,
                                          paths.lowerPath, &count);

  if (!topLayer) {
    freeOverlayPaths(&paths);
    return false;
  }

  if (count == 0) {
    topLayer = calloc(1, sizeof(ObLayerItem));
    strcpy(topLayer->layerPath, paths.lowerPath);
    topLayer->prev = NULL;
    count = 1;
  }

  if (config->useTmpfs && config->upperAsLower) {
    ObLayerItem* extraLayer = calloc(1, sizeof(ObLayerItem));
    sds persistentUpperPath = obGetPersistentUpperPath(context);
    strcpy(extraLayer->layerPath, persistentUpperPath);
    sdsfree(persistentUpperPath);
    extraLayer->prev = topLayer;
    topLayer = extraLayer;
    count += 1;
  }

  char* layers[count];

  obLogI("Collected layers:");
  ObLayerItem* layerItem = topLayer;
  uint8_t i = 0;

  while (layerItem) {
    layers[count - i - 1] = layerItem->layerPath;
    obLogI("Layer root path [%i]: %s", i, layerItem->layerPath);
    i += 1;
    layerItem = layerItem->prev;
  }

  if (!obMountOverlay(layers, count, paths.upperPath,
                      paths.workPath, context->root)) {
    obLogE("Cannot mount overlay");
    result = false;
  }

  if (context->deviceType == OB_DEV_BLK) {
    obRemountRo(paths.lowerPath, NULL);
  }

  layerItem = topLayer;
  while (layerItem) {
    ObLayerItem* currentItem = layerItem;
    layerItem = layerItem->prev;
    free(currentItem);
  }

  if (context->deviceType == OB_DEV_DIR
      && !obBlockByTmpfs(context->foundDevicePath)) {
    result = false;
  }

  //TODO: block image by whiteout?

  chmod(context->root, OB_ROOT_MODE); //TODO: move to mount?

  freeOverlayPaths(&paths);
  return result;
}


bool obInitManagementBindings(ObContext* context)
{
  bool result = true;
  ObConfig* config = &context->config;

  sds bindedOverlay = obGetBindedOverlayPath(context);

  if (!obRbind(context->overbootDir, bindedOverlay)) {
    sdsfree(bindedOverlay);
    return false;
  }

  if (config->bindLayers) {
    sds repoPath = obGetRepoPath(context);

    sds layersDir = sdsnew(repoPath);
    layersDir = sdscat(layersDir, "/layers");

    sds bindedLayersDir = obGetBindedLayersPath(bindedOverlay);

    obMkpath(layersDir, OB_MKPATH_MODE);
    obMkpath(bindedLayersDir, OB_MKPATH_MODE);
    if (!obRbind(layersDir, bindedLayersDir)) {
      result = false;
    }

    sdsfree(repoPath);
    sdsfree(layersDir);
    sdsfree(bindedLayersDir);
  }

  if (result && !config->useTmpfs) {
    sds upper = obGetUpperPath(context);
    sds bindedUpper = obGetBindedUpperPath(context);
    if (!obRbind(upper, bindedUpper)) {
      result = false;
    }
    sdsfree(upper);
    sdsfree(bindedUpper);
  }

  result = result && obBindJobsDir(context, bindedOverlay);

  sdsfree(bindedOverlay);
  return result;
}


bool obInitFstab(ObContext* context)
{
  sds mtabPath = sdsnew(context->config.prefix);
  mtabPath = sdscat(mtabPath, "/etc/mtab");
  bool result = obUpdateFstab(context->root, mtabPath);

  sdsfree(mtabPath);
  return result;
}


bool obInitDurables(ObContext* context)
{
  bool result = true;
  ObConfig* config = &context->config;
  ObDurable* durable = config->durable;
  sds repoPath = obGetRepoPath(context);

  while (durable != NULL && result == true) {

    sds persistentPath = sdsempty();
    persistentPath = sdscatprintf(persistentPath, "%s/%s%s", repoPath, OB_DURABLES_DIR_NAME, durable->path);

    sds bindPath = sdsnew(context->root);
    bindPath = sdscat(bindPath, durable->path);
    obLogI("Preparing durable %s", bindPath);

    if (!obExists(bindPath)) {
      if (durable->forceFileType) {
        obCreateBlankFile(bindPath);
        obCreateBlankFile(persistentPath);
      }
      else {
        obMkpath(bindPath, OB_MKPATH_MODE);
        obMkpath(persistentPath, OB_MKPATH_MODE);
      }
    }
    else {
      bool isDir = obIsDirectory(bindPath);
      if (isDir && !obExists(persistentPath)) {
        obLogI("Persistent directory not found, creating: %s", persistentPath);
        obLogI("is dir: %i", obIsDirectory(persistentPath));
        obMkpath(persistentPath, OB_MKPATH_MODE);
        obLogI("is dir: %i", obIsDirectory(persistentPath));

        if (durable->copyOrigin) {
          obLogI("Copying origin from %s", bindPath);
          obSync(bindPath, persistentPath);
        }
      }
      else if (!isDir && !obExists(persistentPath)) {
        obLogI("This durable is not a directory");
        if (durable->copyOrigin) {
          obLogI("Copying original file from %s to %s", bindPath, persistentPath);
          if (!obCopyFile(bindPath, persistentPath)) {
            obLogE("Copying originl file failed");
          }
        }
        else {
          obCreateBlankFile(persistentPath);
        }
      }
    }

    obLogI("Binding durable: %s to %s", persistentPath, bindPath);
    if (!obRbind(persistentPath, bindPath)) {
      result = false;
    }
    durable = durable->next;

    sdsfree(persistentPath);
    sdsfree(bindPath);
  }

  sdsfree(repoPath);
  return result;
}


bool obInitLock(ObContext* context)
{
  if (!context->config.safeMode) {
    return true;
  }

  uint64_t currentConfigHash = obCalcualateFileHash(context->config.configPath);

  sds lockPath = obGetLockFilePath(context);
  if (obExists(lockPath)) {
    obLogW("Lock file found in %s", lockPath);
    uint64_t oldConfigHash = obReadHashValue(lockPath);
    obLogI("Comparing old config (%" PRIx64 ") with current config (%" PRIx64 ")",
           oldConfigHash, currentConfigHash);
    if (oldConfigHash == currentConfigHash) {
      obLogW("Locked config hasn't changed, aborting due to enabled safe mode");
      sdsfree(lockPath);
      return false;
    }
  }

  bool result = obWriteAsHexStr(currentConfigHash, lockPath);
  sdsfree(lockPath);
  return result;
}

bool obUnsetLock(ObContext* context)
{
  sds lockPath = obGetLockFilePath(context);
  bool result = true;
  if (obExists(lockPath) && !obRemovePath(lockPath)) {
    obLogE("Cannot remove lock file: %s", lockPath);
    result = false;
  }
  sdsfree(lockPath);
  return result;
}
