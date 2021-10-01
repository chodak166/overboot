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

  char upperPath[OB_DEV_PATH_MAX];
  sprintf(upperPath, "%s/upper", context->overbootDir);

  if (!config->useTmpfs) {
    char srcPath[OB_PATH_MAX];
    sprintf(srcPath, "%s/%s/upper", context->devMountPoint, config->repository);

    if (config->clearUpper) {
      obLogI("Clearing upper directory (%s)", srcPath);
      if (!obRemoveDirR(srcPath)) {
        return false;
      }
      if (obMkpath(srcPath, OB_MKPATH_MODE) != 0) {
        return false;
      }

    }
    else if (!obExists(srcPath) && obMkpath(srcPath, OB_MKPATH_MODE) != 0) {
      return false;
    }

    if (!obRbind(srcPath, upperPath)) {
      return false;
    }
  }

  return true;
}


bool obInitLowerRoot(ObContext* context)
{
  char lowerPath[OB_PATH_MAX];

  sprintf(lowerPath, "%s/lower-root", context->overbootDir);
  obLogI("Moving %s to %s", context->root, lowerPath);

  if (!obMove(context->root, lowerPath)) {
    return false;
  }

  return true;
}


bool obInitOverlayfs(ObContext* context)
{
  ObConfig* config = &context->config;

  char workPath[OB_DEV_PATH_MAX];
  sprintf(workPath, "%s/work", context->overbootDir);

  char repoPath[OB_PATH_MAX];
  sprintf(repoPath, "%s/%s", context->devMountPoint, config->repository);
  char layersPath[OB_PATH_MAX];
  sprintf(layersPath, "%s/%s", repoPath, OB_LAYERS_DIR_NAME);

  //TODO: use sds
  char lowerPath[OB_PATH_MAX];
  sprintf(lowerPath, "%s/lower-root", context->overbootDir);

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

  //TODO: use sds
  char upperPath[OB_DEV_PATH_MAX];
  sprintf(upperPath, "%s/upper", context->overbootDir);

  if (!obMountOverlay(layers, count, upperPath, workPath, context->root)) {
    obLogE("Cannot mount overlay");
    return false;
  }

  return true;
}


bool obInitManagementBindings(ObContext* context)
{
  ObConfig* config = &context->config;

  char bindedOverlay[OB_PATH_MAX];
  sprintf(bindedOverlay, "%s/%s", context->root, OB_USER_BINDINGS_DIR);

  if (!obRbind(context->overbootDir, bindedOverlay)) {
    return false;
  }

  if (config->bindLayers) {
    char layersDir[OB_PATH_MAX];
    char bindedLayersDir[OB_PATH_MAX];

    //TODO: use sds
    char repoPath[OB_PATH_MAX];
    sprintf(repoPath, "%s/%s", context->devMountPoint, config->repository);

    sprintf(layersDir, "%s/layers", repoPath);
    sprintf(bindedLayersDir, "%s/layers", bindedOverlay);

    obMkpath(layersDir, OB_MKPATH_MODE);
    obMkpath(bindedLayersDir, OB_MKPATH_MODE);
    if (!obRbind(layersDir, bindedLayersDir)) {
      return false;
    }
  }

  return true;
}


bool obInitFstab(ObContext* context)
{
  ObConfig* config = &context->config;
  char mtabPath[OB_PATH_MAX];
  sprintf(mtabPath, "%s/etc/mtab", config->prefix);

  return updateFstab(context->root, mtabPath) == 0;
}


bool obInitDurables(ObContext* context)
{
  ObConfig* config = &context->config;
  ObDurable* durable = config->durable;

  while (durable != NULL) {
    char persistentPath[OB_PATH_MAX];
    char bindPath[OB_PATH_MAX];

    //TODO: use sds
    char repoPath[OB_PATH_MAX];
    sprintf(repoPath, "%s/%s", context->devMountPoint, config->repository);

    sprintf(persistentPath, "%s/%s%s", repoPath, OB_DURABLES_DIR_NAME, durable->path);
    sprintf(bindPath, "%s%s", context->root, durable->path);

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
      return false;
    }
    durable = durable->next;
  }

  return true;
}

