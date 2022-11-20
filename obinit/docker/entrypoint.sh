#!/bin/bash

# env variables:
# OWNER - output package owner UID
# OUT_DIR - package output directory name

PROJECT_DIR=/usr/src/overboot/obinit

main()
{
  makePackage
  makeTests
}

makePackage()
{
  pushd $PROJECT_DIR
  mkdir -p ${OUT_DIR}/source 2>/dev/null ||: 
  dpkg-buildpackage -us -uc -ui -i -I --build=source
  mv -v $PROJECT_DIR/../*.* ${OUT_DIR}/source
  dpkg-buildpackage -us -uc -ui -i -I --build=binary
  mv -v $PROJECT_DIR/../*.* ${OUT_DIR}/
  chown -v -R ${OWNER}:${OWNER} ${OUT_DIR}
  popd
}

makeTests()
{
  pushd $PROJECT_DIR
  dir=build-$(date "+%y%m%d_%H%M%S")
  mkdir $dir && cd $dir
  cmake -DCMAKE_BUILD_TYPE=Debug \
    -DOB_BUILD_TESTS=ON \
    ..
  cmake --build . -- -j4
  #TODO: use ctest, try running bats
  cd bin
  ./ObSyncTest
  ./TaskListTest
  popd
}

main $@
