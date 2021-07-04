#!/bin/env bats

load helpers

setup()
{
  test_setupFakeRamfsRoot
  
  if [ ! -f "$OBINIT_BIN"  ]; then
    echo "ERROR: obinit binary not found ($OBINIT_BIN), please provide OBINIT_BIN environment variable" 1>&2 && exit 1
  fi
}

teardown()
{
  test_cleanup
}


@test "obinit should mount data device as configured" {
  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
  
  [ $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
}

@test "obinit should prepare overlay directory" {
  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
  
  [ -d "$TEST_OB_OVERLAY_DIR" ]
  [ -d "$TEST_OB_OVERLAY_DIR/work" ]
  [ -d "$TEST_OB_OVERLAY_DIR/lower" ] # rootmnt?
  [ -d "$TEST_OB_OVERLAY_DIR/upper" ]
  [ $(test_isMounted "$TEST_OB_OVERLAY_DIR") -eq 0 ]
}

@test "obinit should set tmpfs size as configured" {
  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
  expectedSize=$(cat "$TEST_OB_CONFIG_PATH" | yq -r ".upper.size")
  actualSize=$(df -Pk "$TEST_OB_OVERLAY_DIR" | tail -1 | awk '{print $2}')k

  echo "expected size: $expectedSize"
  echo "actual size: $actualSize"
  [[ "$expectedSize" == "$actualSize" ]]
}

@test "obinit should bind upper directory from the persistent device if configured" {
  cp "$TEST_RES_DIR/configs/overboot-persistent.yaml" "$TEST_ROOTMNT_DIR/etc/overboot.yaml"
  $OBINIT_BIN -r "$TEST_RAMFS_DIR"

  SAMPLE_FILE="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_DIR/upper/binded_upper_sample_file"
  BINDED_SAMPLE_FILE="$TEST_OB_OVERLAY_DIR/upper/binded_upper_sample_file"

  mkdir -p $(dirname "$SAMPLE_FILE")
  echo $RANDOM > "$SAMPLE_FILE"

  mount | grep -q "$TEST_OB_OVERLAY_DIR/upper type"
  mntStatus=$?
  
  mntTmpfsStatus=0
  mount | grep -q "$TEST_OB_OVERLAY_DIR/upper type tmpfs" || mntTmpfsStatus=1

  cmp "$SAMPLE_FILE" "$BINDED_SAMPLE_FILE"
  [ "$mntStatus" -eq 0 ]
  [ "$mntTmpfsStatus" -eq 1 ]
}

@test "obinit should clear upper if configured" {
  cp "$TEST_RES_DIR/configs/overboot-volatile.yaml" "$TEST_ROOTMNT_DIR/etc/overboot.yaml"
  SAMPLE_FILE="$TEST_OB_OVERLAY_DIR/upper/subdir/removeme"
  mkdir -p $(dirname "$SAMPLE_FILE")
  touch "$SAMPLE_FILE"
  
  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
  
  [ ! -f "$SAMPLE_FILE" ]
}

@test "obinit should move rootmnt to the lower layer directory" {
  skip
}

@test "obinit should mount overlayfs in rootmnt" {
  skip
}

@test "obinit should mount all layers required by the head layer" {
  skip
}

@test "obinit should bind overlay directory in rootmnt" {
  skip
}

@test "obinit should bind layer repository if configured" {
  skip
}

@test "obinit should bind durables" {
  skip
}

@test "obinit should initialize durables if configured" {
  skip
}

@test "obinit should update fstab" {
  skip
}

@test "obinit restore original rootmnt after failure" {
  skip
}
