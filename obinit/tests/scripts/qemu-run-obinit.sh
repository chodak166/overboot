#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

set -e

DEBIAN_SUITE=buster
DOCKER_BUILD_IMAGE=chodak166/dev-cpp:buster-1.1
OBINIT_DIR="$SCRIPT_DIR/../.."

OBINIT_BUILD_DIR="$OBINIT_DIR/build/$DEBIAN_SUITE"
OBINIT_BIN="$OBINIT_BUILD_DIR/bin/obinit"
OBINIT_IRFS_SCRIPT="$OBINIT_DIR/system/usr/share/initramfs-tools/scripts/local-bottom/obinit"
OBINIT_CONFIG="$OBINIT_DIR/tests/configs/overboot-tmpfs-sdb.yaml"
QEMU_DEBIAN_HELPER="$SCRIPT_DIR/qemu-debian-helper"

VM_DIR="$SCRIPT_DIR/sysroot-$DEBIAN_SUITE"

main()
{
  assertCommands
  createBaseImg
  buildObinit
  installObinit
  runVm
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
  docker run --rm -v "$OBINIT_DIR":"$OBINIT_DIR" $DOCKER_BUILD_IMAGE /bin/bash -c <<EOC \
" [ -d '$OBINIT_BUILD_DIR' ] || mkdir '$OBINIT_BUILD_DIR'; \
cd '$OBINIT_BUILD_DIR' && \
cmake -DOB_BUILD_TESTS=OFF '$OBINIT_DIR' && \
cmake --build . -- -j4"
EOC
}

installObinit()
{
  mntDir="$SCRIPT_DIR/mnt.$(date +%s)"
  mkdir "$mntDir"
  echo -n "Mounting qcow2 drive... "
  guestmount -a "$VM_DIR/sysroot-${DEBIAN_SUITE}-overlay.qcow2" -m /dev/sda1 "$mntDir"
  echo "done"

  mntIrfsScript="$mntDir/usr/share/initramfs-tools/scripts/local-bottom/obinit"
  if [ ! -f "$mntIrfsScript" ]; then
    echo "initramfs obinit script not found, installing and updating initramfs..."
    [ -d "$(dirname $mntIrfsScript)" ] || mkdir -p "$(dirname $mntIrfsScript)"
    cp -v "$OBINIT_IRFS_SCRIPT" "$mntIrfsScript"

    grep -q overlay /etc/initramfs-tools/modules || echo overlay >> /etc/initramfs-tools/modules
    
    chrootExec "${mntDir}" update-initramfs -u
  fi

  cp -v "$OBINIT_CONFIG" "$mntDir/etc/overboot.yaml"
  cp -v "$OBINIT_BIN" "$mntDir/usr/bin/"

  echo -n "Unmounting qcow2 drive... "
  umount "$mntDir"
  rmdir "$mntDir"
  echo "done"
  sleep 1s
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

  umount "${mntDir}/proc"
  umount "${mntDir}/sys"
  umount "${mntDir}/dev/pts"
  umount "${mntDir}/dev"
}

runVm()
{
  "$QEMU_DEBIAN_HELPER" --run $DEBIAN_SUITE
}

# ---------

main
