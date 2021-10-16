// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBDEINIT_H
#define OBDEINIT_H

#include "ob/ObContext.h"

bool obDeinitPersistentDevice(ObContext* context);

bool obDeinitOverbootDir(ObContext* context);

bool obDeinitLowerRoot(ObContext* context);

bool obDeinitOverlayfs(ObContext* context);

bool obDeinitManagementBindings(ObContext* context);

bool obDeinitFstab(ObContext* context);

bool obDeinitDurables(ObContext* context);

#endif // OBDEINIT_H
