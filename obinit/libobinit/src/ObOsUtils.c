// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObOsUtils.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

static int obMkdir(const char *path, mode_t mode)
{
    struct stat st;
    int status = 0;

    if (stat(path, &st) != 0) {
        if (mkdir(path, mode) != 0 && errno != EEXIST) {
            status = -1;
        }
    }
    else if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        status = -1;
    }

    return status;
}

int obMkpath(const char* path, mode_t mode)
{
  char* origPath = strdup(path);
  int status = 0;
  char* searchedPath = origPath;
  char* slashPos = NULL;
  while (status == 0 && (slashPos = strchr(searchedPath, '/')) != NULL) {
    if (slashPos != searchedPath) {
        *slashPos = '\0';
        status = obMkdir(origPath, mode);
        *slashPos = '/';
    }
    searchedPath = slashPos + 1;
  }

  if (status == 0) {
      status = obMkdir(path, mode);
  }

  free(origPath);
  return status;
}
