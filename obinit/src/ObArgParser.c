// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObArgParser.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define APP_NAME "obinit"
#define OB_DEFAULT_ROOT "/"
#define OB_DEFAULT_CONFIG_FILE "/rootmnt/etc/overboot.yaml"

static void printVersion()
{
  char buffer[12];
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
  strcpy(options.configFile, OB_DEFAULT_CONFIG_FILE);
  strcpy(options.root, OB_DEFAULT_ROOT);

  char c = -1;
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
        break;
      case 'r':
        strncpy(options.root, optarg, OB_CLI_PATH_MAX);
        break;
      default:
        break;
      }
    }
    else {
      fprintf(stderr, "Unknown argument: %s\n", argv[optind]);
      printUsage();
      options.exitStatus = EXIT_FAILURE;
      break;
    }
  }
  return options;
}
