#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/8/18
# Brief  rapidjson install.

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
SOURCE_DIR="rapidjson-1.1.0"
INSTALL_DIR=/usr/local

rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}

cmake \
  -H${CURRENT_DIR}/${SOURCE_DIR} -B${BUILD_DIR} \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
  -DRAPIDJSON_BUILD_DOC=OFF \
  -DRAPIDJSON_BUILD_EXAMPLES=OFF \
  -DRAPIDJSON_BUILD_TESTS=OFF \
  -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF && \
  cmake --build ${BUILD_DIR} -- -j && \
  cmake --build ${BUILD_DIR} --target install