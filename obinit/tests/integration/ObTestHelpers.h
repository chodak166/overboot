// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBTESTHELPERS_H
#define OBTESTHELPERS_H

#include <stdbool.h>

bool obConcatPaths(char* result, const char* pathA, const char* pathB);

char* obGetSelfPath(char* buffer, int size);

char* obReadFile(const char* path, char* content);

void obCreateFile(const char* path, const char* content);


#endif // OBTESTHELPERS_H
