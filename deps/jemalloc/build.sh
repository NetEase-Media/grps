#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/8/18
# Brief  jemalloc install.

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

cd ./jemalloc-5.3.0 && \
    bash autogen.sh && \
    CC=/usr/bin/gcc CXX=/usr/bin/g++ ./configure --prefix=/usr/local CXXFLAGS=-fPIC CFLAGS=-fPIC && \
    make -j && \
    make install && \
    make clean && \
    cd -