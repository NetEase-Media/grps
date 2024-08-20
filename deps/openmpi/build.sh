#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2024/7/22
# Brief  openapi install.

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

# check mpi if exists
if [ -n "$(command -v mpirun)" ]; then
  LOG WARNING "mpi has been installed, skip."
  exit 0
fi

# download
rm -rf openmpi-5.0.4.tar.gz openmpi-5.0.4
run_and_check wget https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.4.tar.gz
run_and_check tar -zxvf openmpi-5.0.4.tar.gz

# build
cd openmpi-5.0.4
run_and_check ./configure --prefix=/usr/local CXXFLAGS=-fPIC CFLAGS=-fPIC
run_and_check make -j12
run_and_check make install

# clean
cd ../
rm -rf openmpi-5.0.4.tar.gz openmpi-5.0.4