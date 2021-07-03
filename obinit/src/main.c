// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObArgParser.h"
#include "ob/ObContext.h"
#include "ob/ObLogging.h"
#include "ob/ObMount.h"
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

  if (obFindDevice(context)) {
    obMountDevice(context);
  }
  else {
    obLogE("Device %s not found", context->devicePath);
  }
  obFreeObContext(&context);
  return 0;
}
