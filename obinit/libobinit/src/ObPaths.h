// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBPATHS_H
#define OBPATHS_H

#include "ob/ObContext.h"
#include "sds.h"
#include <stdlib.h>

sds obGetRepoPath(const ObContext* context);

sds obGetLowerRootPath(const ObContext* context);

sds obGetUpperPath(const ObContext* context);

sds obGetBindedUpperPath(const ObContext* context);

sds obGetOverlayWorkPath(const ObContext* context);

sds obGetBindedOverlayPath(const ObContext* context);

sds obGetBindedLayersPath(const ObContext* context);

sds obGetLayersPath(const ObContext* context);

sds obGetJobsPath(const ObContext* context);

sds obGetRootFstabPath(const char* rootmnt);

sds obGetRootFstabBackupPath(const char* fstabPath);


#endif // OBPATHS_H
