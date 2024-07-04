#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/10/04
# Brief  boost install.

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

rm -rf boost-1.83.0 boost-1.83.0.tar.gz
run_and_check wget https://github.com/boostorg/boost/releases/download/boost-1.83.0/boost-1.83.0.tar.gz
run_and_check tar -zxvf boost-1.83.0.tar.gz

cd boost-1.83.0
run_and_check ./bootstrap.sh --prefix=/usr/local
run_and_check ./b2 --prefix=/usr/local cxxflags=-fPIC cflags=-fPIC -a -q -j12 install

cd ..
rm -rf boost-1.83.0 boost-1.83.0.tar.gz