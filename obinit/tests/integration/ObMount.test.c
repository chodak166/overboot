#include "unity.h"
#include "ob/ObMount.h"
#include "ob/ObContext.h"
#include "ObTestHelpers.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_ROOT_PATH            "/test_root"
#define TEST_DEVICE_IMAGE_PATH    "/dev/test_device.img"
#define TEST_MOUNTED_FILE_PATH    "/file.test"
#define TEST_MOUNTED_FILE_VALUE   "test_value_1"
#define TEST_WRONG_DEVICE_PATH    "/dev/none"
#define TEST_BIND_PATH            "/bind"
#define TEST_TMPFS_PATH           "/tmpfs"
#define TEST_TMPFS_SIZE           "50%"
#define TEST_TMP_FILE_VALUE       "tmp_test_value"

#define TEST_LAYER_BOTTOM         "/mount_layers/bottom"
#define TEST_LAYER_MID            "/mount_layers/mid"
#define TEST_LAYER_UPPER          "/mount_layers/upper"
#define TEST_LAYER_WORK           "/mount_layers/work"
#define TEST_LAYER_MOUNT_POINT    "/overlay"
#define TEST_LAYER_MOUNT_POINT_2  "/overlay_moved"
#define TEST_LAYER_FILE_1         "/overlay/file1"
#define TEST_LAYER_FILE_2         "/overlay/file2"
#define TEST_LAYER_FILE_3         "/overlay/file3"
#define TEST_LAYER_FINAL_CONTENT  "mid_value_2"

ObContext helper_getObContext()
{
  char prefix[OB_PATH_MAX];
  obGetSelfPath(prefix, OB_PREFIX_MAX);
  strcat(prefix, TEST_ROOT_PATH);

  ObContext context;
  obInitializeObContext(&context, prefix);

  strcpy(context.config.devicePath, TEST_DEVICE_IMAGE_PATH);
  obFindDevice(&context);

  return context;
}

char* helper_readTestFile(const char* prefix, char* content)
{
  char testFilePath[OB_PATH_MAX];
  sprintf(testFilePath, "%s%s/%s", prefix, OB_DEV_MOUNT_POINT, TEST_MOUNTED_FILE_PATH);
  return obReadFile(testFilePath, content);
}

void test_obMountDevice_shouldReturnTrueOnSuccessfulMount()
{
  ObContext context = helper_getObContext();
  bool mountResult = obMountDevice(context.config.devicePath, context.devMountPoint);
  obUnmountDevice(context.devMountPoint);
  TEST_ASSERT_TRUE(mountResult);
}

void test_obMountDevice_shouldMakeFilesReadableAfterImageMount()
{
  ObContext context = helper_getObContext();
  obMountDevice(context.config.devicePath, context.devMountPoint);

  char content[16];
  helper_readTestFile(context.config.prefix, content);
  obUnmountDevice(context.devMountPoint);

  TEST_ASSERT_EQUAL_STRING(TEST_MOUNTED_FILE_VALUE, content);
}

void test_obMountDevice_shouldMakeFilesReadableAfterDeviceMount()
{
  ObContext context = helper_getObContext();
  char loopDevice[OB_PATH_MAX];
  int loopDev = obMountLoopDevice(context.config.devicePath, loopDevice);

  strcpy(context.config.devicePath, loopDevice);
  obMountDevice(context.config.devicePath, context.devMountPoint);

  obFreeLoopDevice(loopDev);

  char content[16];
  helper_readTestFile(context.config.prefix, content);
  obUnmountDevice(context.devMountPoint);

  TEST_ASSERT_EQUAL_STRING(TEST_MOUNTED_FILE_VALUE, content);
}

void test_obMountDevice_shouldFailWhenWrongPath()
{
  ObContext context = helper_getObContext();
  strcpy(context.config.devicePath, TEST_WRONG_DEVICE_PATH);
  bool result = obMountDevice(context.config.devicePath, context.devMountPoint);
  TEST_ASSERT_FALSE(result);
}

