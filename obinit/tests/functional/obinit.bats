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

# run command through valgrind if present
vgRun()
{
  if command -v valgrind &>/dev/null; then
    local vgLog="$TEST_TMP_DIR/valgrind-${RANDOM}.log"
    vgExitCode=0
    valgrind -q --leak-check=full --track-origins=yes \
      "$@" 2> "$vgLog" || vgExitCode=$?

    local leaks=true
    cat "$vgLog" | grep -q "lost" || leaks=false
    cat "$vgLog"
    [ $leaks = false ]
  else
    "$@"
    vgExitCode=$?
  fi
}

@test "obinit should mount data device by path" {
  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
  [ $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should not mount data device and exit with success when disabled in the configuration file" {
  test_composeConfig disabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
   
  [ ! $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should mount data device by UUID" {
  test_composeConfig enabled layers-uuid upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG" ||:  

  [ $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should prepare overlay directory" {
  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
   
  [ -d "$TEST_OB_OVERLAY_DIR" ]
  [ -d "$TEST_OB_OVERLAY_DIR/work" ]
  [ -d "$TEST_OB_OVERLAY_DIR/lower-root" ]
  [ -d "$TEST_OB_OVERLAY_DIR/upper" ]
  [ $(test_isMounted "$TEST_OB_OVERLAY_DIR") -eq 0 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should set tmpfs size as configured" {
  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
  local expectedSize=$(cat "$TEST_COMPOSED_CONFIG" | yq -r ".upper.size")
  local actualSize=$(df -Pk "$TEST_OB_OVERLAY_DIR" | tail -1 | awk '{print $2}')k
 
  [[ "$expectedSize" == "$actualSize" ]]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind upper directory from the persistent device if configured" {
  test_composeConfig enabled layers-loop upper-persistent
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local SAMPLE_FILE="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/upper/binded_upper_sample_file"
  local BINDED_SAMPLE_FILE="$TEST_ROOTMNT_BINDINGS_DIR/upper/binded_upper_sample_file"
 
  mkdir -p $(dirname "$SAMPLE_FILE")
  echo $RANDOM > "$SAMPLE_FILE"
  
  mount | grep -q "$TEST_ROOTMNT_BINDINGS_DIR/upper type"
  local mntStatus=$?
   
  local mntTmpfsStatus=0
  mount | grep -q "$TEST_ROOTMNT_BINDINGS_DIR/upper type tmpfs" || mntTmpfsStatus=1
  
  cmp "$SAMPLE_FILE" "$BINDED_SAMPLE_FILE"
  [ "$mntStatus" -eq 0 ]
  [ "$mntTmpfsStatus" -eq 1 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should clear upper if configured" {
  local SAMPLE_FILE="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/upper/subdir/removeme"
  mkdir -p $(dirname "$SAMPLE_FILE")
  touch "$SAMPLE_FILE"
   
  test_composeConfig enabled layers-loop upper-volatile
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
   
  [ ! -f "$SAMPLE_FILE" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should move rootmnt to the lower layer directory" {
  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
 
  [ -f "$TEST_OB_OVERLAY_DIR/lower-root/etc/fstab" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should mount overlayfs upper dir from ramdisk when configured as tmpfs" {
  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
  
  local testFileName=test-$RANDOM.file
  touch "$TEST_ROOTMNT_DIR/$testFileName"
 
  [ -f "$TEST_OB_OVERLAY_DIR/upper/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should mount overlayfs upper dir from repository when configured as persistent" {
  test_composeConfig enabled layers-loop upper-persistent
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG" > /tmp/obinit.log

  local testFileName=test-$RANDOM.file
  touch "$TEST_ROOTMNT_DIR/$testFileName"
 
  [ -f "$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/upper/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should mount persistent upper under the tmpfs when the configuration says so" {
  
  local testFileName="${RANDOM}-$(date +%s).test"
  local persistentUpper="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/upper"
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"
  mkdir "$persistentUpper"
  touch "$persistentUpper/$testFileName"
  tree $TEST_RAMFS_DIR
  umount -fl "$TEST_MNT_DIR"

  test_composeConfig enabled layers-loop upper-tmpfs-include
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  [ -f "$TEST_ROOTMNT_DIR/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should not mount persistent upper under the tmpfs when the configuration says so" {
  
  local testFileName="${RANDOM}-$(date +%s).test"
  local persistentUpper="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/upper"
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"
  mkdir "$persistentUpper"
  touch "$persistentUpper/$testFileName"
  umount -fl "$TEST_MNT_DIR"

  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  [ ! -f "$TEST_ROOTMNT_DIR/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind ramfs overboot directory into rootmnt" {
  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
 
  local testFileName=test-$RANDOM.file
  touch "$TEST_ROOTMNT_DIR/$testFileName"
 
  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/lower-root" ]
  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/upper" ]
  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/work" ]
  [ -f "$TEST_ROOTMNT_BINDINGS_DIR/upper/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind layer repository if the configuration says so" {
  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/layers" ]
  [ -f "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_LAYER_1_NAME/root/etc/layer.yaml" ]
  [ $vgExitCode -eq 0 ]
}


@test "obinit should bind durable directories and overlay the origin if the configuration says so" {
  test_composeConfig enabled layers-loop upper-tmpfs durables-all
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local testFileName="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_1/$testFileName"
  
  [ ! -f "$TEST_ROOTMNT_DIR/$TEST_DURABLES_DIR_1/orig.txt" ]
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_1/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable directories and copy the original directory if the configuration says so" {
  test_composeConfig enabled layers-loop upper-tmpfs durables-all
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local testFileName="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/$testFileName"
  
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_2/$testFileName" ]
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_2/orig.txt" ]
  [ -f "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/orig.txt" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable directories and skip copying origin if the persistent directory already exists" {
  local testFileName="${RANDOM}-$(date +%s).test"
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  local mountedDurableDir="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/durables/$TEST_DURABLE_DIR_2"
  mkdir -p "$mountedDurableDir"
  touch "$mountedDurableDir/$testFileName"

  umount -fl "$TEST_MNT_DIR"
  
  test_composeConfig enabled layers-loop upper-tmpfs durables-all
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
  
  [ ! -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_2/orig.txt" ]
  [ ! -f "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/orig.txt" ]
  [ -f "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable file" {
  test_composeConfig enabled layers-loop upper-tmpfs durables-all
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local persistentFile="$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_FILE_1"
  local bindedFile="$TEST_ROOTMNT_DIR/$TEST_DURABLE_FILE_1"
  local testValue=${RANDOM}-$(date +%s)
  
  echo -n $testValue > "$bindedFile"  
  cmp "$persistentFile" "$bindedFile"
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable file and copy the original file if the configuration says so and the persistent file is not present" {
  test_composeConfig enabled layers-loop upper-tmpfs durables-all
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
  
  local copiedFile="$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_FILE_2"

  [ -f "$copiedFile" ]
  [[ "$(cat "$copiedFile")" == "$TEST_DURABLE_VALUE" ]]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable file and skip copying the original file if the persistent one is present" {
  local testValue=${RANDOM}-$(date +%s)
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"
  local newDurableFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/durables/$TEST_DURABLE_FILE_2"
  mkdir -p "$(dirname "$newDurableFile")"
  echo $testValue > "$newDurableFile"

  umount -fl "$TEST_MNT_DIR"
  
  test_composeConfig enabled layers-loop upper-tmpfs durables-all
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local persistentFile="$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_FILE_2"

  [ -f "$persistentFile" ]
  [[ "$(cat "$persistentFile")" == "$testValue" ]]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should create and bind an empty durable directory if there is no original file or directory and the default type is directory or is undefined" {
  test_composeConfig enabled layers-loop upper-tmpfs durables-all
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
  
  local createdDurable="$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_NO_ORIGIN"

  local testFileName="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_NO_ORIGIN/$testFileName"
  
  [ -d "$createdDurable" ]
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_NO_ORIGIN/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should create and bind empty durable file if there is no original file or directory and the default type is file" {
  test_composeConfig enabled layers-loop upper-tmpfs durables-all
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
  
  local createdDurable="$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_FILE_NO_ORIGIN"

  echo $RANDOM >> "$TEST_ROOTMNT_DIR/$TEST_DURABLE_FILE_NO_ORIGIN"

  [ -f "$createdDurable" ]
  cmp -s "$TEST_ROOTMNT_DIR/$TEST_DURABLE_FILE_NO_ORIGIN" "$createdDurable"
  [ $vgExitCode -eq 0 ]
}

@test "obinit should update fstab" {
  local ESCAPED_ROOTMNT_DIR=$(sed 's/[&/\]/\\&/g' <<<"$TEST_ROOTMNT_DIR")
  sed -i "s/%rootmnt%/$ESCAPED_ROOTMNT_DIR/g" "$TEST_RAMFS_DIR/etc/mtab"

  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  echo -e "\n\nmtab:"
  cat "$TEST_RAMFS_DIR/etc/mtab"
  echo -e "\n\nexpected fstab:"
  cat "$TEST_TMP_DIR/fstab-parsed"
  echo -e "\n\nactual fstab:"
  cat "$TEST_ROOTMNT_DIR/etc/fstab"

  cmp -s "$TEST_ROOTMNT_DIR/etc/fstab" "$TEST_TMP_DIR/fstab-parsed"
  [ $vgExitCode -eq 0 ]
}

@test "obinit should mount all the layers required by the head layer" {
  test_composeConfig enabled layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
  
  local layersDir="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/layers"
  local expectedValue1=$(cat "$layersDir/$TEST_LAYER_1_NAME/root/test/file1")
  local expectedValue2=$(cat "$layersDir/$TEST_LAYER_2_NAME/root/test/file2")
  local expectedValue3=$(cat "$layersDir/$TEST_LAYER_3_NAME/root/test/file3")
  
  local value1=$(cat "$TEST_ROOTMNT_DIR/test/file1") 
  local value2=$(cat "$TEST_ROOTMNT_DIR/test/file2") 
  local value3=$(cat "$TEST_ROOTMNT_DIR/test/file3") 

  [[ "$value1" == "$expectedValue1" ]]
  [[ "$value2" == "$expectedValue2" ]]
  [[ "$value3" == "$expectedValue3" ]]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should assume 'root' head layer if not configured" {
  test_composeConfig enabled layers-nohead upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  [ ! -f "$TEST_ROOTMNT_DIR/test/file1" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should not mount root if the bottom layer's underlayer is 'none'" {
  # alt-root.obld uses 'none' underlayer
  test_composeConfig enabled layers-alt-root upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  [ ! -f "$TEST_ROOTMNT_DIR/etc/mtab" ]
  [ $vgExitCode -eq 0 ]
}


@test "obinit should use config_dir field and read all config files inside given directory" {
  test_composeConfig enabled config-dir layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local testFileName1="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_1/$testFileName1"
  
  local testFileName2="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/$testFileName2"
  
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_1/$testFileName1" ]
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_2/$testFileName2" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should use rootmnt subdirectory as a device if the configuration doesn't point to a file or a device" {
  test_composeConfig enabled layers-dir upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_INNER_LAYER_NAME" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should block write access to the embedded repository directory via mounted overlayfs" {
  test_composeConfig enabled layers-dir upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local testFileName="${RANDOM}-$(date +%s).test"
  local canTouchThis=1
  touch "$TEST_INNER_DEV_DIR/$testFileName" 2>/dev/null || canTouchThis=0

  [ -d "$TEST_INNER_DEV_DIR" ]
  [ $canTouchThis -eq 0 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should fail if the embedded repository is used without tmpfs upper layer" {
  test_composeConfig enabled layers-dir upper-persistent
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  [ $vgExitCode -eq 1 ]
  [ ! -d "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_INNER_LAYER_NAME" ]
}

@test "obinit should allow write access to the embedded repository directory via durables" {
  test_composeConfig enabled layers-dir upper-tmpfs durables-all
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local testFileName="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_1/$testFileName"
  
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_1/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should use embedded image as a device if the configuration points to an image file inside rootmnt (tmpfs)" {
  test_composeConfig enabled layers-img upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_INNER_LAYER_NAME" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should use embedded image as a device if the configuration points to an image file inside rootmnt (persistent)" {
  test_composeConfig enabled layers-img upper-persistent
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_INNER_LAYER_NAME" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should restore original rootmnt after rollback" {
  test_composeConfig enabled layers-loop upper-tmpfs rollback
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  [ -f "$TEST_ROOTMNT_DIR/etc/fstab" ]

  umount -fl "$TEST_ROOTMNT_DIR"

  local tmpInMount=1
  mount | grep -q "$TEST_TMP_DIR" || tmpInMount=0

  [ $vgExitCode -ne 0 ]
  [ $tmpInMount -eq 0 ]
  [ ! -d "$TEST_OB_OVERLAY_DIR" ]
  [ ! -d "$TEST_OB_DEVICE_MNT_PATH" ]
}

@test "obinit should rollback when the layer chain is broken" {
  test_composeConfig enabled layers-wrong-head upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"
   
  [ $vgExitCode -ne 0 ]
  [ ! -d "$TEST_OB_OVERLAY_DIR" ]
}

# --- safe mode ---

# --- jobs ---

@test "obinit should update the configuration file on update-config job" {
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  local newConfig="$TEST_CONFIGS_DIR/overboot-embedded-tmpfs.yaml"
  test_composeConfig enabled layers-dir upper-tmpfs
  cp "$TEST_COMPOSED_CONFIG" "$newConfig"

  test_composeConfig enabled layers-loop upper-tmpfs
  local oldConfig="$TEST_COMPOSED_CONFIG"
  local jobFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME/update-config"

  cp "$newConfig" "$jobFile"

  local oldConfigSum=$(md5sum "$oldConfig" | awk '{print $1}')
  local newConfigSum=$(md5sum "$newConfig" | awk '{print $1}')

  [[ $oldConfigSum != $newConfigSum ]]

  umount -lf "$TEST_MNT_DIR"
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$oldConfig"
   
  local oldConfigSum=$(md5sum "$oldConfig" | awk '{print $1}')

  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"
  [[ $oldConfigSum == $newConfigSum ]]
  [ ! -f "$jobFile" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should load and apply the new configuration after the update-config job is finished" {
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  local newConfig="$TEST_CONFIGS_DIR/overboot-embedded-tmpfs.yaml"
  test_composeConfig enabled layers-dir upper-tmpfs
  cp "$TEST_COMPOSED_CONFIG" "$newConfig"

  test_composeConfig enabled layers-loop upper-tmpfs
  local oldConfig="$TEST_COMPOSED_CONFIG"
  local jobFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME/update-config"

  cp "$newConfig" "$jobFile"

  umount -lf "$TEST_MNT_DIR"
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$oldConfig"
   
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"
 
  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_INNER_LAYER_NAME" ]
  [ ! -f "$jobFile" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should copy new partial configuration file to config directory on install-config job" {
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  local configDir="$TEST_ROOTMNT_DIR/etc/test-overboot.d"

  local configName="my-config-${RANDOM}.yaml"
  local newConfig="$TEST_TMP_DIR/$configName"
  echo ${RANDOM} > "$newConfig"
  local newConfigSum=$(md5sum "$newConfig" | awk '{print $1}')
  local jobFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME/install-config-$configName"

  cp "$newConfig" "$jobFile"
  
  umount -lf "$TEST_MNT_DIR"

  test_composeConfig enabled config-dir layers-loop upper-tmpfs
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_ROOTMNT_DIR/etc/overboot.yaml"
   
  local installedConfig="$configDir/$configName"

  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  [ -f "$installedConfig" ]
  cmp "$newConfig" "$installedConfig"
  [ ! -f "$jobFile" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should create a new layer from the upper layer on commit job" {
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  local testFileName=test-$RANDOM.file
  local upperTestFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/upper/$testFileName"
  mkdir -p $(dirname "$upperTestFile") ||:
  local testValue=$RANDOM
  echo $testValue > "$upperTestFile"
  
  local jobFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME/commit"
  local layerName="new-layer-$RANDOM"
  echo "name: $layerName" > "$jobFile"
  local jobFileSum=$(md5sum "$jobFile" | awk '{print $1}')

  umount -lf "$TEST_MNT_DIR"
  test_composeConfig enabled layers-loop upper-persistent
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local newLayer="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/layers/${layerName}.obld"
  local layerMetaFileSum=$(md5sum "$newLayer/root/etc/layer.yaml" | awk '{print $1}')

  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"
  [ -d "$newLayer" ]
  [ ! -f "$upperTestFile" ]
  [[ $(cat "$newLayer/root/$testFileName") == $testValue ]]
  [[ $jobFileSum == $layerMetaFileSum ]]
  [ ! -f "$jobFile" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should move whiteout files on commit job" {
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  local whiteoutFileName=whiteout_test_$RANDOM
  local upperWhiteoutFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/upper/$whiteoutFileName"
  mkdir -p $(dirname "$upperWhiteoutFile") ||:
  mknod "$upperWhiteoutFile" c 0 0
  
  local jobFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME/commit"
  local layerName="new-layer-$RANDOM"
  echo "name: $layerName" > "$jobFile"
  local jobFileSum=$(md5sum "$jobFile" | awk '{print $1}')

  umount -lf "$TEST_MNT_DIR"
  test_composeConfig enabled layers-loop upper-persistent
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_COMPOSED_CONFIG"

  local newLayer="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/layers/${layerName}.obld"
  local layerMetaFileSum=$(md5sum "$newLayer/root/etc/layer.yaml" | awk '{print $1}')

  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  [ -d "$newLayer" ]
  [ ! -c "$upperWhiteoutFile" ]
  [ -c "$newLayer/root/$whiteoutFileName" ]
  [ ! -f "$jobFile" ]
  [ $vgExitCode -eq 0 ]
}

