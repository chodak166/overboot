// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ObBlkid.h"
#include "ob/ObDefs.h"
#include "ob/ObLogging.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <dirent.h>
#include <sys/stat.h>

#ifdef OB_USE_BLKID
# include <blkid/blkid.h>
#endif

#define DEV_PATH "/dev"
#define DEV_UUID_PREFIX "UUID="


static int filterNonBlk(const struct dirent* entry)
{
  struct stat entryStat;
  char path[OB_DEV_PATH_MAX] = {0};
  sprintf(path, "%s/%s", DEV_PATH, entry->d_name);
  if (stat(path, &entryStat) != 0) {
    return false;
  }
  return S_ISBLK(entryStat.st_mode);
}

static bool matchUuid(const char* uuid, const char* devicePath)
{
  blkid_probe pr = blkid_new_probe_from_filename(devicePath);
  if (!pr) {
    return false;
  }

  blkid_probe_enable_partitions(pr, true);
  blkid_probe_set_superblocks_flags(pr, BLKID_SUBLKS_USAGE | BLKID_SUBLKS_UUID);
  blkid_do_fullprobe(pr);

  const char* usage;
  const char* devUuid;
  size_t len;

  blkid_probe_lookup_value(pr, "USAGE", &usage, &len);

  bool match = false;
  if (strcmp(usage, "filesystem") == 0) {
    blkid_probe_lookup_value(pr, "UUID", &devUuid, &len);
    printf("Partition UUID detected, size: %lu, value: %s\n", len, uuid);
    if (strcmp(uuid, devUuid) == 0) {
      obLogI("UUID %s matched with %s", uuid, devicePath);
      match = true;
    }
  }
  blkid_free_probe(pr);
  return match;
}

// --------- public API ---------- //

bool obIsUuid(const char* str)
{
  size_t prefixLen = strlen(DEV_UUID_PREFIX);
  if (strlen(str) < prefixLen) {
    return false;
  }

  return strncmp(str, DEV_UUID_PREFIX, prefixLen) == 0;
}

bool obGetPathByUuid(char* str, size_t size)
{
#ifndef OB_USE_BLKID
  obLogE("Cannot find device by UUID: this build does not support libblkid");
  return false;
#else

  struct dirent **namelist;
  int n = scandir(DEV_PATH, &namelist, filterNonBlk, alphasort);
  if (n == -1) {
    obLogE("Cannot scan %s: %s", DEV_PATH, strerror(errno));
    return false;
  }

  bool match = false;
  int i = 0;
  for (i = 0; i < n && !match; ++i) {
    char path[OB_DEV_PATH_MAX] = {0};
    sprintf(path, "%s/%s", DEV_PATH, namelist[i]->d_name);
    free(namelist[i]);
    namelist[i] = NULL;

    if (matchUuid(str + strlen(DEV_UUID_PREFIX), path)) {
      if (strlen(path) <= size) {
        strcpy(str, path);
        match = true;
      }
      else {
        obLogE("Device path too long");
        break;
      }
    }
  }

  for (; i < n; ++i) {
    free(namelist[i]);
  }
  free(namelist);

  return match;
#endif
}
