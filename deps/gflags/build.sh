#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/8/18
# Brief  gflags install.

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

SOURCE_ROOT=./gflags-2.2.2

rm -rf ./build
mkdir ./build

cmake -H${SOURCE_ROOT} -Bbuild -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_C_FLAGS="-fPIC" && \
    cmake --build build -- -j && \
    cmake --build build --target install