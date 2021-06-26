// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

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
