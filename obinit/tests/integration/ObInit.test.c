#include "unity.h"
#include "ob/ObDefs.h"

#include <stdbool.h>

typedef struct ObDurable
{
  char name[OB_NAME_MAX];
  char path[OB_PATH_MAX];
} ObDurable;

typedef struct ObInitConfig
{
  bool enabled;
  char dataDevicePath[OB_DEV_PATH_MAX];
  bool useTmpfs;
  char tmpfsSize[16];
  bool cleanUpperLayer;
  bool bindRepository;
  char headLayer[OB_NAME_MAX];
} ObInitConfig;

void test_obInitializeOverboot_shouldMountDataDevice()
{

}

void test_obInitializeOverboot_shouldPrepareOverlayDirectory()
{

}

void test_obInitializeOverboot_shouldUseTmpfsIfConfigured()
{

}

void test_obInitializeOverboot_shouldSetTmpfsSize()
{

}

void test_obInitializeOverboot_shouldClearUpperIfConfigured()
{

}

void test_obInitializeOverboot_shouldMoveRootfs()
{

}

void test_obInitializeOverboot_shouldBindUpperLayerIfConfigured()
{

}

void test_obInitializeOverboot_shouldMountOverlayFs()
{

}

void test_obInitializeOverboot_shouldMountAllLayersRequiredByHeadLayer()
{

}

void test_obInitializeOverboot_shouldBindOverlayDirectory()
{

}

void test_obInitializeOverboot_shouldBindRepositoryIfConfigured()
{

}

void test_obInitializeOverboot_shouldBindDurables()
{

}

void test_obInitializeOverboot_shouldInitializeDurablesIfConfigured()
{

}

void test_obInitializeOverboot_shouldNotInitializeDurablesIfConfigured()
{

}

void test_obInitializeOverboot_shouldUpdateFstab()
{

}

void test_obDeinitializeOverboot_shouldRestoreRootfs()
{

}
