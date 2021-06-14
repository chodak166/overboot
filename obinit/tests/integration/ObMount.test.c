#include "unity.h"
#define __STDC_WANT_LIB_EXT1__ 1

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <linux/loop.h>
#include <fcntl.h>

#include <errno.h>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>


#define TEST_ROOT_PATH            "/test_root"
#define TEST_NOT_MOUNTED_PATH     "/no_mnt"
#define TEST_DEVICE_IMAGE_PATH    "/dev/test_device.img"
#define TEST_MOUNTED_FILE_PATH    "/file.test"
#define TEST_MOUNTED_FILE_VALUE   "test_value_1"
#define TEST_WRONG_DEVICE_PATH    "/dev/none"
#define TEST_BIND_PATH            "/bind"
#define TEST_TMPFS_PATH           "/tmpfs"
#define TEST_TMP_FILE_VALUE       "tmp_test_value"

#define TEST_LAYER_BOTTOM         "/layers/bottom"
#define TEST_LAYER_MID            "/layers/mid"
#define TEST_LAYER_UPPER          "/layers/upper"
#define TEST_LAYER_WORK           "/layers/work"
#define TEST_LAYER_MOUNT_POINT    "/overlay"
#define TEST_LAYER_FILE_1         "/overlay/file1"
#define TEST_LAYER_FILE_2         "/overlay/file2"
#define TEST_LAYER_FILE_3         "/overlay/file3"
#define TEST_LAYER_FINAL_CONTENT  "mid_value_2"

static int do_mkdir(const char *path, mode_t mode)
{
    struct stat            st;
    int             status = 0;

    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = -1;
    }

    return(status);
}

int mkpath(const char *path, mode_t mode)
{
    char           *pp;
    char           *sp;
    int             status;
    char           *copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = do_mkdir(path, mode);
    free(copypath);
    return (status);
}



#define OB_PREFIX_MAX 256
#define OB_PATH_MAX 512
#define OB_DEV_PATH_MAX 512
#define OB_DEV_MOUNT_POINT "/ob_device"
#define OB_DEV_MOUNT_MODE 0700
#define OB_DEV_IMAGE_FS "ext4"
#define OB_DEV_MOUNT_FLAGS 0
#define OB_DEV_MOUNT_OPTIONS ""


bool helper_concatPaths(char* result, const char* pathA, const char* pathB)
{
  if (strlen(pathA) + strlen(pathB) > OB_PATH_MAX) {
    fprintf(stderr, "Cannot concat %s and %s: result path too long\n", pathA, pathB);
    return false;
  }
  strcpy(result, pathA);
  strcat(result, pathB);
  return true;
}

typedef struct ObContext
{
  char prefix[OB_PREFIX_MAX];
  char devicePath[OB_PATH_MAX];
  char devMountPoint[OB_PATH_MAX];
} ObContext;

void obSetPrefix(ObContext* context, const char* prefix)
{
  strcpy(context->prefix, prefix);
  sprintf(context->devMountPoint, "%s%s", prefix, OB_DEV_MOUNT_POINT);
}

