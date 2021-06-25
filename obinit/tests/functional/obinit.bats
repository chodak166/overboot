#!/usr/bin/env bats

load helpers

setup()
{
  test_setupFakeRamfsRoot
  
  if [ ! -f "$OBINIT_BIN" ]; then
    echo "ERROR: obinit binary not found ($OBINIT_BIN), please provide OBINIT_BIN environment variable" 1>&2 && exit 1
  fi
}

teardown()
{
  test_cleanup
}

@test "obinit should mount data device as configured" {
  $OBINIT_BIN "$TEST_RAMFS_DIR"
  
  [ $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
}

@test "obinit should prepare overlay directory" {
  skip
  $OBINIT_BIN "$TEST_RAMFS_DIR"
  
  [ -d "$TEST_OB_OVERLAY_DIR" ]
  [ -d "$TEST_OB_OVERLAY_DIR/work" ]
  [ -d "$TEST_OB_OVERLAY_DIR/lower" ] # rootmnt?
  [ -d "$TEST_OB_OVERLAY_DIR/upper" ]
  [ $(test_isMounted "$TEST_OB_OVERLAY_DIR") -eq 0 ]
}

@test "obinit should use tmpfs as upper if configured" {
  skip
  $OBINIT_BIN "$TEST_RAMFS_DIR"
  mount | grep -q "tmpfs on /tmp/fstest"
  status=$?
  
  [ "$status" -eq 0 ]
}

@test "obinit should set tmpfs size as configured" {
  skip
  $OBINIT_BIN "$TEST_RAMFS_DIR"
  expectedSize=$(cat "$TEST_OB_CONFIG_PATH" | yq ".upper.size")
  actualSize=$(df -Pk /tmp/fstest/ | tail -1 | awk '{print $2}')
  
  [[ "$expectedSize" == "$actualSize" ]]
}

@test "obinit should clear upper if configured" {
  skip
  cp "$TEST_RES_DIR/configs/overboot-volatile.yaml" "$TEST_ROOTMNT_DIR/etc/overboot.yaml"
  SAMPLE_FILE="$TEST_OB_OVERLAY_DIR/upper/removeme"
  touch "$SAMPLE_FILE"
  
  $OBINIT_BIN "$TEST_RAMFS_DIR"
  
  [ ! -f "$SAMPLE_FILE" ]
}

@test "obinit should move rootmnt to the lower layer directory" {
  skip
}

@test "obinit should bind persistent upper layer if configured (no tmpfs)" {
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