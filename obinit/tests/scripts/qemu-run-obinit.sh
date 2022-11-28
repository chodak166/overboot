#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

set -e
set -x

DEBIAN_SUITE=buster
DOCKER_BUILD_IMAGE=chodak166/dev-cpp:buster-1.2
OBINIT_DIR="$SCRIPT_DIR/../.."

OBINIT_BUILD_DIR="$OBINIT_DIR/build/$DEBIAN_SUITE"
OBINIT_BIN="$OBINIT_BUILD_DIR/bin/obinit"
OBINIT_IRFS_SCRIPT="$OBINIT_DIR/apps/obinit/system/usr/share/initramfs-tools/scripts/local-bottom/obinit"
OBINIT_CONFIG_DIR="$OBINIT_DIR/tests/configs"
QEMU_DEBIAN_HELPER="$SCRIPT_DIR/qemu-debian-helper"

MNT_DIR="$SCRIPT_DIR/mnt.$(date +%s)"
VM_DIR="$SCRIPT_DIR/sysroot-$DEBIAN_SUITE"

main()
{
  trap finish EXIT

  assertCommands
  createBaseImg
  buildObinit
  installObinit

  initObRepository

  runVm
}

finish()
{
  MAX_NESTED_MOUNTS=10
  for u in $(seq $MAX_NESTED_MOUNTS); do
    for i in $(awk "\$2 ~ \"^$MNT_DIR\" { print \$2 }" /proc/mounts); do
      umount -fl "$i" 2>/dev/null || :
    done
  done
  umount -fl $MNT_DIR 2>/dev/null || :
}

assertCommands()
{
  requiredCommands=(
    debootstrap
    parted
    qemu-system-x86_64
    guestmount
    docker
  )
  
  for i in "${requiredCommands[@]}"
  do
    if ! command -v $i &>/dev/null; then
      echo "Required command '$i' not found, aborting"
      exit 1
    fi
  done
}

createBaseImg()
{
  if [ ! -d "$VM_DIR" ]; then
    "$QEMU_DEBIAN_HELPER" --create $DEBIAN_SUITE
    "$QEMU_DEBIAN_HELPER" --no-boot --run $DEBIAN_SUITE
  else
    echo "$VM_DIR present, skipping VM creation"
  fi
}

buildObinit()
{
  docker run --rm \
    -v "$OBINIT_DIR":"$OBINIT_DIR" \
    -v "$OBINIT_DIR/../cmake":"$OBINIT_DIR/../cmake" \
    $DOCKER_BUILD_IMAGE /bin/bash -c <<EOC \
" [ -d '$OBINIT_BUILD_DIR' ] && rm -r '$OBINIT_BUILD_DIR'; mkdir -p '$OBINIT_BUILD_DIR'; \
cd '$OBINIT_BUILD_DIR' && \
cmake -DOB_BUILD_TESTS=OFF -DPROJECT_VERSION_SUFFIX=-$(git rev-parse --short HEAD) '$OBINIT_DIR' && \
cmake --build . -- -j4"
EOC
}

installObinit()
{
  mkdir -p "$MNT_DIR"
  echo -n "Mounting qcow2 drive... "
  guestmount -a "$VM_DIR/sysroot-${DEBIAN_SUITE}-overlay.qcow2" -m /dev/sda1 "$MNT_DIR"
  echo "done"

  mntIrfsScript="$MNT_DIR/usr/share/initramfs-tools/scripts/local-bottom/obinit"
  if [ ! -f "$mntIrfsScript" ]; then
    echo "initramfs obinit script not found, installing and updating initramfs..."
    [ -d "$(dirname $mntIrfsScript)" ] || mkdir -p "$(dirname $mntIrfsScript)"
    cp -v "$OBINIT_IRFS_SCRIPT" "$mntIrfsScript"

    grep -q overlay /etc/initramfs-tools/modules || echo overlay >> /etc/initramfs-tools/modules
    
    chrootExec "${MNT_DIR}" update-initramfs -u
  fi

  cat "$OBINIT_CONFIG_DIR/overboot-enabled.yaml" \
  "$OBINIT_CONFIG_DIR/overboot-layers-sdb.yaml" \
  "$OBINIT_CONFIG_DIR/overboot-upper-tmpfs.yaml" \
  "$OBINIT_CONFIG_DIR/overboot-durables-all.yaml" \
   > "$MNT_DIR/etc/overboot.yaml"

  cp -v "$OBINIT_BIN" "$MNT_DIR/sbin/"
  cp -v "$OBINIT_DIR/apps/obinit/system/usr/bin/obhelper" "$MNT_DIR/usr/bin/obhelper"
  chmod +x "$MNT_DIR/usr/bin/obhelper"

  echo -n "Unmounting qcow2 drive... "
  umount -lf "$MNT_DIR"
  rmdir "$MNT_DIR"
  echo "done"
  sleep 1s
}

initObRepository()
{
  mntDir="$SCRIPT_DIR/mnt.$(date +%s)"
  mkdir -p "$mntDir"
  mount "$VM_DIR/data.img" "$mntDir"

  mkdir "$mntDir/overboot" 2>/dev/null ||:
  cp -r "$OBINIT_DIR/tests/functional/res/layers" "$mntDir/overboot/"

  umount -lf "$mntDir"
  rmdir "$mntDir"
}

chrootExec()
{
  mntDir="$1"
  shift
  cmd="su - -c '${@}'"

  mkdir -p       "${mntDir}/proc"
  mkdir -p       "${mntDir}/sys"
  mkdir -p       "${mntDir}/dev"
  mkdir -p       "${mntDir}/dev/pts"
 
  mount --bind /proc      "${mntDir}/proc"
  mount --bind /sys       "${mntDir}/sys"
  mount --bind /dev       "${mntDir}/dev"
  mount --bind /dev/pts   "${mntDir}/dev/pts"
  
  chroot "${mntDir}" /bin/bash -c "HOME=/root LC_ALL=C LANG=C.UTF-8 TERM=xterm-256color $cmd"
  sync

  umount -lf "${mntDir}/proc"
  umount -lf "${mntDir}/sys"
  umount -lf "${mntDir}/dev/pts"
  umount -lf "${mntDir}/dev"
}

runVm()
{
  "$QEMU_DEBIAN_HELPER" --run $DEBIAN_SUITE
}

# ---------

main
