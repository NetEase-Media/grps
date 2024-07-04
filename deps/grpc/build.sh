#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/9/28
# Brief  grpc install.

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

function check_ret() {
  if [ $? -ne 0 ]; then
    LOG ERROR "$1 failed."
    exit 1
  fi
}

function run_and_check() {
  $*
  check_ret "$*"
}

rm -rf grpc
run_and_check git clone --recurse-submodules -b v1.25.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc

# patch apply
cp benchmark.diff grpc/third_party/benchmark
cd grpc/third_party/benchmark
run_and_check git apply benchmark.diff

# build
cd ../../
mkdir cmake/build
cd cmake/build
run_and_check cmake ../.. -DgRPC_PROTOBUF_PROVIDER=package -DgRPC_SSL_PROVIDER=package -DgRPC_ZLIB_PROVIDER=package -DgRPC_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=ON -DgRPC_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/usr/local/
run_and_check make -j12
run_and_check make install
cp grpc_cpp_plugin /usr/local/bin/
cp lib*.so /usr/local/lib/

# clean
cd ../../../
rm -rf grpc