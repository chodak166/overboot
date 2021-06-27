// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObArgParser.h"
#include "ob/ObContext.h"
#include "ObYamlConfigReader.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
  ObCliOptions options = obParseArgs(argc, argv);
  if (options.exitProgram) {
    exit(options.exitStatus);
  }

  ObContext* context = obCreateObContext(options.rootPrefix);

  obLoadYamlConfig(context, options.configFile);

  printf("enabled: %i\n", context->enabled);
  printf("use tmpfs: %i\n", context->useTmpfs);
  printf("tmpfs size: %s\n", context->tmpfsSize);
  printf("bind layers: %i\n", context->bindLayers);
  printf("Device path: %s\n", context->devicePath);
  printf("head layer: %s\n", context->headLayer);
  printf("repo: %s\n", context->repository);

  int durablesCount = obCountDurables(context);

  printf("durables (%i):\n", durablesCount);

  ObDurable* durable = context->durable;
  while (durable != NULL) {
    printf("path: %s, copy_origin: %i\n", durable->path, durable->copyOrigin);
    durable = durable->next;
  }

  obFreeObContext(&context);
  return 0;
}
