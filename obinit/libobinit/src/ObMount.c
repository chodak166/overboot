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

static bool obMountBlockDevice(const char* device, const char* mountPoint)
{
  int result = mount(device, mountPoint,
                 OB_DEV_IMAGE_FS, OB_DEV_MOUNT_FLAGS, OB_DEV_MOUNT_OPTIONS);
  if (result != 0) {
    obLogE("Cannot mount %s in %s: %s",
           device, mountPoint, strerror(errno));
    return false;
  }
  return true;
}

static bool obMountImageFile(const char* device, const char* mountPoint)
{
  char loopDevice[OB_DEV_PATH_MAX];
  int loopDeviceFd = obMountLoopDevice(device, loopDevice);

  if (loopDeviceFd < 0) {
    obLogE("Loop device mount failed");
    return false;
  }

  if (mount(loopDevice, mountPoint, OB_DEV_IMAGE_FS,
            OB_DEV_MOUNT_FLAGS, OB_DEV_MOUNT_OPTIONS) < 0) {
      obLogE("Mounting %s in %s failed with error: %s",
             loopDevice, mountPoint, strerror(errno));
  }

  obFreeLoopDevice(loopDeviceFd);
  return true;
}



// --------- public API ---------- //

bool obMountDevice(const char* device, const char* mountPoint)
{
  obLogI("Mounting device %s in %s", device, mountPoint);

  if (!obMkpath(mountPoint, OB_DEV_MOUNT_MODE)) {
    return false;
  }

  obLogI("Directory %s created", mountPoint);

  struct stat devStat;
  int result = lstat(device, &devStat);
  if (result != 0) {
    obLogE("Cannot stat %s: %s", device, strerror(errno));
    return false;
  }

  if (S_ISBLK(devStat.st_mode)) {
    obLogI("%s identified as a block device", device);
    return obMountBlockDevice(device, mountPoint);
  }
  else if (S_ISREG(devStat.st_mode)){
    obLogI("%s identifed as a regular file", device);
    return obMountImageFile(device, mountPoint);
  }

  obLogE("Device type not supported");
  return false;
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

bool obUnmountDevice(const char* mountPoint)
{
  return obUnmount(mountPoint);
}

bool obRbind(const char* srcPath, const char* dstPath)
{
  obLogI("Binding %s to %s", srcPath, dstPath);
  if (!obExists(dstPath) && !obMkpath(dstPath, OB_DEV_MOUNT_MODE)) {
    return false;
  }

  int result = mount(srcPath, dstPath, "", MS_BIND | MS_REC, "");
  if (result != 0) {
    obLogE("Cannot bind %s: %s", srcPath, strerror(errno));
    return false;
  }

  return true;
}

bool obMove(const char* srcPath, const char* dstPath)
{
  obLogI("Moving %s to %s", srcPath, dstPath);
  if (!obMkpath(dstPath, OB_DEV_MOUNT_MODE)) {
    return false;
  }

  int result = mount(srcPath, dstPath, "", MS_MOVE, "");
  if (result != 0) {
    obLogE("Cannot move %s to %s: %s", srcPath, dstPath, strerror(errno));
    return false;
  }

  return true;
}

bool obMountTmpfs(const char* path, const char* sizeStr)
{
  obLogI("Mounting %s as tmpfs of size %s", path, sizeStr);
  if (!obMkpath(path, OB_DEV_MOUNT_MODE)) {
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
  obLogI("Mounting overlayfs in %s", mountPoint);

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

  if (!obMkpath(mountPoint, OB_DEV_MOUNT_MODE)) {
    return false;
  }

  if (!obMkpath(work, OB_DEV_MOUNT_MODE)) {
    return false;
  }

  obLogI("Overlay options: %s", options);
  int result = mount("overlay", mountPoint, "overlay", 0, options);
  if (result != 0) {
    obLogE("Cannot mount %s: %s", mountPoint, strerror(errno));
  }
  else {
    obLogI("OVERLAY MOUNTED");
  }

  return result == 0;
}


//TODO: move
bool obPrepareOverlay(const char* overlayDir, const char* tmpfsSize)
{
  if (!obMountTmpfs(overlayDir, tmpfsSize)) {
    return false;
  }

  //TODO: use sds?
  //TODO: define dirnames in one place
  char path[OB_DEV_PATH_MAX];
  sprintf(path, "%s/work", overlayDir);
  if (!obMkpath(path, OB_MKPATH_MODE)) {
    return false;
  }

  sprintf(path, "%s/lower-root", overlayDir);
  if (!obMkpath(path, OB_MKPATH_MODE)) {
    return false;
  }

  sprintf(path, "%s/upper", overlayDir);
  if (!obMkpath(path, OB_MKPATH_MODE)) {
    return false;
  }

  return true;
}
