// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ObArgParser.h"
#include "Version.h"

#include "ob/ObLogging.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define APP_NAME "obinit"
#define OB_DEFAULT_ROOT_PREFIX ""
#define OB_DEFAULT_CONFIG_FILE "/root/etc/overboot.yaml"

static void printVersion()
{
  char buffer[24];
  printf("%s %s\n", APP_NAME, getVersionString(buffer));
}

static void printUsage()
{
  printf("Usage: %s [-h][-v][-r root_path][-c config_file]\n", APP_NAME);
}

ObCliOptions obParseArgs(int argc, char* argv[])
{
  ObCliOptions options;
  options.exitProgram = false;
  options.exitStatus = EXIT_SUCCESS;
  strcpy(options.rootPrefix, OB_DEFAULT_ROOT_PREFIX);

  char c = -1;
  bool isConfigSet = false;
  while (optind < argc) {
    if ((c = getopt(argc, argv, "vhr:c:")) != -1) {
      switch (c) {
      case 'v': {
        printVersion();
        options.exitProgram = true;
        optind = argc;
        break;
      }
      case 'h': {
        printUsage();
        options.exitProgram = true;
        optind = argc;
        break;
      }
      case 'c':
        strncpy(options.configFile, optarg, OB_CLI_PATH_MAX);
        isConfigSet = true;
        break;
      case 'r':
        strncpy(options.rootPrefix, optarg, OB_CLI_PATH_MAX);
        break;
      default:
        break;
      }
    }
    else {
      obLogE("Unknown argument: %s", argv[optind]);
      printUsage();
      options.exitStatus = EXIT_FAILURE;
      break;
    }
  }

  if (!isConfigSet) {
    strcpy(options.configFile, options.rootPrefix);
    strcat(options.configFile, OB_DEFAULT_CONFIG_FILE);
  }

  return options;
}
