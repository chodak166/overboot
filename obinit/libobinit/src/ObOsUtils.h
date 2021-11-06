// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBOSUTILS_H
#define OBOSUTILS_H

#include <stdbool.h>
#include <sys/types.h>

bool obMkpath(const char *path, mode_t mode);
bool obExists(const char* path);
bool obIsFile(const char* path);
bool obIsBlockDevice(const char* path);
bool obIsDirectory(const char* path);
bool obIsDirectoryEmpty(const char* path);
bool obRemoveDirR(const char* path);
bool obRemovePath(const char* path);
bool obCreateBlankFile(const char* path);
bool obCopyFile(const char* src, const char* dst);
bool obSync(const char* src, const char* dst);

#endif // OBOSUTILS_H
