// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObMount.h"
#include "ob/ObLogging.h"
#include "ObOsUtils.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <linux/loop.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>

bool obMountDevice(const ObContext* context)
{
  int result = obMkpath(context->devMountPoint, OB_DEV_MOUNT_MODE);
  if (result != 0) {
    obLogE("Cannot create %s: %s", context->devMountPoint, strerror(errno));
    return false;
  }

  obLogI("Directory %s created", context->devMountPoint);

  struct stat devStat;
  result = lstat(context->devicePath, &devStat);
  if (result != 0) {
    obLogE("Cannot stat %s: %s", context->devicePath, strerror(errno));
    return false;
  }

  if (S_ISBLK(devStat.st_mode)) {
    obLogI("%s is a block device", context->devicePath);
    result = mount(context->devicePath, context->devMountPoint,
                   OB_DEV_IMAGE_FS, OB_DEV_MOUNT_FLAGS, OB_DEV_MOUNT_OPTIONS);
    if (result != 0) {
      obLogE("Cannot mount %s: %s", context->devicePath, strerror(errno));
      return false;
    }

  }
  else if (S_ISREG(devStat.st_mode)){
    obLogI("%s is a regular file", context->devicePath);

    char loopDevice[OB_DEV_PATH_MAX];
    int loopDeviceFd = obMountLoopDevice(context->devicePath, loopDevice);

    if (loopDeviceFd < 0) {
      obLogE("Loop device mount failed");
      return false;
    }

    if (mount(loopDevice, context->devMountPoint, OB_DEV_IMAGE_FS,
              OB_DEV_MOUNT_FLAGS, OB_DEV_MOUNT_OPTIONS) < 0) {
        obLogE("Mount failed");
    } else {
        obLogI("Mount successful");
    }

    obFreeLoopDevice(loopDeviceFd);
  }

  return true;
}

bool obUnmount(const char* path)
{
  int result = umount2(path, MNT_DETACH);
  if (result != 0) {
    fprintf(stderr, "Cannot unmount %s: %s\n", path, strerror(errno));
    return false;
  }
  return true;
}

bool obUnmountDevice(ObContext* context)
{
  return obUnmount(context->devMountPoint);
}

bool obRbind(const char* srcPath, const char* dstPath)
{
  if (obMkpath(dstPath, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  int result = mount(srcPath, dstPath, NULL, MS_BIND | MS_REC, "");
  if (result != 0) {
    fprintf(stderr, "Cannot bind %s: %s\n", srcPath, strerror(errno));
    return false;
  }

  return true;
}

bool obMove(const char* srcPath, const char* dstPath)
{
  if (obMkpath(dstPath, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  int result = mount(srcPath, dstPath, NULL, MS_MOVE, "");
  if (result != 0) {
    fprintf(stderr, "Cannot move %s: %s\n", srcPath, strerror(errno));
    return false;
  }

  return true;
}

bool obMountTmpfs(const char* path)
{
  if (obMkpath(path, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  int result = mount("tmpfs", path, "tmpfs", 0, "");
  if (result != 0) {
    fprintf(stderr, "Cannot mount tmpfs in %s: %s\n", path, strerror(errno));
    return false;
  }
  return true;
}

int obMountLoopDevice(const char* imagePath, char* loopDevice)
{
  int controlFd = open("/dev/loop-control", O_RDWR);
  if (controlFd < 0) {
      perror("open loop control device failed");
      return 0;
  }

  int loopId = ioctl(controlFd, LOOP_CTL_GET_FREE);
  sprintf(loopDevice, "/dev/loop%d", loopId);
  close(controlFd);

  printf("using loop device: %s\n", loopDevice);

  int imageFd = open(imagePath, O_RDWR);
  if (imageFd < 0) {
      perror("open backing file failed");
      return 0;
  }

  int deviceFd = open(loopDevice, O_RDWR);
  if (deviceFd < 0) {
      perror("open loop device failed");
      close(imageFd);
      return 0;
  }

  if (ioctl(deviceFd, LOOP_SET_FD, imageFd) < 0) {
      perror("ioctl LOOP_SET_FD failed");
      close(imageFd);
      close(deviceFd);
      return 0;
  }

  close(imageFd);
  return deviceFd;
}

void obFreeLoopDevice(int deviceFd)
{
  ioctl(deviceFd, LOOP_CLR_FD, 0);
  close(deviceFd);
}

bool obMountOverlay(char** layers, int layerCount, const char* upper,
                    const char* work, const char* mountPoint)
{
  char lowerLayers[OB_PATH_MAX * layerCount];
  lowerLayers[0] = '\0';

  for (int i = layerCount -1; i >=0; --i) {
    printf("layer: %s\n", layers[i]);
    strcat(lowerLayers, layers[i]);
    if (i != 0) {
      strcat(lowerLayers, ":");
    }
  }

  char options[OB_DEV_PATH_MAX * (layerCount+2)];
  sprintf(options, "lowerdir=%s,upperdir=%s,workdir=%s", lowerLayers, upper, work);

  printf("options: %s\n", options);

  if (obMkpath(mountPoint, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  if (obMkpath(work, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  int result = mount("overlay", mountPoint, "overlay", 0, options);
  if (result != 0) {
    fprintf(stderr, "Cannot mount %s: %s\n", mountPoint, strerror(errno));
  }
  else {
    printf("OVERLAY MOUNTED\n");
  }

  return result == 0;
}
