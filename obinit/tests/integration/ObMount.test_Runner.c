/* AUTOGENERATED FILE. DO NOT EDIT. */

/*=======Automagically Detected Files To Include=====*/
#include "unity.h"
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

/*=======External Functions This Runner Calls=====*/
extern void setUp(void);
extern void tearDown(void);
extern void test_obMountDevice_shouldMakeFilesReadableAfterImageMount();
extern void test_obMountDevice_shouldMakeFilesReadableAfterDeviceMount();
extern void test_obMountDevice_shouldFailWhenWrongPath();
extern void test_obMountDevice_shouldFailWhenPathNotMounted();
extern void test_obUnmountDevice_shouldUnmountMountedDevice();
extern void test_obRbind_shouldMakeFilesAccessibleAfterBind();
extern void test_obMountTmpfs_shouldCreateDirAndMakeItWritable();
extern void test_obMountOverlay_shouldMergeAllGivenLayers();


/*=======Mock Management=====*/
static void CMock_Init(void)
{
}
static void CMock_Verify(void)
{
}
static void CMock_Destroy(void)
{
}

/*=======Setup (stub)=====*/
void setUp(void) {}

/*=======Teardown (stub)=====*/
void tearDown(void) {}

/*=======Test Reset Options=====*/
void resetTest(void);
void resetTest(void)
{
  tearDown();
  CMock_Verify();
  CMock_Destroy();
  CMock_Init();
  setUp();
}
void verifyTest(void);
void verifyTest(void)
{
  CMock_Verify();
}

/*=======Test Runner Used To Run Each Test=====*/
static void run_test(UnityTestFunction func, const char* name, UNITY_LINE_TYPE line_num)
{
    Unity.CurrentTestName = name;
    Unity.CurrentTestLineNumber = line_num;
#ifdef UNITY_USE_COMMAND_LINE_ARGS
    if (!UnityTestMatches())
        return;
#endif
    Unity.NumberOfTests++;
    UNITY_CLR_DETAILS();
    UNITY_EXEC_TIME_START();
    CMock_Init();
    if (TEST_PROTECT())
    {
        setUp();
        func();
    }
    if (TEST_PROTECT())
    {
        tearDown();
        CMock_Verify();
    }
    CMock_Destroy();
    UNITY_EXEC_TIME_STOP();
    UnityConcludeTest();
}

/*=======MAIN=====*/
int main(void)
{
  UnityBegin("../../../tests/integration/ObMount.test.c");
  run_test(test_obMountDevice_shouldMakeFilesReadableAfterImageMount, "test_obMountDevice_shouldMakeFilesReadableAfterImageMount", 360);
  run_test(test_obMountDevice_shouldMakeFilesReadableAfterDeviceMount, "test_obMountDevice_shouldMakeFilesReadableAfterDeviceMount", 372);
  run_test(test_obMountDevice_shouldFailWhenWrongPath, "test_obMountDevice_shouldFailWhenWrongPath", 390);
  run_test(test_obMountDevice_shouldFailWhenPathNotMounted, "test_obMountDevice_shouldFailWhenPathNotMounted", 398);
  run_test(test_obUnmountDevice_shouldUnmountMountedDevice, "test_obUnmountDevice_shouldUnmountMountedDevice", 406);
  run_test(test_obRbind_shouldMakeFilesAccessibleAfterBind, "test_obRbind_shouldMakeFilesAccessibleAfterBind", 420);
  run_test(test_obMountTmpfs_shouldCreateDirAndMakeItWritable, "test_obMountTmpfs_shouldCreateDirAndMakeItWritable", 441);
  run_test(test_obMountOverlay_shouldMergeAllGivenLayers, "test_obMountOverlay_shouldMergeAllGivenLayers", 459);

  return UnityEnd();
}
