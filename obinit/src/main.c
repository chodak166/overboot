// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObArgParser.h"
#include "ob/ObContext.h"
#include "ob/ObLogging.h"
#include "ob/ObMount.h"
#include "ob/ObOsUtils.h"
#include "ObYamlConfigReader.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef OB_LOG_STDOUT
# define OB_LOG_USE_STD true
#else
# define OB_LOG_USE_STD false
#endif


#ifdef OB_LOG_KMSG
# define OB_LOG_USE_KMSG true
#else
# define OB_LOG_USE_KMSG false
#endif

int main(int argc, char* argv[])
{
  obInitLogger(OB_LOG_USE_STD, OB_LOG_USE_KMSG);

  ObCliOptions options = obParseArgs(argc, argv);
  if (options.exitProgram) {
    exit(options.exitStatus);
  }

  ObContext* context = obCreateObContext(options.rootPrefix);

  obLoadYamlConfig(context, options.configFile);

  obLogI("enabled: %i", context->enabled);
  obLogI("use tmpfs: %i", context->useTmpfs);
  obLogI("tmpfs size: %s", context->tmpfsSize);
  obLogI("bind layers: %i", context->bindLayers);
  obLogI("Device path: %s", context->devicePath);
  obLogI("head layer: %s", context->headLayer);
  obLogI("repo: %s", context->repository);

  int durablesCount = obCountDurables(context);

  obLogI("durables (%i):", durablesCount);

  ObDurable* durable = context->durable;
  while (durable != NULL) {
    obLogI("path: %s, copy_origin: %i", durable->path, durable->copyOrigin);
    durable = durable->next;
  }

  if (!obFindDevice(context)) {
    obLogE("Device %s not found", context->devicePath);
    return EXIT_FAILURE;
  }

  if (!obMountDevice(context)) {
    obLogE("Device mount (%s -> %s) failed", context->devicePath, context->devMountPoint);
    return EXIT_FAILURE;
  }

\
  if (!obPrepareOverlay(context)) {
    obLogE("Cannot prepare overlay dir (%s)", context->overlayDir);
    return EXIT_FAILURE;
  }

  if (!context->useTmpfs) {
    char srcPath[OB_PATH_MAX];
    sprintf(srcPath, "%s/%s/upper", context->devMountPoint, context->repository);

    if (context->clearUpper) {
      obRemoveDirR(srcPath);
      obMkpath(srcPath, OB_MKPATH_MODE);
    }
    else if (!obExists(srcPath)) {
      obMkpath(srcPath, OB_MKPATH_MODE);
    }

    char dstPath[OB_DEV_PATH_MAX];
    sprintf(dstPath, "%s/upper", context->overlayDir);
    obRbind(srcPath, dstPath);
  }

  obFreeObContext(&context);
  return 0;
}
