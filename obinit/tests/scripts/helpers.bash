#!/bin/bash

#set -x
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/../.."
TEST_RES_DIR="$PROJECT_ROOT/tests/functional/res"

if [[ -z "$TEST_TMP_DIR" ]]; then
  TEST_TMP_DIR="$SCRIPT_DIR/tmp_$(date +%s)"
fi

TEST_RAMFS_DIR="$TEST_TMP_DIR/ramfs"
TEST_ROOTMNT_DIR="$TEST_RAMFS_DIR/root"
TEST_OB_DEVICE_PATH="$TEST_RAMFS_DIR/dev/ob_test.img"
TEST_OB_DEVICE_MNT_PATH="$TEST_RAMFS_DIR/obmnt"
TEST_MNT_DIR="$TEST_TMP_DIR/mnt"
TEST_OB_REPOSITORY_DIR="/overboot"

TEST_OB_CONFIG_PATH="$TEST_ROOTMNT_DIR/etc/overboot.yaml"
TEST_DURABLES_DIR_1="$TEST_ROOTMNT_DIR/test/durables-1"
TEST_DURABLES_DIR_1="$TEST_ROOTMNT_DIR/test/durables-2"
TEST_NON_EMPTY_DURABLE="$TEST_ROOTMNT_DIR/test/non_empty_durable"
TEST_DURABLE_FILE="$TEST_NON_EMPTY_DURABLE/file.txt"
TEST_DURABLE_VALUE="durable value"

TEST_OB_OVERLAY_DIR="$TEST_RAMFS_DIR/overlay"

TEST_MAX_NESTED_MOUNTS=4

test_setupFakeRamfsRoot()
{
  test_createOverbootDeviceImage
  test_mountRootmnt

  mkdir -p "$TEST_ROOTMNT_DIR/dev"
  mkdir -p "$TEST_ROOTMNT_DIR/etc"
  mkdir -p "$TEST_RAMFS_DIR/etc"
  mkdir -p "$TEST_DURABLES_DIR_1"
  mkdir -p "$TEST_DURABLES_DIR_1"

  echo "$TEST_DURABLE_VALUE" > "$TEST_DURABLES_DIR_1/orig.txt"
  echo "$TEST_DURABLE_VALUE" > "$TEST_DURABLES_DIR_2/orig.txt"
  cp "$TEST_RES_DIR/configs/overboot-tmpfs.yaml" "$TEST_ROOTMNT_DIR/etc/overboot.yaml"
  cp -v "$TEST_RES_DIR/fstab" "$TEST_ROOTMNT_DIR/etc/fstab"
  cp "$TEST_RES_DIR/fstab-parsed" "$TEST_TMP_DIR/"
  cp "$TEST_RES_DIR/mtab" "$TEST_RAMFS_DIR/etc/mtab"
}

test_mountRootmnt()
{
  mkdir -p "$TEST_ROOTMNT_DIR"
  mount -t tmpfs tmpfs "$TEST_ROOTMNT_DIR"
}

test_createOverbootDeviceImage()
{
  mkdir -p "$(dirname $TEST_OB_DEVICE_PATH)"
  head -c 16M /dev/zero > "$TEST_OB_DEVICE_PATH"
  mke2fs -t ext4 "$TEST_OB_DEVICE_PATH" 2>/dev/null
  mkdir -p "$TEST_MNT_DIR"

  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  mkdir -p "$TEST_MNT_DIR/$TEST_OB_REPOSITORY_DIR"
  cp -r "$TEST_RES_DIR/layers"/* "$TEST_MNT_DIR/$TEST_OB_REPOSITORY_DIR"
  sync
  umount "$TEST_MNT_DIR"
}

test_unmountAll()
{
  for u in $(seq $TEST_MAX_NESTED_MOUNTS); do
    for i in $(awk "\$2 ~ \"^$TEST_TMP_DIR\" { print \$2 }" /proc/mounts); do
      umount "$i" 2>/dev/null ||:
    done
  done
}

test_cleanup()
{
  test_unmountAll
  rm -r "$TEST_TMP_DIR"
}

test_isMounted()
{
  path="$1"
  if mount | grep -q "$path"; then
    echo 0
  else
    echo 1
  fi
}

test_success()
{
  echo 0
}


test_fail()
{
  echo 1
}

