// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#ifndef OBPATHS_H
#define OBPATHS_H

#include "ob/ObContext.h"
#include "sds.h"
#include <stdlib.h>

sds obGetRepoPath(const ObContext* context);

sds obGetLowerRootPath(const ObContext* context);

sds obGetPersistentUpperPath(const ObContext* context);

sds obGetUpperPath(const ObContext* context);

sds obGetBindedUpperPath(const ObContext* context);

sds obGetOverlayWorkPath(const ObContext* context);

sds obGetBindedOverlayPath(const ObContext* context);

sds obGetBindedLayersPath(const char* bindedOverlay);

sds obGetLayersPath(const ObContext* context);

sds obGetJobsPath(const ObContext* context);

sds obGetBindedJobsPath(const char* bindedOverlay);

sds obGetRootFstabPath(const char* rootmnt);

sds obGetRootFstabBackupPath(const char* fstabPath);

sds obGetLockFilePath(const ObContext* context);

#endif // OBPATHS_H
