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


int main(int argc, char* argv[])
{
  obInitLogger(OB_LOG_USE_STD, OB_LOG_USE_KMSG);

  ObCliOptions options = obParseArgs(argc, argv);
  if (options.exitProgram) {
    exit(options.exitStatus);
  }

  ObContext* context = obCreateObContext(options.rootPrefix);
  ObConfig* config = &context->config;
  obLoadYamlConfig(config, options.configFile);
  obLogObContext(context);

  int exitCode = EXIT_SUCCESS;
  if (!obExecObInitTasks(context)) {
    exitCode = EXIT_FAILURE;
  }

  obFreeObContext(&context);

  obLogI("Overboot initialization sequence finished with exit code %i", exitCode);
  return exitCode;
}
