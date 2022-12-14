// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBARGPARSER_H
#define OBARGPARSER_H

#include <stdbool.h>

#define OB_CLI_PATH_MAX 255

typedef struct ObCliOptions
{
  char rootPrefix[OB_CLI_PATH_MAX];
  char configFile[OB_CLI_PATH_MAX];
  int exitStatus;
  bool exitProgram;
} ObCliOptions;

ObCliOptions obParseArgs(int argc, char* argv[]);

#endif // OBARGPARSER_H
