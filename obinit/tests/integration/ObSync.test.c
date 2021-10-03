
#include "unity.h"
#include "ob/ObOsUtils.h"
#include "ob/ObDefs.h"
#include "ObTestHelpers.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <errno.h>
//#define __USE_XOPEN_EXTENDED

#define UNUSED(x) (void)(x)

#define TEST_SRC_NAME "src"
#define TEST_DST_NAME "dst"
#define TEST_DIR_1 "dir_a"
#define TEST_DIR_2 "dir_b"
#define TEST_SUBDIR_1 "dir_a/subdir_a"
#define TEST_SUBDIR_2 "dir_a/subdir_b"
#define TEST_SUBDIR_3 "dir_a/subdir_a/subdir_c"
#define TEST_FILE_1 "dir_a/subdir_a/file_a"
#define TEST_LINK_1 "dir_b/link_a"
#define TEST_CONTENT "sync test content"

char treePath[OB_PATH_MAX] = {0};
char srcPath[OB_PATH_MAX] = {0};
char dstPath[OB_PATH_MAX] = {0};

void helper_setupSrcTree()
{
  srand(time(0));
  obGetSelfPath(treePath, OB_PATH_MAX);

  char topName[OB_NAME_MAX];
  strcpy(topName, "/obsync-test-");
  for (int i = 0; i < 6; ++i) {
    char c[2] = {(rand()%26) + 97, '\0'};
    strcat(topName, c);
  }
  strcat(treePath, topName);

  sprintf(srcPath, "%s/%s", treePath, TEST_SRC_NAME);
  sprintf(dstPath, "%s/%s", treePath, TEST_DST_NAME);
  obMkpath(srcPath, OB_MKPATH_MODE);

  char path[OB_CCPATH_MAX];
  sprintf(path, "%s/%s", srcPath, TEST_DIR_1);
  obMkpath(path, OB_MKPATH_MODE);
  sprintf(path, "%s/%s", srcPath, TEST_DIR_2);
  obMkpath(path, OB_MKPATH_MODE);
  sprintf(path, "%s/%s", srcPath, TEST_SUBDIR_1);
  obMkpath(path, OB_MKPATH_MODE);
  sprintf(path, "%s/%s", srcPath, TEST_SUBDIR_2);
  obMkpath(path, OB_MKPATH_MODE);
  sprintf(path, "%s/%s", srcPath, TEST_SUBDIR_3);
  obMkpath(path, OB_MKPATH_MODE);

  sprintf(path, "%s/%s", srcPath, TEST_FILE_1);
  obCreateFile(path, TEST_CONTENT);

  char linkPath[OB_CCPATH_MAX];
  sprintf(linkPath, "%s/%s", srcPath, TEST_LINK_1);
  symlink(path, linkPath);
}

void setUp(void)
{
  helper_setupSrcTree();
}

void tearDown(void)
{
  if (strlen(treePath) > 1) {
    obRemoveDirR(treePath);
  }
}


void test_obSync_shouldRecreateDirectoriesRecursively()
{
  obSync(srcPath, dstPath);

  char testPath[OB_CPATH_MAX];
  sprintf(testPath, "%s/%s", dstPath, TEST_DIR_1);
  TEST_ASSERT_TRUE(obExists(testPath));
  sprintf(testPath, "%s/%s", dstPath, TEST_DIR_2);
  TEST_ASSERT_TRUE(obExists(testPath));
  sprintf(testPath, "%s/%s", dstPath, TEST_SUBDIR_1);
  TEST_ASSERT_TRUE(obExists(testPath));
  sprintf(testPath, "%s/%s", dstPath, TEST_SUBDIR_2);
  TEST_ASSERT_TRUE(obExists(testPath));
  sprintf(testPath, "%s/%s", dstPath, TEST_SUBDIR_3);
}

void test_obSync_shouldCopyFiles()
{
  obSync(srcPath, dstPath);

  char testPath[OB_CPATH_MAX];
  sprintf(testPath, "%s/%s", dstPath, TEST_FILE_1);
  TEST_ASSERT_TRUE(obExists(testPath));
}

void test_obSync_shouldCopySymbolicLinks()
{
  obSync(srcPath, dstPath);

  char testPath[OB_CPATH_MAX];
  sprintf(testPath, "%s/%s", dstPath, TEST_LINK_1);
  struct stat buf;
  lstat(testPath, &buf);
  TEST_ASSERT_TRUE(obExists(testPath));
  TEST_ASSERT_TRUE(S_ISLNK(buf.st_mode));
}
