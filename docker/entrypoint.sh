#!/bin/bash

# env variables:
# OWNER - output package owner UID
# OUT_DIR - package output directory name

PROJECT_NAME=overboot
PROJECT_DIR=/usr/src/${PROJECT_NAME}

main()
{
  makeSourcePackage
  makeBinaryPackage
  makeTests
}

makeBinaryPackage()
{
  pushd $PROJECT_DIR
  dpkg-buildpackage -us -uc -ui -i -I --build=binary
  
  mkdir -p ${OUT_DIR} 2>/dev/null ||: 
  mv -v $PROJECT_DIR/../*.buildinfo \
    $PROJECT_DIR/../*.changes \
    $PROJECT_DIR/../*.deb \
    $PROJECT_DIR/../*.ddeb \
    ${OUT_DIR}/
  chown -v -R ${OWNER}:${OWNER} ${OUT_DIR}
  popd
}

makeSourcePackage()
{
  pushd $PROJECT_DIR
  
  # build orig tarball since dpkg-source will not do that
  # for quilt format:
  srcName=$(dpkg-parsechangelog -S Source)
  srcVersion=$(dpkg-parsechangelog -S Version | cut -d '-' -f 1)
  git config --global --add safe.directory $PWD
  git ls-files -z | xargs -0 \
    tar --transform "s/^/${srcName}\//" \
    -czvf ../${srcName}_${srcVersion}.orig.tar.gz

  dpkg-buildpackage -us -uc -ui -i=* -I --build=source
  
  mkdir -p ${OUT_DIR}/source 2>/dev/null ||: 
  mv -v $PROJECT_DIR/../*.buildinfo \
  $PROJECT_DIR/../*.changes \
  $PROJECT_DIR/../*.tar.gz \
  $PROJECT_DIR/../*.dsc \
  ${OUT_DIR}/source/
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
  bin/ObSyncTest
  bin/TaskListTest
  cd ..
  rm -r $dir
  popd
}

main $@
