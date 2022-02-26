// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBHASH_H
#define OBHASH_H

#include <inttypes.h>
#include <stdbool.h>

uint64_t obCalcualateFileHash(const char* path);

bool obWriteAsHexStr(uint64_t value, const char* outputPath);

uint64_t obReadHashValue(const char* txtFilePath);

#endif // OBHASH_H
