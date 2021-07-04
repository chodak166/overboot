// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#include "ob/ObMount.h"
#include "ob/ObLogging.h"
#include "ob/ObOsUtils.h"

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

//TODO: add info logs

//TODO: split it
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
      obLogE("Cannot mount %s in %s: %s",
             context->devicePath, context->devMountPoint, strerror(errno));
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
        obLogE("Mounting %s in %s failed with error: %s",
               loopDevice, context->devMountPoint, strerror(errno));
    }

    obFreeLoopDevice(loopDeviceFd);
  }

  return true;
}

bool obUnmount(const char* path)
{
  int result = umount2(path, MNT_DETACH);
  if (result != 0) {
    obLogE("Cannot unmount %s: %s", path, strerror(errno));
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

  obLogI("Binding %s to %s", srcPath, dstPath);
  int result = mount(srcPath, dstPath, NULL, MS_BIND | MS_REC, "");
  if (result != 0) {
    obLogE("Cannot bind %s: %s", srcPath, strerror(errno));
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
    obLogE("Cannot move %s: %s", srcPath, strerror(errno));
    return false;
  }

  return true;
}

bool obMountTmpfs(const char* path, const char* sizeStr)
{
  if (obMkpath(path, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  char options[16];
  sprintf(options, "size=%s", sizeStr);
  int result = mount("tmpfs", path, "tmpfs", 0, options);
  if (result != 0) {
    obLogE("Cannot mount tmpfs (options: %s) in %s: %s", options, path, strerror(errno));
    return false;
  }
  return true;
}

int obMountLoopDevice(const char* imagePath, char* loopDevice)
{
  int controlFd = open("/dev/loop-control", O_RDWR);
  if (controlFd < 0) {
      obLogE("Opening loop control device failed");
      return 0;
  }

  int loopId = ioctl(controlFd, LOOP_CTL_GET_FREE);
  sprintf(loopDevice, "/dev/loop%d", loopId);
  close(controlFd);

  obLogI("Using loop device: %s", loopDevice);

  int imageFd = open(imagePath, O_RDWR);
  if (imageFd < 0) {
      obLogE("Opening image file failed");
      return 0;
  }

  int deviceFd = open(loopDevice, O_RDWR);
  if (deviceFd < 0) {
      obLogE("Opening loop device failed");
      close(imageFd);
      return 0;
  }

  if (ioctl(deviceFd, LOOP_SET_FD, imageFd) < 0) {
      obLogE("ioctl LOOP_SET_FD failed");
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
    obLogI("Layer: %s", layers[i]);
    strcat(lowerLayers, layers[i]);
    if (i != 0) {
      strcat(lowerLayers, ":");
    }
  }

  char options[OB_DEV_PATH_MAX * (layerCount+2)];
  sprintf(options, "lowerdir=%s,upperdir=%s,workdir=%s", lowerLayers, upper, work);

  if (obMkpath(mountPoint, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  if (obMkpath(work, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  int result = mount("overlay", mountPoint, "overlay", 0, options);
  if (result != 0) {
    obLogE("Cannot mount %s: %s", mountPoint, strerror(errno));
  }
  else {
    obLogI("OVERLAY MOUNTED");
  }

  return result == 0;
}

//TODO check mkpath status
bool obPrepareOverlay(const ObContext* context)
{
  obMkpath(context->overlayDir, OB_MKPATH_MODE);
  obMountTmpfs(context->overlayDir, context->tmpfsSize);

  char path[OB_DEV_PATH_MAX];
  sprintf(path, "%s/work", context->overlayDir);
  obMkpath(path, OB_MKPATH_MODE);

  sprintf(path, "%s/lower", context->overlayDir);
  obMkpath(path, OB_MKPATH_MODE);

  sprintf(path, "%s/upper", context->overlayDir);
  obMkpath(path, OB_MKPATH_MODE);

  return true;
}
