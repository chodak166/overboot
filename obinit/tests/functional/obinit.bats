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
    vgLog="$TEST_TMP_DIR/valgrind-${RANDOM}.log"
    vgExitCode=0
    valgrind -q --leak-check=full --track-origins=yes \
      "$@" 2> "$vgLog" || vgExitCode=$?

    leaks=true
    cat "$vgLog" | grep -q "lost" || leaks=false
    cat "$vgLog"
    [ $leaks = false ]
  else
    "$@"
    vgExitCode=$?
  fi
}

@test "obinit should mount data device by path" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  [ $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should not mount data device and exit with success when disabled in the configuration file" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-disabled.yaml"
   
  [ ! $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
  [ $vgExitCode -eq 0 ]
}


@test "obinit should mount data device by UUID" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-uuid.yaml" ||:

  [ $(test_isMounted "$TEST_OB_DEVICE_MNT_PATH") -eq 0 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should prepare overlay directory" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
   
  [ -d "$TEST_OB_OVERLAY_DIR" ]
  [ -d "$TEST_OB_OVERLAY_DIR/work" ]
  [ -d "$TEST_OB_OVERLAY_DIR/lower-root" ]
  [ -d "$TEST_OB_OVERLAY_DIR/upper" ]
  [ $(test_isMounted "$TEST_OB_OVERLAY_DIR") -eq 0 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should set tmpfs size as configured" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  expectedSize=$(cat "$TEST_OB_CONFIG_PATH" | yq -r ".upper.size")
  actualSize=$(df -Pk "$TEST_OB_OVERLAY_DIR" | tail -1 | awk '{print $2}')k
 
  echo "expected size: $expectedSize"
  echo "actual size: $actualSize"
  [[ "$expectedSize" == "$actualSize" ]]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind upper directory from the persistent device if configured" {

  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-persistent.yaml"

  SAMPLE_FILE="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/upper/binded_upper_sample_file"
  BINDED_SAMPLE_FILE="$TEST_ROOTMNT_BINDINGS_DIR/upper/binded_upper_sample_file"
 
  mkdir -p $(dirname "$SAMPLE_FILE")
  echo $RANDOM > "$SAMPLE_FILE"
 
  mount
 
  mount | grep -q "$TEST_ROOTMNT_BINDINGS_DIR/upper type"
  mntStatus=$?
   
  mntTmpfsStatus=0
  mount | grep -q "$TEST_ROOTMNT_BINDINGS_DIR/upper type tmpfs" || mntTmpfsStatus=1
  
  cmp "$SAMPLE_FILE" "$BINDED_SAMPLE_FILE"
  [ "$mntStatus" -eq 0 ]
  [ "$mntTmpfsStatus" -eq 1 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should clear upper if configured" {
  SAMPLE_FILE="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/upper/subdir/removeme"
  mkdir -p $(dirname "$SAMPLE_FILE")
  touch "$SAMPLE_FILE"
   
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-volatile.yaml"
   
  [ ! -f "$SAMPLE_FILE" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should move rootmnt to the lower layer directory" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
 
  [ -f "$TEST_OB_OVERLAY_DIR/lower-root/etc/fstab" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should mount overlayfs in rootmnt when the upper layer is tmpfs" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  testFileName=test-$RANDOM.file
  touch "$TEST_ROOTMNT_DIR/$testFileName"
 
  [ -f "$TEST_OB_OVERLAY_DIR/upper/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should mount overlayfs in rootmnt when the upper layer is persistent" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-persistent.yaml" > /tmp/obinit.log
  testFileName=test-$RANDOM.file
  touch "$TEST_ROOTMNT_DIR/$testFileName"
 
  [ -f "$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/upper/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind ramfs overboot directory into rootmnt" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
 
  testFileName=test-$RANDOM.file
  touch "$TEST_ROOTMNT_DIR/$testFileName"
 
  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/lower-root" ]
  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/upper" ]
  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/work" ]
  [ -f "$TEST_ROOTMNT_BINDINGS_DIR/upper/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind layer repository if the configuration says so" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"

  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/layers" ]
  [ -f "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_LAYER_1_NAME/root/etc/layer.yaml" ]
  [ $vgExitCode -eq 0 ]
}


@test "obinit should bind durable directories and overlay origin if the configuration says so" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"

  testFileName="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_1/$testFileName"
  
  [ ! -f "$TEST_ROOTMNT_DIR/$TEST_DURABLES_DIR_1/orig.txt" ]
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_1/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable directories and copy origin if the configuration says so" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"

  testFileName="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/$testFileName"
  
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_2/$testFileName" ]
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_2/orig.txt" ]
  [ -f "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/orig.txt" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable directories and skip copying origin if the persistent directory already exists" {
  testFileName="${RANDOM}-$(date +%s).test"
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  mountedDurableDir="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/durables/$TEST_DURABLE_DIR_2"
  mkdir -p "$mountedDurableDir"
  touch "$mountedDurableDir/$testFileName"

  umount -fl "$TEST_MNT_DIR"
  
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  
  [ ! -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_2/orig.txt" ]
  [ ! -f "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/orig.txt" ]
  [ -f "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable file" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"

  persistentFile="$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_FILE_1"
  bindedFile="$TEST_ROOTMNT_DIR/$TEST_DURABLE_FILE_1"
  testValue=${RANDOM}-$(date +%s)
  
  echo -n $testValue > "$bindedFile"  
  cmp "$persistentFile" "$bindedFile"
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable file and copy the original file if the configuration says so and the persistent file is not present" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  
  copiedFile="$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_FILE_2"

  [ -f "$copiedFile" ]
  [[ "$(cat "$copiedFile")" == "$TEST_DURABLE_VALUE" ]]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should bind durable file and skip copying the original file if the persistent one is present" {
  testValue=${RANDOM}-$(date +%s)
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"
  newDurableFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/durables/$TEST_DURABLE_FILE_2"
  mkdir -p "$(dirname "$newDurableFile")"
  echo $testValue > "$newDurableFile"

  umount -fl "$TEST_MNT_DIR"
  
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"

  persistentFile="$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_FILE_2"

  [ -f "$persistentFile" ]
  [[ "$(cat "$persistentFile")" == "$testValue" ]]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should update fstab" {
  ESCAPED_ROOTMNT_DIR=$(sed 's/[&/\]/\\&/g' <<<"$TEST_ROOTMNT_DIR")
  sed -i "s/%rootmnt%/$ESCAPED_ROOTMNT_DIR/g" "$TEST_RAMFS_DIR/etc/mtab"

  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"

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
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml"
  
  layersDir="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/layers"
  expectedValue1=$(cat "$layersDir/$TEST_LAYER_1_NAME/root/test/file1")
  expectedValue2=$(cat "$layersDir/$TEST_LAYER_2_NAME/root/test/file2")
  expectedValue3=$(cat "$layersDir/$TEST_LAYER_3_NAME/root/test/file3")
  
  value1=$(cat "$TEST_ROOTMNT_DIR/test/file1") 
  value2=$(cat "$TEST_ROOTMNT_DIR/test/file2") 
  value3=$(cat "$TEST_ROOTMNT_DIR/test/file3") 

  [[ "$value1" == "$expectedValue1" ]]
  [[ "$value2" == "$expectedValue2" ]]
  [[ "$value3" == "$expectedValue3" ]]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should assume 'root' head layer if not configured" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs-nohead.yaml"

  [ ! -f "$TEST_ROOTMNT_DIR/test/file1" ]
  [ -f "$TEST_OB_CONFIG_PATH" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should not mount root if the bottom layer's underlayer is 'none'" {
  # alt-root.obld uses 'none' underlayer
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-tmpfs-altroot.yaml"

  [ ! -f "$TEST_OB_CONFIG_PATH" ]
  [ $vgExitCode -eq 0 ]
}


@test "obinit should use config_dir field and read all config files inside given directory" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-include.yaml"

  testFileName1="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_1/$testFileName1"
  
  testFileName2="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_2/$testFileName2"
  
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_1/$testFileName1" ]
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_2/$testFileName2" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should use rootmnt subdirectory as a device if the configuration doesn't point to a file or a device" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-embedded-tmpfs.yaml"
  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_INNER_LAYER_NAME" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should block write access to the embedded repository directory via mounted overlayfs" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-embedded-tmpfs.yaml"

  testFileName="${RANDOM}-$(date +%s).test"
  canTouchThis=1
  touch "$TEST_INNER_DEV_DIR/$testFileName" 2>/dev/null || canTouchThis=0

  [ -d "$TEST_INNER_DEV_DIR" ]
  [ $canTouchThis -eq 0 ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should fail if the embedded repository is used without tmpfs upper layer" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-embedded-persistent.yaml"

  [ $vgExitCode -eq 1 ]
  [ ! -d "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_INNER_LAYER_NAME" ]
}

@test "obinit should allow write access to the embedded repository directory via durables" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-embedded-tmpfs.yaml"

  testFileName="${RANDOM}-$(date +%s).test"
  touch "$TEST_ROOTMNT_DIR/$TEST_DURABLE_DIR_1/$testFileName"
  
  [ -f "$TEST_DURABLES_STORAGE_DIR/$TEST_DURABLE_DIR_1/$testFileName" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should use embedded image as a device if the configuration points to an image file inside rootmnt (tmpfs)" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-embedded-img-tmpfs.yaml"

  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_INNER_LAYER_NAME" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should use embedded image as a device if the configuration points to an image file inside rootmnt (persistent)" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-embedded-img-persistent.yaml"

  [ -d "$TEST_ROOTMNT_BINDINGS_DIR/layers/$TEST_INNER_LAYER_NAME" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should restore original rootmnt after rollback" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-rollback.yaml"

  [ -f "$TEST_ROOTMNT_DIR/etc/fstab" ]

  umount -fl "$TEST_ROOTMNT_DIR"

  tmpInMount=1
  mount | grep -q "$TEST_TMP_DIR" || tmpInMount=0

  [ $vgExitCode -ne 0 ]
  [ $tmpInMount -eq 0 ]
  [ ! -d "$TEST_OB_OVERLAY_DIR" ]
  [ ! -d "$TEST_OB_DEVICE_MNT_PATH" ]
}

@test "obinit should rollback when the layer chain is broken" {
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-wrong-head.yaml"
   
  [ $vgExitCode -ne 0 ]
  [ ! -d "$TEST_OB_OVERLAY_DIR" ]
}

# --- jobs ---

@test "obinit should update the configuration file on update-config job" {
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  oldConfig="$TEST_TMP_DIR/overboot.yaml"
  newConfig="$TEST_CONFIGS_DIR/overboot-embedded-tmpfs.yaml"
  jobFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME/update-config"

  cp "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml" "$oldConfig"
  cp "$newConfig" "$jobFile"

  oldConfigSum=$(md5sum "$oldConfig" | awk '{print $1}')
  newConfigSum=$(md5sum "$newConfig" | awk '{print $1}')

  [[ $oldConfigSum != $newConfigSum ]]

  umount -lf "$TEST_MNT_DIR"
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$oldConfig"
   
  oldConfigSum=$(md5sum "$oldConfig" | awk '{print $1}')

  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"
  [[ $oldConfigSum == $newConfigSum ]]
  [ ! -f "$jobFile" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should load and apply the new configuration after the update-config job is finished" {
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  oldConfig="$TEST_TMP_DIR/overboot.yaml"
  newConfig="$TEST_CONFIGS_DIR/overboot-embedded-tmpfs.yaml"
  jobFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME/update-config"

  cp "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml" "$oldConfig"
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

  configDir="$TEST_ROOTMNT_DIR/etc/test-overboot.d"

  configName="my-config-${RANDOM}.yaml"
  newConfig="$TEST_TMP_DIR/$configName"
  echo ${RANDOM} > "$newConfig"
  newConfigSum=$(md5sum "$newConfig" | awk '{print $1}')
  jobFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME/install-config-$configName"

  mount -o remount,rw "$TEST_ROOTMNT_DIR"
  cp "$TEST_CONFIGS_DIR/overboot-tmpfs.yaml" "$TEST_ROOTMNT_DIR/etc/overboot.yaml"
  mount -o remount,ro "$TEST_ROOTMNT_DIR"

  cp "$newConfig" "$jobFile"
  
  umount -lf "$TEST_MNT_DIR"
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_ROOTMNT_DIR/etc/overboot.yaml"
   
  installedConfig="$configDir/$configName"

  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  [ -f "$installedConfig" ]
  cmp "$newConfig" "$installedConfig"
  [ ! -f "$jobFile" ]
  [ $vgExitCode -eq 0 ]
}

@test "obinit should create a new layer from the upper layer on commit job" {
  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"

  testFileName=test-$RANDOM.file
  upperTestFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/upper/$testFileName"
  mkdir -p $(dirname "$upperTestFile") ||:
  testValue=$RANDOM
  echo $testValue > "$upperTestFile"
  
  jobFile="$TEST_MNT_DIR/$TEST_OB_REPOSITORY_NAME/$TEST_JOBS_DIR_NAME/commit"
  layerName="new-layer-$RANDOM"
  echo "name: $layerName" > "$jobFile"
  jobFileSum=$(md5sum "$jobFile" | awk '{print $1}')

  umount -lf "$TEST_MNT_DIR"
  vgRun $OBINIT_BIN -r "$TEST_RAMFS_DIR" -c "$TEST_CONFIGS_DIR/overboot-persistent.yaml"

  newLayer="$TEST_OB_DEVICE_MNT_PATH/$TEST_OB_REPOSITORY_NAME/layers/${layerName}.obld"
  layerMetaFileSum=$(md5sum "$newLayer/root/etc/layer.yaml" | awk '{print $1}')

  mount -o loop "$TEST_OB_DEVICE_PATH" "$TEST_MNT_DIR"
  [ -d "$newLayer" ]
  [ ! -f "$upperTestFile" ]
  [[ $(cat "$newLayer/root/$testFileName") == $testValue ]]
  [[ $jobFileSum == $layerMetaFileSum ]]
  [ ! -f "$jobFile" ]
  [ $vgExitCode -eq 0 ]
}

# TODO: make new layer from root?