void test_obUnmount_shouldFailWhenPathNotMounted()
{
  char path[OB_PATH_MAX];
  obGetSelfPath(path, OB_PATH_MAX);
  bool result = obUnmount(path);
  TEST_ASSERT_FALSE(result);
}

void test_obUnmountDevice_shouldUnmountMountedDevice()
{
  ObContext context = helper_getObContext();
  char testFile[OB_PATH_MAX];
  obConcatPaths(testFile, context.devMountPoint, TEST_MOUNTED_FILE_PATH);

  bool mntResult = obMountDevice(context.config.devicePath, context.devMountPoint);
  bool umntResult = obUnmountDevice(context.devMountPoint);
  int accessResult = access(testFile, F_OK);
  TEST_ASSERT_TRUE(mntResult);
  TEST_ASSERT_TRUE(umntResult);
  TEST_ASSERT_TRUE(accessResult != 0);
}

void test_obRbind_shouldMakeFilesAccessibleAfterBind()
{
  char srcPath[OB_PATH_MAX];
  char dstPath[OB_PATH_MAX];
  obGetSelfPath(srcPath, OB_PATH_MAX);
  obGetSelfPath(dstPath, OB_PATH_MAX);

  strcat(srcPath, TEST_ROOT_PATH);
  strcat(dstPath, TEST_BIND_PATH);

  obRbind(srcPath, dstPath);

  char imgFile[OB_PATH_MAX];
  obConcatPaths(imgFile, dstPath, TEST_DEVICE_IMAGE_PATH);
  int accessResult = access(imgFile, F_OK);

  obUnmount(dstPath);

  TEST_ASSERT_TRUE(accessResult == 0);
}

void test_obMountTmpfs_shouldCreateDirAndMakeItWritable()
{
  char tmpfsPath[OB_PATH_MAX];
  obGetSelfPath(tmpfsPath, OB_PATH_MAX);
  strcat(tmpfsPath, TEST_TMPFS_PATH);

  obMountTmpfs(tmpfsPath, TEST_TMPFS_SIZE);

  char testFile[OB_PATH_MAX];
  obConcatPaths(testFile, tmpfsPath, "/tmp_file");
  obCreateFile(testFile, TEST_TMP_FILE_VALUE);

  int accessResult = access(testFile, F_OK);
  obUnmount(tmpfsPath);

  TEST_ASSERT_TRUE(accessResult == 0);
}

void test_obMountOverlay_shouldMergeAllGivenLayers()
{
  char selfPath[OB_PATH_MAX];
  obGetSelfPath(selfPath, OB_PATH_MAX);

  char mountPoint[OB_PATH_MAX];
  strcpy(mountPoint, selfPath);
  strcat(mountPoint, TEST_LAYER_MOUNT_POINT);

  char upper[OB_PATH_MAX];
  obConcatPaths(upper, selfPath, TEST_LAYER_UPPER);

  char work[OB_PATH_MAX];
  obConcatPaths(work, selfPath, TEST_LAYER_WORK);

  char** layers = malloc(sizeof(char*) * 2);
  layers[0] = malloc(OB_PATH_MAX);
  layers[1] = malloc(OB_PATH_MAX);

  sprintf(layers[0], "%s%s", selfPath, TEST_LAYER_BOTTOM);
  sprintf(layers[1], "%s%s", selfPath, TEST_LAYER_MID);

  obMountOverlay(layers, 2,
      upper, work, mountPoint);

  char filePath[OB_PATH_MAX];

  obConcatPaths(filePath, selfPath, TEST_LAYER_FILE_1);
  int accessFile1Result = access(filePath, F_OK);

  obConcatPaths(filePath, selfPath, TEST_LAYER_FILE_2);
  int accessFile2Result = access(filePath, F_OK);

  char content[16];
  obReadFile(filePath, content);

  obConcatPaths(filePath, selfPath, TEST_LAYER_FILE_3);
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
