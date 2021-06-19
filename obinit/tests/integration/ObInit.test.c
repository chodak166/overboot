#include "unity.h"

#include <stdbool.h>

#define OB_PREFIX_MAX 256
#define OB_PATH_MAX 512
#define OB_NAME_MAX 255
#define OB_DEV_PATH_MAX 512
#define OB_DEV_MOUNT_POINT "/ob_device"
#define OB_DEV_MOUNT_MODE 0700
#define OB_DEV_IMAGE_FS "ext4"
#define OB_DEV_MOUNT_FLAGS 0
#define OB_DEV_MOUNT_OPTIONS ""

typedef struct ObInitConfig
{
  bool enabled;
  char dataDevicePath[OB_DEV_PATH_MAX];
  bool useTmpfs;
  char tmpfsSize[16];
  bool cleanUpperLayer;
  bool enableManagement;
  char headLayer[OB_NAME_MAX];
} ObInitConfig;

void test_xxx()
{

}
