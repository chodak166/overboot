// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ObDeinit.h"
#include "ObMount.h"
#include "ObOsUtils.h"
#include "ObPaths.h"

#include <sds.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

// --------- public API ---------- //

bool obDeinitPersistentDevice(ObContext* context)
{
  if (!obExists(context->devMountPoint)) {
    return true;
  }
  bool result = obUnmountDevice(context->devMountPoint);
  rmdir(context->devMountPoint);

  return result;
}


bool obDeinitOverbootDir(ObContext* context)
{
  sds path = obGetOverlayWorkPath(context);
  bool result = rmdir(path) == 0;

  sdsfree(path);
  path = obGetLowerRootPath(context);
  result = rmdir(path) == 0 && result;

  sdsfree(path);
  path = obGetBindedUpperPath(context);
  result = rmdir(path) == 0 && result;

  result = obUnmount(context->overbootDir) && result;
  result = rmdir(context->overbootDir) == 0 && result;

  sdsfree(path);
  return result;
}


bool obDeinitLowerRoot(ObContext* context)
{
  sds path = obGetLowerRootPath(context);
  bool result = obMove(path, context->root);
  sdsfree(path);

  return result;
}


bool obDeinitOverlayfs(ObContext* context)
{
  if (context->deviceType == OB_DEV_DIR) {
    obUnmount(context->foundDevicePath);
  }
  return obUnmount(context->root);
}


bool obDeinitManagementBindings(ObContext* context)
{
  sds bindedOverlay = obGetBindedOverlayPath(context);
  sds bindedLayersDir = obGetBindedLayersPath(bindedOverlay);
  sds bindedJobsDir = obGetBindedJobsPath(bindedOverlay);

  bool result = obUnmount(bindedJobsDir);
  result = rmdir(bindedJobsDir) && result;
  result = obUnmount(bindedLayersDir) && result;
  result = rmdir(bindedLayersDir) && result;
  result = obUnmount(bindedOverlay) && result;
  result = rmdir(bindedOverlay) && result;

  sdsfree(bindedJobsDir);
  sdsfree(bindedLayersDir);
  sdsfree(bindedOverlay);
  return result;
}


bool obDeinitFstab(ObContext* context)
{
  sds fstabPath = obGetRootFstabPath(context->root);
  sds origFstabPath = obGetRootFstabBackupPath(fstabPath);

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