void obSetDevicePath(ObContext* context, const char* path)
{
  sprintf(context->devicePath, "%s%s", context->prefix, path);
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

bool obMountDevice(const ObContext* context)
{
  int result = mkpath(context->devMountPoint, OB_DEV_MOUNT_MODE);
  if (result != 0) {
    fprintf(stderr, "Cannot create %s: %s\n", context->devMountPoint, strerror(errno));
    return false;
  }
  printf("dir created\n");


  struct stat devStat;
  result = lstat(context->devicePath, &devStat);
  if (result != 0) {
    fprintf(stderr, "Cannot stat %s: %s\n", context->devicePath, strerror(errno));
    return false;
  }

  if (S_ISBLK(devStat.st_mode)) {
    printf("%s is a block device\n", context->devicePath);


    result = mount(context->devicePath, context->devMountPoint,
                   OB_DEV_IMAGE_FS, OB_DEV_MOUNT_FLAGS, OB_DEV_MOUNT_OPTIONS);
    printf("mount result: %d\n", result);
    if (result != 0) {
      fprintf(stderr, "Cannot mount %s: %s\n", context->devicePath, strerror(errno));
      return false;
    }

    printf("mounted as a block device\n");


  }
  else if (S_ISREG(devStat.st_mode)){
    printf("%s is a regular file\n", context->devicePath);

    char loopDevice[OB_DEV_PATH_MAX];
    int loopDeviceFd = obMountLoopDevice(context->devicePath, loopDevice);

    if (loopDeviceFd < 0) {
      perror("loop device mount failed");
      return false;
    }

    if (mount(loopDevice, context->devMountPoint, OB_DEV_IMAGE_FS,
              OB_DEV_MOUNT_FLAGS, OB_DEV_MOUNT_OPTIONS) < 0) {
        perror("mount failed");
    } else {
        printf("mount successful\n");
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

bool obMountTmpfs(const char* path)
{
  if (mkpath(path, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  int result = mount("tmpfs", path, "tmpfs", 0, "");
  if (result != 0) {
    fprintf(stderr, "Cannot mount tmpfs in %s: %s\n", path, strerror(errno));
    return false;
  }
  return true;
}


bool obRbind(const char* srcPath, const char* dstPath)
{
  if (mkpath(dstPath, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  int result = mount(srcPath, dstPath, NULL, MS_BIND | MS_REC, "");
  if (result != 0) {
    fprintf(stderr, "Cannot mount %s: %s\n", srcPath, strerror(errno));
    return false;
  }

  return true;
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

  if (mkpath(mountPoint, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  if (mkpath(work, OB_DEV_MOUNT_MODE) != 0) {
    return false;
  }

  int result = mount("overlay", mountPoint, "overlay", 0, options);
  if (result != 0) {
    fprintf(stderr, "Cannot mount %s: %s\n", mountPoint, strerror(errno));
  }
  else {
    printf("OVERLAY MOUNTED\n");
  }

  //free(layerCount);
  return result == 0;
}

char* helper_getSelfPath(char* buffer, int size)
{
  int result = readlink("/proc/self/exe", buffer, size - 1);
  if (result < 0 || (result >= size - 1)) {
    return NULL;
  }

  buffer[result] = '\0';
  for (int i = result; i >= 0; i--) {
    if (buffer[i] == '/') {
      buffer[i] = '\0';
      break;
    }
  }
  return buffer;
}

ObContext helper_getObContext()
{
  char prefix[OB_PREFIX_MAX];
  helper_getSelfPath(prefix, OB_PREFIX_MAX);
  strcat(prefix, TEST_ROOT_PATH);

  ObContext context;
  obSetPrefix(&context, prefix);
  obSetDevicePath(&context, TEST_DEVICE_IMAGE_PATH);

  return context;
}

char* helper_readFile(const char* path, char* content)
{
  FILE* file = fopen(path, "r");
  if (file != NULL) {
    fscanf(file, "%s", content);
    fclose(file);
  }
  else {
    content[0] = '\0';
  }
  return content;
}

char* helper_readTestFile(const ObContext* context, char* content)
{
  char testFilePath[OB_PATH_MAX];
  sprintf(testFilePath, "%s%s/%s", context->prefix, OB_DEV_MOUNT_POINT, TEST_MOUNTED_FILE_PATH);
  return helper_readFile(testFilePath, content);
}

void helper_createFile(const char* path, const char* content)
{
  FILE* file = fopen(path, "w");
  if (file != NULL) {
    fputs(content, file);
    fclose(file);
  }
}


void test_obMountDevice_shouldMakeFilesReadableAfterImageMount()
{
  ObContext context = helper_getObContext();
  obMountDevice(&context);

  char content[16];
  helper_readTestFile(&context, content);
  obUnmountDevice(&context);

  TEST_ASSERT_EQUAL_STRING(TEST_MOUNTED_FILE_VALUE, content);
}

void test_obMountDevice_shouldMakeFilesReadableAfterDeviceMount()
{
  ObContext context = helper_getObContext();
  char loopDevice[OB_PATH_MAX];
  int loopDev = obMountLoopDevice(context.devicePath, loopDevice);

  strcpy(context.devicePath, loopDevice);
  obMountDevice(&context);

  obFreeLoopDevice(loopDev);

  char content[16];
  helper_readTestFile(&context, content);
  obUnmountDevice(&context);

  TEST_ASSERT_EQUAL_STRING(TEST_MOUNTED_FILE_VALUE, content);
}

void test_obMountDevice_shouldFailWhenWrongPath()
{
  ObContext context = helper_getObContext();
  obSetDevicePath(&context, TEST_WRONG_DEVICE_PATH);
  bool result = obMountDevice(&context);
  TEST_ASSERT_FALSE(result);
}

void test_obUnmount_shouldFailWhenPathNotMounted()
{
  char path[OB_PATH_MAX];
  helper_getSelfPath(path, OB_PATH_MAX);
  strcat(path, TEST_NOT_MOUNTED_PATH);
  mkpath(path, 0700);
  bool result = obUnmount(path);
  TEST_ASSERT_FALSE(result);
}

void test_obUnmountDevice_shouldUnmountMountedDevice()
{
  ObContext context = helper_getObContext();
  char testFile[OB_PATH_MAX];
  helper_concatPaths(testFile, context.devMountPoint, TEST_MOUNTED_FILE_PATH);

  bool mntResult = obMountDevice(&context);
  bool umntResult = obUnmountDevice(&context);
  int accessResult = access(testFile, F_OK);
  TEST_ASSERT_TRUE(mntResult);
  TEST_ASSERT_TRUE(umntResult);
  TEST_ASSERT_TRUE(accessResult != 0);
}

void test_obRbind_shouldMakeFilesAccessibleAfterBind()
{
  char srcPath[OB_PATH_MAX];
  char dstPath[OB_PATH_MAX];
  helper_getSelfPath(srcPath, OB_PATH_MAX);
  helper_getSelfPath(dstPath, OB_PATH_MAX);

  strcat(srcPath, TEST_ROOT_PATH);
  strcat(dstPath, TEST_BIND_PATH);

  obRbind(srcPath, dstPath);

  char imgFile[OB_PATH_MAX];
  helper_concatPaths(imgFile, dstPath, TEST_DEVICE_IMAGE_PATH);
  int accessResult = access(imgFile, F_OK);

  obUnmount(dstPath);

  TEST_ASSERT_TRUE(accessResult == 0);
}

void test_obMountTmpfs_shouldCreateDirAndMakeItWritable()
{
  char tmpfsPath[OB_PATH_MAX];
  helper_getSelfPath(tmpfsPath, OB_PATH_MAX);
  strcat(tmpfsPath, TEST_TMPFS_PATH);

  obMountTmpfs(tmpfsPath);

  char testFile[OB_PATH_MAX];
  helper_concatPaths(testFile, tmpfsPath, "/tmp_file");
  helper_createFile(testFile, TEST_TMP_FILE_VALUE);

  int accessResult = access(testFile, F_OK);
  obUnmount(tmpfsPath);

  TEST_ASSERT_TRUE(accessResult == 0);
}

void test_obMountOverlay_shouldMergeAllGivenLayers()
{
  char selfPath[OB_PATH_MAX];
  helper_getSelfPath(selfPath, OB_PATH_MAX);

  char mountPoint[OB_PATH_MAX];
  strcpy(mountPoint, selfPath);
  strcat(mountPoint, TEST_LAYER_MOUNT_POINT);

  char upper[OB_PATH_MAX];
  helper_concatPaths(upper, selfPath, TEST_LAYER_UPPER);

  char work[OB_PATH_MAX];
  helper_concatPaths(work, selfPath, TEST_LAYER_WORK);

  //char layers[2][OB_PATH_MAX];
  char** layers = malloc(sizeof(char*) * 2);
  layers[0] = malloc(OB_PATH_MAX);
  layers[1] = malloc(OB_PATH_MAX);

  sprintf(layers[0], "%s%s", selfPath, TEST_LAYER_BOTTOM);
  sprintf(layers[1], "%s%s", selfPath, TEST_LAYER_MID);

  obMountOverlay(layers, 2,
      upper, work, mountPoint);

  char filePath[OB_PATH_MAX];

  helper_concatPaths(filePath, selfPath, TEST_LAYER_FILE_1);
  int accessFile1Result = access(filePath, F_OK);

  helper_concatPaths(filePath, selfPath, TEST_LAYER_FILE_2);
  int accessFile2Result = access(filePath, F_OK);

  char content[16];
  helper_readFile(filePath, content);

  helper_concatPaths(filePath, selfPath, TEST_LAYER_FILE_3);
  int accessFile3Result = access(filePath, F_OK);

  obUnmount(mountPoint);
  free(layers[0]);
  free(layers[1]);
  free(layers);

  TEST_ASSERT_TRUE(accessFile1Result == 0);
  TEST_ASSERT_TRUE(accessFile2Result == 0);
  TEST_ASSERT_TRUE(accessFile3Result == 0);
  TEST_ASSERT_EQUAL_STRING(TEST_LAYER_FINAL_CONTENT, content);
}
