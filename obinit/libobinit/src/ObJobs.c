// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObJobs.h"
#include "ob/ObLogging.h"
#include "ObPaths.h"
#include "ObOsUtils.h"
#include "ObMount.h"

#include "ObYamlLayerReader.h"

#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>

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
    sync();

    result = obRemovePath(jobPath) && result;
    context->reloadConfig = true;
  }

  sdsfree(jobPath);
  return result;
}


static bool obInstallParitalConfig(const char* jobPath, const char* dir)
{
  if (!dir || strlen(dir) == 0) {
    obLogE("Configuration dir not set");
    return false;
  }

  if (!obExists(dir)) {
    obMkpath(dir, OB_MKPATH_MODE);
  }

  bool result = true;
  sds nameBuffer = sdsnew(jobPath);
  const char* name = basename(nameBuffer) + strlen(JOB_INSTALL_CONFIG_PREFIX) + 1;

  sds partialConfigFile = sdsnew(dir);
  partialConfigFile = sdscatfmt(partialConfigFile, "/%s", name);

  obLogI("Installing partial config in %s", partialConfigFile);
  result = obCopyFile(jobPath, partialConfigFile)
      && obRemovePath(jobPath);

  sdsfree(nameBuffer);
  sdsfree(partialConfigFile);
  return result;
}

static int configInstgallFilter(const struct dirent* entry)
{
  if (strncmp(entry->d_name, JOB_INSTALL_CONFIG_PREFIX,
              strlen(JOB_INSTALL_CONFIG_PREFIX)) == 0) {
    return 1;
  }
  return 0;
}

static bool obExecInstallConfigJob(ObContext* context, const char* jobsDir)
{
  bool result = true;
  struct dirent **namelist;
  int n = scandir(jobsDir, &namelist, configInstgallFilter, alphasort);
  if (n == -1) {
    obLogE("Cannot open directory: %s", jobsDir);
    result = false;
  }
  else {
    sds buffer = sdsnew(context->config.configPath);
    char* configDirname = dirname(buffer);
    sds configDirPath = sdsempty();
    configDirPath = sdscatfmt(configDirPath, "%s/%s", configDirname, context->config.configDir);

    for (int i = 0; i < n; ++i) {
        sds fullPath = sdsempty();
        fullPath = sdscatfmt(fullPath, "%s/%s", jobsDir, namelist[i]->d_name);
        result = obInstallParitalConfig(fullPath, configDirPath) && result;
        sdsfree(fullPath);
        free(namelist[i]);
    }

    free(namelist);
    sdsfree(buffer);
    sdsfree(configDirPath);
  }

  return result;
}

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
  newLayerPath = sdscatfmt(newLayerPath, "/%s.obld", info.name);

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
  bool result = true;
  sds jobPath = sdsnew(jobsDir);
  jobPath = sdscatfmt(jobPath, "/%s", JOB_COMMIT_NAME);

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

  obLogI("Looking for pre-init jobs to be executed");
  sds jobsDir = obGetJobsPath(context);

  if (!obExists(jobsDir)) {
    bool result = obMkpath(jobsDir, OB_MKPATH_MODE);
    sdsfree(jobsDir);
    return result;
  }

  if (obIsDirectoryEmpty(jobsDir)) {
    sdsfree(jobsDir);
    return true;
  }
  obRemountRw(context->root, NULL);

  bool result = obExecCommitJob(context, jobsDir)
           && obExecUpdateConfigJob(context, jobsDir);

  if (context->reloadConfig) {
    result = false;
  }
  else {
    result = result && obExecInstallConfigJob(context, jobsDir);
    if (result && context->reloadConfig) {
      result = false;
    }
  }

  sdsfree(jobsDir);
  return result;
}
