/* AUTOGENERATED FILE. DO NOT EDIT. */

/*=======Automagically Detected Files To Include=====*/
#include "unity.h"
#include "ob/ObMount.h"
#include "ob/ObContext.h"
#include "ObTestHelpers.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*=======External Functions This Runner Calls=====*/
extern void setUp(void);
extern void tearDown(void);
extern void test_obMountDevice_shouldReturnTrueOnSuccessfulMount();
extern void test_obMountDevice_shouldMakeFilesReadableAfterImageMount();
extern void test_obMountDevice_shouldMakeFilesReadableAfterDeviceMount();
extern void test_obMountDevice_shouldFailWhenWrongPath();
extern void test_obUnmount_shouldFailWhenPathNotMounted();
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
  run_test(test_obMountDevice_shouldReturnTrueOnSuccessfulMount, "test_obMountDevice_shouldReturnTrueOnSuccessfulMount", 53);
  run_test(test_obMountDevice_shouldMakeFilesReadableAfterImageMount, "test_obMountDevice_shouldMakeFilesReadableAfterImageMount", 61);
  run_test(test_obMountDevice_shouldMakeFilesReadableAfterDeviceMount, "test_obMountDevice_shouldMakeFilesReadableAfterDeviceMount", 73);
  run_test(test_obMountDevice_shouldFailWhenWrongPath, "test_obMountDevice_shouldFailWhenWrongPath", 91);
  run_test(test_obUnmount_shouldFailWhenPathNotMounted, "test_obUnmount_shouldFailWhenPathNotMounted", 99);
  run_test(test_obUnmountDevice_shouldUnmountMountedDevice, "test_obUnmountDevice_shouldUnmountMountedDevice", 107);
  run_test(test_obRbind_shouldMakeFilesAccessibleAfterBind, "test_obRbind_shouldMakeFilesAccessibleAfterBind", 121);
  run_test(test_obMountTmpfs_shouldCreateDirAndMakeItWritable, "test_obMountTmpfs_shouldCreateDirAndMakeItWritable", 142);
  run_test(test_obMountOverlay_shouldMergeAllGivenLayers, "test_obMountOverlay_shouldMergeAllGivenLayers", 160);

  return UnityEnd();
}
