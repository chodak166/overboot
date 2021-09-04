#!/bin/env bats

load ../scripts/helpers

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


#@test "obinit should mount data device as configured" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
  
#  [ $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
#}

#@test "obinit should prepare overlay directory" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
  
#  [ -d "$TEST_OB_OVERLAY_DIR" ]
#  [ -d "$TEST_OB_OVERLAY_DIR/work" ]
#  [ -d "$TEST_OB_OVERLAY_DIR/lower" ] # rootmnt?
#  [ -d "$TEST_OB_OVERLAY_DIR/upper" ]
#  [ $(test_isMounted "$TEST_OB_OVERLAY_DIR") -eq 0 ]
#}

#@test "obinit should set tmpfs size as configured" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
#  expectedSize=$(cat "$TEST_OB_CONFIG_PATH" | yq -r ".upper.size")
#  actualSize=$(df -Pk "$TEST_OB_OVERLAY_DIR" | tail -1 | awk '{print $2}')k

#  echo "expected size: $expectedSize"
#  echo "actual size: $actualSize"
#  [[ "$expectedSize" == "$actualSize" ]]
#}

#@test "obinit should bind upper directory from the persistent device if configured" {
#  cp "$TEST_RES_DIR/configs/overboot-persistent.yaml" "$TEST_ROOTMNT_DIR/etc/overboot.yaml"
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"

#  SAMPLE_FILE="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_DIR/upper/binded_upper_sample_file"
#  BINDED_SAMPLE_FILE="$TEST_OB_OVERLAY_DIR/upper/binded_upper_sample_file"

#  mkdir -p $(dirname "$SAMPLE_FILE")
#  echo $RANDOM > "$SAMPLE_FILE"

#  mount | grep -q "$TEST_OB_OVERLAY_DIR/upper type"
#  mntStatus=$?
  
#  mntTmpfsStatus=0
#  mount | grep -q "$TEST_OB_OVERLAY_DIR/upper type tmpfs" || mntTmpfsStatus=1

#  cmp "$SAMPLE_FILE" "$BINDED_SAMPLE_FILE"
#  [ "$mntStatus" -eq 0 ]
#  [ "$mntTmpfsStatus" -eq 1 ]
#}

#@test "obinit should clear upper if configured" {
#  cp "$TEST_RES_DIR/configs/overboot-volatile.yaml" "$TEST_ROOTMNT_DIR/etc/overboot.yaml"
#  SAMPLE_FILE="$TEST_OB_OVERLAY_DIR/upper/subdir/removeme"
#  mkdir -p $(dirname "$SAMPLE_FILE")
#  touch "$SAMPLE_FILE"
  
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
  
#  [ ! -f "$SAMPLE_FILE" ]
#}

#@test "obinit should move rootmnt to the lower layer directory" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"

#  [ -f "$TEST_OB_OVERLAY_DIR/lower/etc/fstab" ]
#}

#@test "obinit should mount overlayfs in rootmnt" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
#  testFileName=test-$RANDOM.file
#  touch "$TEST_ROOTMNT_DIR/$testFileName"

#  [ -f "$TEST_OB_OVERLAY_DIR/upper/$testFileName" ]
#}

#@test "obinit should mount all layers required by the head layer" {
#  skip
#}

#@test "obinit should bind overlay directory in rootmnt" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"

#  [ -d "$TEST_ROOTMNT_DIR/overlay/lower" ]
#  [ -d "$TEST_ROOTMNT_DIR/overlay/upper" ]
#  [ -d "$TEST_ROOTMNT_DIR/overlay/work" ]
#}

#@test "obinit should bind layer repository if the configuration says so" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
#  [ -d "$TEST_ROOTMNT_DIR/overlay/layers" ]
#  [ -f "$TEST_ROOTMNT_DIR/overlay/layers/20190202-1-first-test-layer.obld/layer.yaml" ]
#}

#@test "obinit should bind durables" {
#  skip
#}

#@test "obinit should initialize durables if configured" {
#  skip
#}

@test "obinit should update fstab" {
  ESCAPED_ROOTMNT_DIR=$(sed 's/[&/\]/\\&/g' <<<"$TEST_ROOTMNT_DIR")
  sed -i "s/%rootmnt%/$ESCAPED_ROOTMNT_DIR/g" "$TEST_RAMFS_DIR/etc/mtab"

  $OBINIT_BIN -r "$TEST_RAMFS_DIR"

  tree $TEST_TMP_DIR
  
  echo -e "\n\nmtab:"
  cat "$TEST_RAMFS_DIR/etc/mtab"
  echo -e "\n\nexpected fstab:"
  cat "$TEST_TMP_DIR/fstab-parsed"
  echo -e "\n\nactual fstab:"
  cat "$TEST_ROOTMNT_DIR/etc/fstab"

  cmp -s "$TEST_ROOTMNT_DIR/etc/fstab" "$TEST_TMP_DIR/fstab-parsed"
}

#@test "obinit restore original rootmnt after failure" {
#  skip
#}
