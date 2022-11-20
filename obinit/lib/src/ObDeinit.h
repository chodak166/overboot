// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

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
