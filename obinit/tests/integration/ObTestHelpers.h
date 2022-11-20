// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBTESTHELPERS_H
#define OBTESTHELPERS_H

#include <stdbool.h>

bool obConcatPaths(char* result, const char* pathA, const char* pathB);

char* obGetSelfPath(char* buffer, int size);

char* obReadFile(const char* path, char* content);

void obCreateFile(const char* path, const char* content);


#endif // OBTESTHELPERS_H
