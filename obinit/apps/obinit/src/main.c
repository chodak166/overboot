// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObArgParser.h"
#include "ob/ObInitTasks.h"
#include "ob/ObLogging.h"
#include "ob/ObYamlConfigReader.h"

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

#define OB_MAX_CONFIG_RELOADS 8

static void loadContext(ObContext** context, const ObCliOptions* options)
{
  if (*context) {
    obFreeObContext(context);
  }
  *context = obCreateObContext(options->rootPrefix);
  obLoadYamlConfig(&(*context)->config, options->configFile);
  obLogObContext(*context);
}

int main(int argc, char* argv[])
{
  obInitLogger(OB_LOG_USE_STD, OB_LOG_USE_KMSG);

  ObCliOptions options = obParseArgs(argc, argv);
  if (options.exitProgram) {
    exit(options.exitStatus);
  }

  ObContext* context = NULL;
  int exitCode = EXIT_SUCCESS;
  size_t maxReloads = OB_MAX_CONFIG_RELOADS;
  do {
    if (context && context->reloadConfig) {
      obLogI("Reloading configuration file due to the config-update job execution");
      maxReloads -= 1;
    }

    loadContext(&context, &options);
    if (context->config.enabled) {
      exitCode = obExecObInitTasks(context) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
  } while (maxReloads > 0
           && context->reloadConfig
           && context->config.enabled);

  if (!obErrorOccurred()
      && maxReloads != OB_MAX_CONFIG_RELOADS
      && !context->config.enabled) {
    exitCode = EXIT_SUCCESS;
  }

  obFreeObContext(&context);

  obLogI("Overboot initialization sequence finished with exit code %i", exitCode);
  return exitCode;
}
