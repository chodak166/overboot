// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBMOUNT_H
#define OBMOUNT_H

#include <stdbool.h>
#include "ob/ObConfig.h"

bool obMountDevice(const char* device, const char* mountPoint);

bool obUnmount(const char* path);

bool obUnmountDevice(const char* mountPoint);

bool obRbind(const char* srcPath, const char* dstPath);

bool obMove(const char* srcPath, const char* dstPath);

bool obMountTmpfs(const char* path, const char* sizeStr);

int obMountLoopDevice(const char* imagePath, char* loopDevice);

void obFreeLoopDevice(int deviceFd);

bool obMountOverlay(char** layers, int layerCount, const char* upper,
                    const char* work, const char* mountPoint);

#endif // OBMOUNT_H
