#!/bin/bash

# /                              # initramfs root
# ├── obmnt                      # persistent device mount point
# │   └── overboot               # repository directory
# │       ├── durables           # durables storage directory
# │       ├── upper              # overlayfs upper layer data (if configured)
# │       └── layers             # layers directory
# ├── overlay                    # overlayfs parent directory
# │   ├── lower-root             # the lowest layer, moved /root
# │   ├── upper                  # upper layer (overlayfs changes, bind from /obdev if configured)
# │   └── work                   # overlayfs work directory
# └── root                       # $rootmnt, final root mounted as overlayfs
#     ├── etc                    # user /etc dir
#     │   ├── layer.yaml         # head layer metadata (RO)
#     │   └── overboot.yaml      # overboot config file, (RW durable)
#     └── overboot               # user bindings directory
#         ├── layers             # /obdev/overboot/layers binding
#         ├── lower-root         # /overlay/lower-root binding
#         └── upper              # /overlay/upper binding

#set -x
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/../.."
TESTS_DIR="$PROJECT_ROOT/tests"
TEST_RES_DIR="$TESTS_DIR/functional/res"
TEST_CONFIGS_DIR="$TESTS_DIR/configs"

if [[ -z "$TEST_TMP_DIR" ]]; then
  TEST_TMP_DIR="$SCRIPT_DIR/tmp_$(date +%s)_${RANDOM}"
fi

TEST_MNT_DIR="$TEST_TMP_DIR/mnt"

TEST_RAMFS_DIR="$TEST_TMP_DIR/ramfs"
TEST_ROOTMNT_DIR="$TEST_RAMFS_DIR/root"
TEST_ROOTMNT_BINDINGS_DIR="$TEST_ROOTMNT_DIR/overboot"
TEST_OB_DEVICE_UUID="ddbdd956-4bef-4c43-b99f-6a7ae2a95a86"
TEST_OB_DEVICE_PATH="$TEST_RAMFS_DIR/dev/ob_test.img"
TEST_OB_DEVICE_MNT_PATH="$TEST_RAMFS_DIR/obmnt"
TEST_OB_REPOSITORY_NAME="overboot"
TEST_DURABLES_STORAGE_DIR="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/durables"
TEST_EMBEDDED_IMG="$TEST_ROOTMNT_DIR/var/obdev.img"
TEST_OB_LOCK_NAME="obinit.lock"

TEST_DURABLE_DIR_1="/test/durables-dir-1"
TEST_DURABLE_DIR_2="/test/durables-dir-2"
TEST_DURABLE_FILE_1="/test/persistent_file_1"
TEST_DURABLE_FILE_2="/test/persistent_file_2"
TEST_DURABLE_DIR_NO_ORIGIN="/test/durable_dir_with_no_origin"
TEST_DURABLE_FILE_NO_ORIGIN="/test/durable_file_with_no_origin"
TEST_DURABLE_VALUE="durable value"

TEST_LAYER_1_NAME="first-test-layer.obld"
TEST_LAYER_2_NAME="second-test-layer.obld"
TEST_LAYER_3_NAME="third-test-layer.obld"

TEST_OB_OVERLAY_DIR="$TEST_RAMFS_DIR/overlay"

TEST_INNER_DEV_DIR="$TEST_ROOTMNT_DIR/var/obdev.d"
TEST_INNER_LAYER_NAME="internal-test-layer"
TEST_INNER_LAYER_DIR="$TEST_INNER_DEV_DIR/$TEST_OB_REPOSITORY_NAME/layers/$TEST_INNER_LAYER_NAME"

TEST_JOBS_DIR_NAME="jobs"

TEST_MAX_NESTED_MOUNTS=4
TEST_LOOP_DEVICE=
TEST_LOOP_DEVICE_LINK=/dev/ob_test_device

