// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBBLKID_H
#define OBBLKID_H

#include <stdbool.h>
#include <stdlib.h>

bool obIsUuid(const char* str);

bool obGetPathByUuid(char* str, size_t size);

#endif // OBBLKID_H
