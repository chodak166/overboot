#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

TEST_TMP_DIR="$SCRIPT_DIR/tmp_$(date +%s)"
TEST_RAMFS_DIR="$TEST_TMP_DIR/ramfs"
TEST_ROOTFS_DIR="$TEST_RAMFS_DIR/rootfs"
TEST_OB_DEVICE_PATH="$TEST_RAMFS_DIR/dev/ob_test.img"

TEST_OB_CONFIG_PATH="$TEST_ROOTFS_DIR/etc/overboot.yaml"
TEST_DURABLES_DIR_1="$TEST_ROOTFS_DIR/test/durables-1"
TEST_DURABLES_DIR_1="$TEST_ROOTFS_DIR/test/durables-2"
TEST_NON_EMPTY_DURABLE="$TEST_ROOTFS_DIR/test/non_empty_durable"
TEST_DURABLE_FILE="$TEST_NON_EMPTY_DURABLE/file.txt"
TEST_DURABLE_VALUE="durable value"

TEST_RES_DIR="$SCRIPT_DIR/res"

test_setupFakeRamfsRoot()
{
  mkdir -p "$TEST_ROOTFS_DIR/dev"
  mkdir -p "$TEST_ROOTFS_DIR/etc"

  test_createRootfsImage
  test_mountRootfsImage
  test_createOverbootDeviceImage

  mkdir -p "$TEST_DURABLES_DIR_1"
  mkdir -p "$TEST_DURABLES_DIR_1"
  echo "$TEST_DURABLE_VALUE" > "$TEST_DURABLES_DIR_1/orig.txt"
  echo "$TEST_DURABLE_VALUE" > "$TEST_DURABLES_DIR_2/orig.txt"
  cp "$TEST_RES_DIR/configs/overboot-tmpfs.yaml" "$TEST_ROOTFS_DIR/etc/overboot.yaml"
}


test_createRootfsImage()
{
}

test_mountRootfsImage()
{
}

test_createOverbootDeviceImage()
{
}