test_setupFakeRamfsRoot() {
  test_createOverbootDeviceImage
  test_mountRootmnt

  mkdir -p "$TEST_ROOTMNT_DIR/dev"
  mkdir -p "$TEST_ROOTMNT_DIR/etc"
  mkdir -p "$TEST_RAMFS_DIR/etc"
  mkdir -p "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_1"
  mkdir -p "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2"
  echo "Creating inner layer dir: $TEST_INNER_LAYER_DIR"
  mkdir -p "$TEST_INNER_LAYER_DIR"

  echo -n "$TEST_DURABLE_VALUE" >"$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_1/orig.txt"
  echo -n "$TEST_DURABLE_VALUE" >"$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/orig.txt"
  echo -n "$TEST_DURABLE_VALUE" >"$TEST_ROOTMNT_DIR/$TEST_DURABLE_FILE_1"
  echo -n "$TEST_DURABLE_VALUE" >"$TEST_ROOTMNT_DIR/$TEST_DURABLE_FILE_2"

  cp -v "$TEST_RES_DIR/fstab" "$TEST_ROOTMNT_DIR/etc/fstab"
  cp "$TEST_RES_DIR/fstab-parsed" "$TEST_TMP_DIR/"
  cp "$TEST_RES_DIR/mtab" "$TEST_RAMFS_DIR/etc/mtab"

  ESCAPED_ROOTMNT_DIR=$(sed 's/[&/\]/\\&/g' <<<"$TEST_ROOTMNT_DIR")
  sed -i "s/%rootmnt%/$ESCAPED_ROOTMNT_DIR/g" "$TEST_RAMFS_DIR/etc/mtab"

  head -c 4M /dev/zero > "$TEST_EMBEDDED_IMG"
  mke2fs -t ext4 "$TEST_EMBEDDED_IMG"
  mount -o loop "$TEST_EMBEDDED_IMG" "$TEST_MNT_DIR"
  mkdir -p "$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/layers/$TEST_INNER_LAYER_NAME"
  umount -fl "$TEST_MNT_DIR"

  sync "$TEST_ROOTMNT_DIR"

  # sometimes root is busy, why?
  n=5
  while [ $n -ge 0 ] && ! mount -o remount,ro "$TEST_ROOTMNT_DIR"
  do
    echo "root mount point busy, retrying"
    n=$((n-1));
    sleep 0.2s
  done

  if [ $n -eq 0 ]; then
    echo "Cannot remount $TEST_ROOTMNT_DIR"
    exit 1
  fi
}

test_mountRootmnt() {
  opt="$1"
  mkdir -p "$TEST_ROOTMNT_DIR"
  mount -t tmpfs $opt tmpfs "$TEST_ROOTMNT_DIR"
}

test_createOverbootDeviceImage() {
  mkdir -p "$(dirname $TEST_OB_DEVICE_PATH)"
  head -c 16M /dev/zero >"$TEST_OB_DEVICE_PATH"
  mke2fs -t ext4 "$TEST_OB_DEVICE_PATH" 2>/dev/null
  echo "y" | tune2fs -U "$TEST_OB_DEVICE_UUID" "$TEST_OB_DEVICE_PATH"

  mkdir -p "$TEST_MNT_DIR"

  TEST_LOOP_DEVICE=$(losetup -P --show -f "$TEST_OB_DEVICE_PATH")
  ln -f $TEST_LOOP_DEVICE $TEST_LOOP_DEVICE_LINK

  mount $TEST_LOOP_DEVICE "$TEST_MNT_DIR"

  mkdir -p "$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME"
  cp -r "$TEST_RES_DIR/layers" "$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/"
  mkdir -p "$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME"
  sync
  umount -fl "$TEST_MNT_DIR"
}

test_unmountAll() {
  for u in $(seq $TEST_MAX_NESTED_MOUNTS); do
    for i in $(awk "\$2 ~ \"^$TEST_TMP_DIR\" { print \$2 }" /proc/mounts); do
      umount -fl "$i" 2>/dev/null || :
    done
  done
}

test_cleanup() {
  sync -f "$TEST_TMP_DIR"
  sleep 0.1s

  losetup -d $TEST_LOOP_DEVICE
  rm $TEST_LOOP_DEVICE_LINK
  test_unmountAll
  rm -r "$TEST_TMP_DIR"
  
}

test_isMounted() {
  path="$1"
  if mount | grep -q "$path"; then
    echo 0
  else
    echo 1
  fi
}

test_success() {
  echo 0
}

test_fail() {
  echo 1
}

test_waitForCont() {
  while [ ! -f /tmp/cont ]; do
    sleep 1s
  done
  rm /tmp/cont
}

test_getPathFsType()
{
  local path="$1"
  local value=$(df -TP $path | tail -1 | awk '{print $2}')
  echo $value
}

test_composeConfig()
{
  partials=($@)
  export TEST_COMPOSED_CONFIG="$TEST_ROOTMNT_DIR/etc/overboot.yaml"
  mount -o remount,rw "$TEST_ROOTMNT_DIR"
  for partial in ${partials[@]}; do
    cat "$TEST_CONFIGS_DIR/overboot-${partial}.yaml" >> "$TEST_COMPOSED_CONFIG"
  done
  cp -r "$TEST_CONFIGS_DIR/test-overboot.d" "$TEST_ROOTMNT_DIR/etc/"
  mount -o remount,ro "$TEST_ROOTMNT_DIR"
}
