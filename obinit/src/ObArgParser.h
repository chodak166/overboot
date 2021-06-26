// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBARGPARSER_H
#define OBARGPARSER_H

#include <stdbool.h>

#define OB_CLI_PATH_MAX 255

typedef struct ObCliOptions
{
  char root[OB_CLI_PATH_MAX];
  char configFile[OB_CLI_PATH_MAX];
  int exitStatus;
  bool exitProgram;
} ObCliOptions;

ObCliOptions obParseArgs(int argc, char* argv[]);

#endif // OBARGPARSER_H
