// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBBLKID_H
#define OBBLKID_H

#include <stdbool.h>
#include <stdlib.h>

bool obIsUuid(const char* str);

bool obGetPathByUuid(char* str, size_t size);

#endif // OBBLKID_H
