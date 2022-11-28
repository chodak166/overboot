// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBYAMLPARSER_H
#define OBYAMLPARSER_H

#include <stdbool.h>

typedef void(*ObYamlValueCallback)(void* context, const char* itemPath, const char* value);
typedef void(*ObYamlEntryCallback)(void* context, const char* itemPath);

bool obParseYamlFile(void* context,
                   const char* path,
                   ObYamlValueCallback valueCallback,
                   ObYamlEntryCallback entryCallback);

#endif // OBYAMLPARSER_H
