// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObJobs.h"
#include "ob/ObLogging.h"
#include "ObPaths.h"
#include "ObOsUtils.h"

#include "ObYamlLayerReader.h"

#include <string.h>

#define JOB_COMMIT_NAME "commit"
#define JOB_UPDATE_CONFIG_NAME "update-config"
#define JOB_INSTALL_CONFIG_PREFIX "install-config"

static bool obExecUpdateConfigJob(ObContext* context, const char* jobsDir)
{
  sds jobPath = sdsnew(jobsDir);
  jobPath = sdscatfmt(jobPath, "/%s", JOB_UPDATE_CONFIG_NAME);
  bool result = true;

  if (obExists(jobPath)) {
    obLogI("Config update job found in: %s", jobPath);
    result = obCopyFile(jobPath, context->config.configPath);
    result = obRemovePath(jobPath) && result;
    context->reloadConfig = true;
  }

  sdsfree(jobPath);
  return result;
}

//static bool obExecInstallConfigJob(ObContext* context)
//{

//  return true;
//}

//TODO: move?
static bool commitUpperLayer(ObContext* context, const char* jobPath)
{
  ObLayerInfo info;
  obLoadLayerInfoYaml(jobPath, &info);

  if (strlen(info.name) == 0) {
    obLogE("Wrong layer name, aborting the commit");
    return false;
  }

  bool result = true;
  sds upperPath = obGetUpperPath(context);
  sds newLayerPath = obGetLayersPath(context);
  newLayerPath = sdscatfmt(newLayerPath, "/%s", info.name);

  if (obExists(newLayerPath)) {
    obLogE("Layer named %s already exists in %s", info.name, newLayerPath);
    result = false;
  }
  else {
    sds path = sdsnew(newLayerPath);
    path = sdscat(path, "/root");
    result = obSync(upperPath, path);

    path = sdscat(path, OB_LAYER_INFO_PATH);
    result = result && obCopyFile(jobPath, path);
    sdsfree(path);

    if (result) {
      result = obRemoveDirR(upperPath);
      obMkpath(upperPath, OB_MKPATH_MODE);
    }
  }

  sdsfree(upperPath);
  sdsfree(newLayerPath);
  return result;
}

static bool obExecCommitJob(ObContext* context, const char* jobsDir)
{
  sds jobPath = sdsnew(jobsDir);
  jobPath = sdscatfmt(jobPath, "/%s", JOB_COMMIT_NAME);
  bool result = true;

  if (obExists(jobPath)) {
    obLogI("Commit job found in: %s", jobPath);

    if (!commitUpperLayer(context, jobPath)) {
      result = false;
    }
    else {
      result = obRemovePath(jobPath);
    }
  }

  sdsfree(jobPath);
  return result;
}

// --------- public API ---------- //

bool obExecPreInitJobs(ObContext* context)
{
  sds jobsDir = obGetJobsPath(context);
  obExecCommitJob(context, jobsDir);
  obExecUpdateConfigJob(context, jobsDir);
  sdsfree(jobsDir);

  if (context->reloadConfig) {
    obLogI("Configuration file requires reloading");
    return false;
  }

  return true;
}
