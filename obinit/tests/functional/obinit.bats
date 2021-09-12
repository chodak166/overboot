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


# @test "obinit should mount data device as configured" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  
#  [ $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
# }

# @test "obinit should prepare overlay directory" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  
#  [ -d "$TEST_OB_OVERLAY_DIR" ]
#  [ -d "$TEST_OB_OVERLAY_DIR/work" ]
#  [ -d "$TEST_OB_OVERLAY_DIR/lower-root" ]
#  [ -d "$TEST_OB_OVERLAY_DIR/upper" ]
#  [ $(test_isMounted "$TEST_OB_OVERLAY_DIR") -eq 0 ]
# }

# @test "obinit should set tmpfs size as configured" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
#  expectedSize=$(cat "$TEST_OB_CONFIG_PATH" | yq -r ".upper.size")
#  actualSize=$(df -Pk "$TEST_OB_OVERLAY_DIR" | tail -1 | awk '{print $2}')k

#  echo "expected size: $expectedSize"
#  echo "actual size: $actualSize"
#  [[ "$expectedSize" == "$actualSize" ]]
# }

# @test "obinit should bind upper directory from the persistent device if configured" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-persistent.yaml"
  
#  SAMPLE_FILE="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/upper/binded_upper_sample_file"
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
# }

# @test "obinit should clear upper if configured" {
#  SAMPLE_FILE="$TEST_OB_OVERLAY_DIR/upper/subdir/removeme" -c "$TEST_CONFIGS_DIR/overboot-volatile.yaml"
#  mkdir -p $(dirname "$SAMPLE_FILE")
#  touch "$SAMPLE_FILE"
  
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR"
  
#  [ ! -f "$SAMPLE_FILE" ]
# }

# @test "obinit should move rootmnt to the lower layer directory" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"

#  [ -f "$TEST_OB_OVERLAY_DIR/lower-root/etc/fstab" ]
# }

# @test "obinit should mount overlayfs in rootmnt" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
#  testFileName=test-$RANDOM.file
#  touch "$TEST_ROOTMNT_DIR/$testFileName"

#  [ -f "$TEST_OB_OVERLAY_DIR/upper/$testFileName" ]
# }

# @test "obinit should mount all layers required by the head layer" {
#  skip
# }

# @test "obinit should bind overlay directory in rootmnt" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"

#  [ -d "$TEST_ROOTMNT_DIR/overlay/lower-root" ]
#  [ -d "$TEST_ROOTMNT_DIR/overlay/upper" ]
#  [ -d "$TEST_ROOTMNT_DIR/overlay/work" ]
# }

# @test "obinit should bind layer repository if the configuration says so" {
#  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
#  [ -d "$TEST_ROOTMNT_DIR/overlay/layers" ]
#  [ -f "$TEST_ROOTMNT_DIR/overlay/layers/20190202-1-first-test-layer.obld/layer.yaml" ]
# }


@test "obinit should bind durable directories and overlay origin if the configuration says so" {
  $OBINIT_BIN -r "$TEST_RAMFS_DIR"

  testFileName="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_PATH_1/$testFileName"
  
  [ ! -f "$TEST_ROOTMNT_DIR/$TEST_DURABLES_DIR_1/orig.txt" ]
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_PATH_1/$testFileName" ]
}

@test "obinit should bind durable directories and copy origin if the configuration says so" {
  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"

  testFileName="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_PATH_2/$testFileName"
  
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_PATH_2/$testFileName" ]
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_PATH_2/orig.txt" ]
  [ -f "$TEST_ROOTMNT_DIR/$TEST_DURABLE_PATH_2/orig.txt" ]
}

@test "obinit should bind durable directories and skip copying origin if the persistent directory already exists" {
  testFileName="${RANDOM}-$(date +%s).test"
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  mountedDurableDir="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/durables/$TEST_DURABLE_PATH_2"
  mkdir -p "$mountedDurableDir"
  touch "$mountedDurableDir/$testFileName"

  umount "$TEST_MNT_DIR"
  
  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  
  [ ! -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_PATH_2/orig.txt" ]
  [ ! -f "$TEST_ROOTMNT_DIR/$TEST_DURABLE_PATH_2/orig.txt" ]
  [ -f "$TEST_ROOTMNT_DIR/$TEST_DURABLE_PATH_2/$testFileName" ]
}

@test "obinit should bind durable file and copy the original file if the persistent one is not present" {
  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  #TODO
}

@test "obinit should bind durable file and skip copying the original file if the persistent one is present" {
  $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  #TODO
}



# @test "obinit should update fstab" {
#   ESCAPED_ROOTMNT_DIR=$(sed 's/[&/\]/\\&/g' <<<"$TEST_ROOTMNT_DIR")
#   sed -i "s/%rootmnt%/$ESCAPED_ROOTMNT_DIR/g" "$TEST_RAMFS_DIR/etc/mtab"

#   $OBINIT_BIN -r "$TEST_RAMFS_DIR"

#   echo -e "\n\nmtab:"
#   cat "$TEST_RAMFS_DIR/etc/mtab"
#   echo -e "\n\nexpected fstab:"
#   cat "$TEST_TMP_DIR/fstab-parsed"
#   echo -e "\n\nactual fstab:"
#   cat "$TEST_ROOTMNT_DIR/etc/fstab"

#   cmp -s "$TEST_ROOTMNT_DIR/etc/fstab" "$TEST_TMP_DIR/fstab-parsed"
# }

# @test "obinit restore original rootmnt after failure" {
#  skip
# }
