// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObPaths.h"

sds obGetRepoPath(const ObContext* context)
{
  sds repoPath = sdsempty();
  return sdscatprintf(repoPath, "%s/%s", context->devMountPoint, context->config.repository);
}

sds obGetLowerRootPath(const ObContext* context)
{
  sds lowerPath = sdsnew(context->overbootDir);
  return sdscat(lowerPath, "/lower-root");
}

sds obGetUpperPath(const ObContext* context)
{
  sds upperPath = NULL;
  if (context->config.useTmpfs) {
    upperPath = sdsnew(context->overbootDir);
  }
  else {
    sds repoPath = obGetRepoPath(context);
    upperPath = sdsnew(repoPath);
    sdsfree(repoPath);
  }
  return sdscat(upperPath, "/upper");
}

sds obGetBindedUpperPath(const ObContext* context)
{
  sds bindedOverlay = obGetBindedOverlayPath(context);
  sds bindedUpper = sdsnew(bindedOverlay);
  bindedUpper = sdscat(bindedUpper, "/upper");
  sdsfree(bindedOverlay);
  return bindedUpper;
}

sds obGetOverlayWorkPath(const ObContext* context)
{
  sds workPath = NULL;
  if (context->config.useTmpfs) {
    workPath = sdsnew(context->overbootDir);
  }
  else {
    sds repoPath = obGetRepoPath(context);
    workPath = sdsnew(repoPath);
    sdsfree(repoPath);
  }
  return sdscat(workPath, "/work");
}


sds obGetBindedOverlayPath(const ObContext* context)
{
  sds bindedOverlay = sdsempty();
  return sdscatprintf(bindedOverlay, "%s/%s",
                      context->root, OB_USER_BINDINGS_DIR);
}

sds obGetJobsPath(const ObContext* context)
{
  sds path = obGetRepoPath(context);
  path = sdscatfmt(path, "/%s", OB_JOBS_DIR_NAME);
  return path;
}

sds obGetBindedJobsPath(const char* bindedOverlay)
{
  sds bindedJobsDir = sdsnew(bindedOverlay);
  return sdscatfmt(bindedJobsDir, "/%s", OB_JOBS_DIR_NAME);
}

sds obGetBindedLayersPath(const char* bindedOverlay)
{
  sds bindedLayersDir = sdsnew(bindedOverlay);
  return sdscat(bindedLayersDir, "/layers");
}

sds obGetLayersPath(const ObContext* context)
{
  sds path = obGetRepoPath(context);
  return sdscatfmt(path, "/%s", OB_LAYERS_DIR_NAME);
}

sds obGetRootFstabPath(const char* rootmnt)
{
  sds fstabPath = sdsnew(rootmnt);
  return sdscat(fstabPath, "/etc/fstab");
}

sds obGetRootFstabBackupPath(const char* fstabPath)
{
  sds backupPath = sdsnew(fstabPath);
  return sdscat(backupPath, ".orig");
}


