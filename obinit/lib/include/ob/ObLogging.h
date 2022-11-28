// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

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
