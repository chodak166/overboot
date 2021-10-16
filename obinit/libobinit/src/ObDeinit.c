// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObDeinit.h"
#include "ob/ObMount.h"

#include <sds.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

// --------- public API ---------- //

bool obDeinitPersistentDevice(ObContext* context)
{
  bool result = obUnmountDevice(context->devMountPoint);
  rmdir(context->devMountPoint);
  return result;
}


bool obDeinitOverbootDir(ObContext* context)
{
  //TODO: use sds?

  char path[OB_DEV_PATH_MAX];
  sprintf(path, "%s/work", context->overbootDir);
  bool result = rmdir(path) == 0;

  sprintf(path, "%s/lower-root", context->overbootDir);
  result = rmdir(path) == 0 && result;

  sprintf(path, "%s/upper", context->overbootDir);
  result = rmdir(path) == 0 && result;

  result = obUnmount(context->overbootDir) && result;
  result = rmdir(context->overbootDir) == 0 && result;
  return result;
}


bool obDeinitLowerRoot(ObContext* context)
{
  //TODO: use obGetLowerRootPath
  char path[OB_DEV_PATH_MAX];
  sprintf(path, "%s/lower-root", context->overbootDir);
  return obMove(path, context->root);
}


bool obDeinitOverlayfs(ObContext* context)
{
  return obUnmount(context->root);
}


bool obDeinitManagementBindings(ObContext* context)
{
  //TODO: move to a common function
  sds bindedOverlay = sdsempty();
  bindedOverlay = sdscatprintf(bindedOverlay, "%s/%s", context->root, OB_USER_BINDINGS_DIR);

  //TODO: move to a common function
  sds bindedLayersDir = sdsnew(bindedOverlay);
  bindedLayersDir = sdscat(bindedLayersDir, "/layers");

  bool result = obUnmount(bindedLayersDir);
  result = rmdir(bindedLayersDir) && result;
  result = obUnmount(bindedOverlay) && result;
  result = rmdir(bindedOverlay) && result;

  sdsfree(bindedLayersDir);
  sdsfree(bindedOverlay);
  return result;
}


bool obDeinitFstab(ObContext* context)
{
  //TODO: move to a common function
  sds fstabPath = sdsnew(context->root);
  fstabPath = sdscat(fstabPath, "/etc/fstab");

  sds origFstabPath = sdsnew(fstabPath);
  origFstabPath = sdscat(origFstabPath, ".orig");

  bool result = rename(origFstabPath, fstabPath) == 0;
  sdsfree(fstabPath);
  sdsfree(origFstabPath);

  return result;
}


bool obDeinitDurables(ObContext* context)
{
  bool result = true;
  ObConfig* config = &context->config;
  ObDurable* durable = config->durable;

  while (durable != NULL) {
    sds bindPath = sdsnew(context->root);
    bindPath = sdscat(bindPath, durable->path);
    result = obUnmount(bindPath) && result;

    sdsfree(bindPath);
    durable = durable->next;
  }

  return result;
}

