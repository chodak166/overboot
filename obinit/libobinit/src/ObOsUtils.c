// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#define _GNU_SOURCE // TODO: move to cmake


#include "ob/ObOsUtils.h"
#include "ob/ObLogging.h"
#include "ob/ObDefs.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <libgen.h>

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

  if (remove(path) < 0) {
    obLogE("Cannot remove %s: %s", path, strerror(errno));
    return -1;
  }
  return 0;
}

static const char* syncSrcDir = NULL;
static const char* syncDstDir = NULL;

static int obSyncCb(const char* path, const struct stat* sbuf, int type, struct FTW* ftwb)
{
  UNUSED(sbuf);
  UNUSED(type);
  UNUSED(ftwb);

  char toPath[OB_PATH_MAX];
  strcpy(toPath, syncDstDir);
  strcat(toPath, "/");
  strcat(toPath, path + strlen(syncSrcDir));

  if (type == FTW_D) {
    obMkpath(toPath, OB_MKPATH_MODE);
  }
  else if (type == FTW_F) {
    obCopyFile(path, toPath);
  }
  else if (type == FTW_SL) {
    char linkTarget[OB_PATH_MAX] = {0};
    readlink(path, linkTarget, OB_PATH_MAX);
    symlink(linkTarget, toPath);
  }
  else {
    obLogE("Type %i not supported (%s)", type, path);
  }

  return 0;
}

static bool obEnsureParentExists(const char* path)
{
  char pathCpy[OB_PATH_MAX];
  strcpy(pathCpy, path);
  char* parentDir = dirname(pathCpy);

  if (!obExists(parentDir)) {
    obLogI("Parent dir does not exists, creating %s", parentDir);
    return obMkpath(parentDir, OB_MKPATH_MODE) == 0;
  }
  return true;
}


// --------- public API ---------- //

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
  if (status != 0) {
    obLogE("Cannot create path %s" , path);
  }
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

bool obCreateBlankFile(const char* path)
{
  if (!obEnsureParentExists(path)) {
    return false;
  }

  obLogI("Creating blank file: %s", path);
  FILE* fp = fopen(path, "w");
  if (fp == NULL || ftruncate(fileno(fp), 0) != 0) {
    obLogE("Cannot create %s: %s", path, strerror(errno));
    fclose(fp);
    return false;
  }
  fclose(fp);
  return true;
}

bool obCopyFile(const char* src, const char* dst)
{
  if (!obEnsureParentExists(dst)) {
    return false;
  }

  int fdIn = open(src, O_RDONLY);
  if (fdIn == -1) {
    obLogE("Cannot open source file %s: %s", src, strerror(errno));
    return false;
  }

  struct stat stat;
  if (fstat(fdIn, &stat) == -1) {
    obLogE("Cannot stat %s", src);
    return false;
  }

  off_t len = stat.st_size;

  int fdOut = open(dst, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (fdOut == -1) {
    obLogE("Cannot open destination file %s: %s", dst, strerror(errno));
    return false;
  }

  off_t ret;
  do {
    ret = copy_file_range(fdIn, NULL, fdOut, NULL, len, 0);
    if (ret == -1) {
      obLogE("Cannot copy %s to %s: %s", src, dst, strerror(errno));
      return false;
    }

    len -= ret;
  } while (len > 0 && ret > 0);

  close(fdIn);
  close(fdOut);
  return true;
}

bool obSync(const char* src, const char* dst)
{
  syncSrcDir = src;
  syncDstDir = dst;
  bool result = nftw(src, obSyncCb, 10, FTW_MOUNT | FTW_PHYS) >= 0;
  syncSrcDir = NULL;
  syncDstDir = NULL;
  return result;
}
