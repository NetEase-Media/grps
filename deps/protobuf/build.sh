#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2024/7/23
# Brief  protobuf install.

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

# check proto if exists
if [ -n "$(command -v protoc)" ]; then
  LOG INFO "protobuf has been installed, skip."
  exit 0
fi

cur_pwd=$(pwd)

# download
cd /root/
rm -rf protobuf-3.9.2 v3.9.2.tar.gz
run_and_check wget https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.9.2.tar.gz
run_and_check tar -xvf v3.9.2.tar.gz

# build
cd protobuf-3.9.2
run_and_check ./autogen.sh && \
  ./configure --prefix=/usr/local CXXFLAGS=-fPIC CFLAGS=-fPIC && \
  autoreconf -f -i && \
  make clean && \
  make _GLIBCXX_USE_CXX11_ABI=1 -j12 && \
  make install && \
  make clean

# clean
cd ../
rm -rf protobuf-3.9.2 v3.9.2.tar.gz

cd $cur_pwd