// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBINIT_H
#define OBINIT_H

#include "ob/ObContext.h"

bool obInitPersistentDevice(ObContext* context);

bool obInitOverbootDir(ObContext* context);

bool obInitLowerRoot(ObContext* context);

bool obInitOverlayfs(ObContext* context);

bool obInitManagementBindings(ObContext* context);

bool obInitFstab(ObContext* context);

bool obInitDurables(ObContext* context);

bool obInitLock(ObContext* context);

bool obUnsetLock(ObContext* context);



#endif // OBINIT_H
