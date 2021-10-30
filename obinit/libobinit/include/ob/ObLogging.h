// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBLOGGING_H
#define OBLOGGING_H

#include <stdbool.h>

void obInitLogger(bool stdOut, bool kmsgOut);

void obLogI(const char* msg, ...);
void obLogW(const char* msg, ...);
void obLogE(const char* msg, ...);

bool obErrorOccurred();
void clearErrorOccurrence();

#endif // OBLOGGING_H
