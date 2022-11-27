#!/bin/bash

# env variables:
# OWNER - output package owner UID
# OUT_DIR - package output directory name

PROJECT_NAME=overboot
PROJECT_DIR=/usr/src/${PROJECT_NAME}
PKG_DIR=${PROJECT_DIR}_pkg

main()
{
  preparePkgDir
  makeSourcePackage
  makeBinaryPackage
  makeTests
}

preparePkgDir()
{
  mkdir -p "$PKG_DIR"
  pushd "$PROJECT_DIR"

  # build orig tarball since dpkg-source will not do that
  # for quilt format:
  srcName=$(dpkg-parsechangelog -S Source)
  srcVersion=$(dpkg-parsechangelog -S Version | cut -d '-' -f 1)
  tarball=../${srcName}_${srcVersion}.orig.tar.gz

  git config --global --add safe.directory $PWD
  git ls-files -z | xargs -0 \
    tar --transform "s/^/${srcName}\//" -czvf $tarball

  popd

  pushd "$PKG_DIR"
  tar --strip-components=1 -xvf $tarball
}

makeBinaryPackage()
{
  pushd "$PKG_DIR"
  dpkg-buildpackage -us -uc -ui -i -I --build=binary
  
  mkdir -p ${OUT_DIR} 2>/dev/null ||: 
  mv -v $PKG_DIR/../*.buildinfo \
    $PKG_DIR/../*.changes \
    $PKG_DIR/../*.deb \
    $PKG_DIR/../*.ddeb \
    ${OUT_DIR}/
  chown -v -R ${OWNER}:${OWNER} ${OUT_DIR}
  popd
}

makeSourcePackage()
{
  pushd $PROJECT_DIR
  
  dpkg-buildpackage -us -uc -ui -i=* -I --build=source
  
  mkdir -p ${OUT_DIR}/source 2>/dev/null ||: 
  mv -v $PKG_DIR/../*.buildinfo \
  $PKG_DIR/../*.changes \
  $PKG_DIR/../*.tar.gz \
  $PKG_DIR/../*.dsc \
  ${OUT_DIR}/source/
  chown -v -R ${OWNER}:${OWNER} ${OUT_DIR}
  popd
}

makeTests()
{
  pushd $PKG_DIR
  dir=build-$(date "+%y%m%d_%H%M%S")
  mkdir $dir && cd $dir
  cmake -DCMAKE_BUILD_TYPE=Debug \
    -DOB_BUILD_TESTS=ON \
    ..
  cmake --build . -- -j4
  #TODO: use ctest, try running bats
  bin/ObSyncTest
  bin/TaskListTest
  popd
}

main $@
