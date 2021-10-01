// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObInit.h"
#include "ob/ObMount.h"
#include "ob/ObOsUtils.h"
#include "ob/ObLayerInfo.h"
#include "ob/ObLogging.h"
#include "ob/ObYamlLayerReader.h" // TODO: move
#include "ObFstab.h"
#include "sds.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


struct ObLayerItem;
typedef struct ObLayerItem
{
  char layerPath[OB_PATH_MAX];
  struct ObLayerItem* prev;
} ObLayerItem;

static bool isRootLayer(const char* layerName)
{
  return strcmp(OB_UNDERLAYER_ROOT, layerName) == 0
      || strcmp("", layerName) == 0;
}

static bool isEndLayer(const char* layerName)
{
  return strcmp(OB_UNDERLAYER_NONE, layerName) == 0;
}

static ObLayerItem* obCollectLayers(const char* layersDir, const char* layerName,
                             const char* lowerPath, uint8_t* count)
{
  ObLayerItem* item = NULL;

  if (isRootLayer(layerName)) {
    item = calloc(1, sizeof(ObLayerItem));
    strcpy(item->layerPath, lowerPath);
    *count += 1;
  }
  else if (!isEndLayer(layerName)) {
    ObLayerInfo info;
    if (obLoadLayerInfo(layersDir, layerName, &info)) {
      item = calloc(1, sizeof(ObLayerItem));
      strcpy(item->layerPath, info.rootPath);
      item->prev = obCollectLayers(layersDir, info.underlayer, lowerPath, count);
      *count += 1;
    }
  }
  return item;
}

static sds obGetUpperPath(const ObContext* context)
{
  sds upperPath = sdsnew(context->overbootDir);
  return sdscat(upperPath, "/upper");
}

static sds obGetLowerRootPath(const ObContext* context)
{
  sds lowerPath = sdsnew(context->overbootDir);
  return sdscat(lowerPath, "/lower-root");
}

static sds obGetRepoPath(const ObContext* context)
{
  sds repoPath = sdsempty();
  return sdscatprintf(repoPath, "%s/%s", context->devMountPoint, context->config.repository);
}

static bool obBindPersistentUpperDir(const ObContext* context, sds upperPath)
{
  bool result = true;
  const ObConfig* config = &context->config;

  sds srcPath = sdsempty();
  srcPath = sdscatprintf(srcPath, "%s/%s/upper", context->devMountPoint, config->repository);

  if (config->clearUpper) {
    obLogI("Clearing upper directory (%s)", srcPath);
    if (!obRemoveDirR(srcPath)) {
      result = false;
    }
    else if (obMkpath(srcPath, OB_MKPATH_MODE) != 0) {
      result = false;
    }
  }
  else if (!obExists(srcPath) && obMkpath(srcPath, OB_MKPATH_MODE) != 0) {
    result = false;
  }
  else if (!obRbind(srcPath, upperPath)) {
    result = false;
  }

  sdsfree(srcPath);
  return result;
}


// --------- public API ---------- //


bool obInitPersistentDevice(ObContext* context)
{
  ObConfig* config = &context->config;
  if (!obFindDevice(context)) {
    obLogE("Device %s not found", config->devicePath);
    return false;
  }

  if (!obMountDevice(config->devicePath, context->devMountPoint)) {
    obLogE("Device mount (%s -> %s) failed", config->devicePath, context->devMountPoint);
    return false;
  }

  return true;
}

bool obInitOverbootDir(ObContext* context)
{
  ObConfig* config = &context->config;
  if (!obPrepareOverlay(context->overbootDir, config->tmpfsSize)) {
    obLogE("Cannot prepare overboot dir (%s)", context->overbootDir);
    return false;
  }

  bool result = true;
  sds upperPath = obGetUpperPath(context);

  if (!config->useTmpfs && !obBindPersistentUpperDir(context, upperPath)) {
    result = false;
  }

  sdsfree(upperPath);
  return result;
}


bool obInitLowerRoot(ObContext* context)
{
  bool result = true;
  sds lowerPath = obGetLowerRootPath(context);

  obLogI("Moving %s to %s", context->root, lowerPath);
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

  sds workPath = sdsnew(context->overbootDir);
  workPath = sdscat(workPath, "/work");

  sds repoPath = sdsempty();
  repoPath = sdscatprintf(repoPath, "%s/%s", context->devMountPoint, config->repository);

  sds layersPath = sdsempty();
  layersPath = sdscatprintf(layersPath, "%s/%s", repoPath, OB_LAYERS_DIR_NAME);

  sds lowerPath = obGetLowerRootPath(context);
  sds upperPath = obGetUpperPath(context);

  uint8_t count = 0;
  ObLayerItem* topLayer = obCollectLayers(layersPath, config->headLayer, lowerPath, &count);
  if (count == 0 && !topLayer) {
    topLayer = calloc(1, sizeof(ObLayerItem));
    strcpy(topLayer->layerPath, lowerPath);
    topLayer->prev = NULL;
    count = 1;
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

  if (!obMountOverlay(layers, count, upperPath, workPath, context->root)) {
    obLogE("Cannot mount overlay");
    result = false;
  }

  layerItem = topLayer;
  while (layerItem) {
    ObLayerItem* currentItem = layerItem;
    layerItem = layerItem->prev;
    free(currentItem);
  }

  sdsfree(workPath);
  sdsfree(repoPath);
  sdsfree(layersPath);
  sdsfree(lowerPath);
  sdsfree(upperPath);
  return result;
}


bool obInitManagementBindings(ObContext* context)
{
  bool result = true;
  ObConfig* config = &context->config;

  sds bindedOverlay = sdsempty();
  bindedOverlay = sdscatprintf(bindedOverlay, "%s/%s", context->root, OB_USER_BINDINGS_DIR);

  if (!obRbind(context->overbootDir, bindedOverlay)) {
    sdsfree(bindedOverlay);
    return false;
  }

  if (config->bindLayers) {
    sds repoPath = obGetRepoPath(context);

    sds layersDir = sdsnew(repoPath);
    layersDir = sdscat(layersDir, "/layers");

    sds bindedLayersDir = sdsnew(bindedOverlay);
    bindedLayersDir = sdscat(bindedLayersDir, "/layers");

    obMkpath(layersDir, OB_MKPATH_MODE);
    obMkpath(bindedLayersDir, OB_MKPATH_MODE);
    if (!obRbind(layersDir, bindedLayersDir)) {
      result = false;
    }

    sdsfree(repoPath);
    sdsfree(layersDir);
    sdsfree(bindedLayersDir);
  }

  sdsfree(bindedOverlay);
  return result;
}


bool obInitFstab(ObContext* context)
{
  sds mtabPath = sdsnew(context->config.prefix);
  mtabPath = sdscat(mtabPath, "/etc/mtab");
  bool result = updateFstab(context->root, mtabPath) == 0;

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

    if (!obExists(bindPath)) {
      obMkpath(bindPath, OB_MKPATH_MODE);
      obMkpath(persistentPath, OB_MKPATH_MODE);
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

