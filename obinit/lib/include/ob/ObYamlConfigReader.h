// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBYAMLCONFIGREADER_H
#define OBYAMLCONFIGREADER_H

#include <stdbool.h>

typedef struct ObConfig ObConfig;

bool obLoadYamlConfig(ObConfig* config, const char* path);

#endif // OBYAMLCONFIGREADER_H
