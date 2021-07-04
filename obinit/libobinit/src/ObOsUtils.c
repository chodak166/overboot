// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObOsUtils.h"
#include "ob/ObLogging.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define __USE_XOPEN_EXTENDED
#include <ftw.h>

#define UNUSED(x) (void)(x)

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

static int obRmPath(const char* path, const struct stat* sbuf, int type, struct FTW* ftwb)
{
  UNUSED(sbuf);
  UNUSED(type);
  UNUSED(ftwb);

  if (unlink(path) < 0) {
    obLogE("Cannot remove %s: %s", path, strerror(errno));
    return -1;
  }
  return 0;
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

bool obExists(const char* path)
{
  return access(path, F_OK) != -1;
}

bool obIsFile(const char* path)
{
  struct stat fileStat;
  int exist = stat(path, &fileStat);
  if (exist == 0 && (fileStat.st_mode & S_IFREG)) {
    return 1;
  }
  else {
    return 0;
  }
}

bool obIsBlockDevice(const char* path)
{
  struct stat fileStat;
  int exist = stat(path, &fileStat);
  if (exist == 0 && (fileStat.st_mode & S_IFBLK)) {
    return 1;
  }
  else {
    return 0;
  }
}

bool obIsDirectory(const char* path)
{
  struct stat fileStat;
  int exist = stat(path, &fileStat);
  if (exist == 0 && (fileStat.st_mode & S_IFDIR)) {
    return 1;
  }
  else {
    return 0;
  }
}


bool obRemoveDirR(const char* path)
{
  return nftw(path, obRmPath, 10, FTW_DEPTH | FTW_MOUNT | FTW_PHYS) >= 0;
}
