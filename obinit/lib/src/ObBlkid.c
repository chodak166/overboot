// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ObBlkid.h"
#include "ob/ObDefs.h"
#include "ob/ObLogging.h"
#include <sds.h>

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
  if (len !=0 && strcmp(usage, "filesystem") == 0) {
    blkid_probe_lookup_value(pr, "UUID", &devUuid, &len);
    obLogI("Partition UUID detected, size: %lu, value: %s\n", len, uuid);
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
  obLogI("Fetching path from device string %s", str);
  struct dirent** namelist;
  int32_t n = scandir(DEV_PATH, &namelist, filterNonBlk, alphasort);
  if (n == -1) {
    obLogE("Cannot scan %s: %s", DEV_PATH, strerror(errno));
    return false;
  }

  bool match = false;
  bool result = false;
  int32_t i = 0;
  for (i = 0; i < n && !match; ++i) {
    sds path = sdsnew(DEV_PATH);
    path = sdscatfmt(path, "/%s", namelist[i]->d_name);

    if (matchUuid(str + strlen(DEV_UUID_PREFIX), path)) {
      match = true;
      if (strlen(path) <= size) {
        strcpy(str, path);
        result = true;
      }
      else {
        obLogE("Device path too long");
      }
    }

    free(namelist[i]);
    namelist[i] = NULL;
    sdsfree(path);
  }

  for (i -= 1; i < n; ++i) {
    free(namelist[i]);
  }
  free(namelist);

  return result;
#endif
}
