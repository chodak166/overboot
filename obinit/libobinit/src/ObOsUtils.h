// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBOSUTILS_H
#define OBOSUTILS_H

#include <stdbool.h>
#include <sys/types.h>

int obMkpath(const char *path, mode_t mode);
bool obIsFile(const char* path);
bool obIsBlockDevice(const char* path);
bool obIsDirectory(const char* path);

#endif // OBOSUTILS_H
