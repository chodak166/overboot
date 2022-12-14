// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the MIT License.
// See accompanying file LICENSE.txt for the full license.

#include "ObMount.h"
#include "ObOsUtils.h"
#include "ob/ObLogging.h"

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

// --------- public API ---------- //

bool obMountBlockDevice(const char* device, const char* mountPoint)
{
  if (!obMkpath(mountPoint, OB_DEV_MOUNT_MODE)) {
    return false;
  }

  int result = mount(device, mountPoint,
                 OB_DEVICE_FS, OB_DEV_MOUNT_FLAGS, OB_DEV_MOUNT_OPTIONS);
  if (result != 0) {
    obLogE("Cannot mount %s in %s: %s",
           device, mountPoint, strerror(errno));
    return false;
  }
  return true;
}

bool obMountImageFile(const char* device, const char* mountPoint)
{
  char loopDevice[OB_DEV_PATH_MAX];
  int loopDeviceFd = obMountLoopDevice(device, loopDevice);

  if (loopDeviceFd < 0) {
    obLogE("Loop device mount failed");
    return false;
  }

  if (!obMkpath(mountPoint, OB_DEV_MOUNT_MODE)) {
    return false;
  }

  obLogI("Mounting image file (%s) via loop device: %s", device, loopDevice);
  bool result = true;
  if (mount(loopDevice, mountPoint, OB_DEVICE_FS,
            OB_DEV_MOUNT_FLAGS, OB_DEV_MOUNT_OPTIONS) < 0) {
      obLogE("Mounting %s in %s failed with error: %s",
             loopDevice, mountPoint, strerror(errno));
      result = false;
  }

  obFreeLoopDevice(loopDeviceFd);
  return result;
}

//bool obMountDevice(const char* device, const char* mountPoint)
//{
//  obLogI("Mounting device %s in %s", device, mountPoint);

//  if (!obMkpath(mountPoint, OB_DEV_MOUNT_MODE)) {
//    return false;
//  }

//  struct stat devStat;
//  int result = lstat(device, &devStat);
//  if (result != 0) {
//    obLogE("Cannot stat %s: %s", device, strerror(errno));
//    return false;
//  }

//  if (S_ISBLK(devStat.st_mode)) {
//    obLogI("%s identified as a block device", device);
//    return obMountBlockDevice(device, mountPoint);
//  }
//  else if (S_ISREG(devStat.st_mode)){
//    obLogI("%s identifed as a regular file", device);
//    return obMountImageFile(device, mountPoint);
//  }

//  obLogE("Device type not supported");
//  return false;
//}

bool obMountEmbeddedRepository(const char* repoPath, const char* mountPoint)
{
  if (!obExists(repoPath)) {
    obLogI("Embedded directory %s does not exist, creating...", repoPath);
    if (!obMkpath(repoPath, OB_MKPATH_MODE)) {
      return false;
    }
  }

  if (!obRbind(repoPath, mountPoint)) {
    return false;
  }

  return true;
}

bool obUnmount(const char* path)
{
  obLogI("Unmounting %s", path);
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
      return controlFd;
  }

  int loopId = ioctl(controlFd, LOOP_CTL_GET_FREE);
  sprintf(loopDevice, "/dev/loop%d", loopId);
  close(controlFd);

  obLogI("Using loop device: %s", loopDevice);

  int imageFd = open64(imagePath, O_RDWR);
  if (imageFd < 0) {
      obLogE("Opening image file (%s) failed: %s", imagePath, strerror(errno));
      return imageFd;
  }

  int deviceFd = open(loopDevice, O_RDWR);
  if (deviceFd < 0) {
      obLogE("Opening loop device failed");
      close(imageFd);
      return deviceFd;
  }

  if (ioctl(deviceFd, LOOP_SET_FD, imageFd) < 0) {
      obLogE("ioctl LOOP_SET_FD failed");
      close(imageFd);
      close(deviceFd);
      return -1;
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

bool obBlockByTmpfs(const char* path)
{
  obLogI("Blocking access to %s", path);
  int result = mount("tmpfs", path, "tmpfs", 0, OB_TMPFS_BLOCK_OPTIONS);
  if (result != 0) {
    obLogE("Mounting tmpfs (rw) in %s failed with error: %s",
           path, strerror(errno));
    return false;
  }

  char readmePath[OB_CPATH_MAX];
  sprintf(readmePath, "%s/%s", path, OB_TMPFS_BLOCK_INFO_FILE);
  FILE* file = fopen(readmePath, "w");
  if (file != NULL) {
    obLogI("Creating %s file: %s", OB_TMPFS_BLOCK_INFO_FILE, readmePath);
    fputs(OB_TMPFS_BLOCK_INFO_TEXT, file);
    fclose(file);
  }
  else {
    obLogE("Cannot create %s", readmePath);
  }
  return obRemountRo(path, NULL);
}

bool obRemountRw(const char* path, void* data)
{
  obLogI("Remounting (RW) %s", path);
  if (mount("", path, "", MS_REMOUNT, data) != 0) {
    obLogE("Remounting (RW) %s failed: ", path, strerror(errno));
    return false;
  }
  return true;
}

bool obRemountRo(const char* path, void* data)
{
  obLogI("Remounting (RO) %s", path);
  if (mount("", path, "", MS_REMOUNT | MS_RDONLY, data) != 0) {
    obLogE("Remounting (RO) %s failed: ", path, strerror(errno));
    return false;
  }
  return true;
}
