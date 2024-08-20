#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/8/18
# Brief  yaml-cpp install.

COLORRED="\033[31m"
COLORGREEN="\033[32m"
COLORYELLOW="\033[33m"
COLOREND="\033[0m"

function LOG() {
date_str=`date "+%Y-%m-%d %H:%M:%S"`
case "$1" in
  "WARNING")
    shift
    echo -e "${COLORYELLOW}WARNING `date "+%Y-%m-%d %H:%M:%S"` $* $COLOREND"
    ;;
  "ERROR")
    shift
    echo -e "${COLORRED}ERROR `date "+%Y-%m-%d %H:%M:%S"` $* $COLOREND"
    ;;
  *)
    echo -e "${COLORGREEN}INFO `date "+%Y-%m-%d %H:%M:%S"` $* $COLOREND"
esac
}

CURRENT_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-${CURRENT_DIR}/build}
BUILD_TYPE=${BUILD_TYPE:-RelWithDebInfo}
INSTALL_DIR=/usr/local

rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}

SOURCE_DIR=yaml-cpp-yaml-cpp-0.7.0

cmake \
  -H${CURRENT_DIR}/${SOURCE_DIR} -B${BUILD_DIR} \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DYAML_CPP_BUILD_TESTS=FALSE \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE && \
  cmake --build ${BUILD_DIR} -- -j && \
  cmake --build ${BUILD_DIR} --target install