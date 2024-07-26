#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/8/18
# Brief  Dependencies install.

COLORRED="\033[31m"
COLORGREEN="\033[32m"
COLORYELLOW="\033[33m"
COLOREND="\033[0m"

function LOG() {
  date_str=$(date "+%Y-%m-%d %H:%M:%S")
  case "$1" in
  "WARNING")
    shift
    echo -e "${COLORYELLOW}WARNING $(date "+%Y-%m-%d %H:%M:%S") $* $COLOREND"
    ;;
  "ERROR")
    shift
    echo -e "${COLORRED}ERROR $(date "+%Y-%m-%d %H:%M:%S") $* $COLOREND"
    ;;
  *)
    echo -e "${COLORGREEN}INFO $(date "+%Y-%m-%d %H:%M:%S") $* $COLOREND"
    ;;
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

function install_dep() {
  dep_name=$1
  LOG INFO "Installing ${dep_name}..."
  cd ./${dep_name}

  if [ -f ".install_flag" ]; then
    LOG INFO "${dep_name} has been installed, skip."
    cd ../
    return
  fi

  bash build.sh
  check_ret "Install ${dep_name}"

  touch .install_flag
  cd ../
}

if [ "$1" = "" -o "$2" = "" -o "$3" = "" ]; then
  LOG ERROR "Usage: bash install.sh cpp_enable py_enable java_enable"
  exit 1
fi
cpp_enable=$1
py_enable=$2
java_enable=$3

LOG INFO "Installing from apt..."
apt update
run_and_check apt install -yq make automake bison flex libtool pkg-config zip curl lsof build-essential
run_and_check apt install -yq libz-dev libevent-dev libssl-dev libunwind8-dev libc-ares-dev libleveldb-dev libsnappy-dev libapr1-dev libaprutil1-dev libdw-dev

install_dep openmpi
if [ "$cpp_enable" = "1" ]; then
  LOG INFO "Installing cmake..."
  if [ -z "$(command -v cmake)" ]; then
    rm -rf ./cmake-3.18.5-Linux-x86_64.sh
    wget https://github.com/Kitware/CMake/releases/download/v3.18.5/cmake-3.18.5-Linux-x86_64.sh
    run_and_check bash ./cmake-3.18.5-Linux-x86_64.sh --prefix=/usr/ --skip-license
    rm -rf ./cmake-3.18.5-Linux-x86_64.sh
  fi
  install_dep protobuf
  install_dep gflags
  install_dep gtest
  install_dep glog
  install_dep brpc
  install_dep eigen
  install_dep grpc
  install_dep yaml-cpp
  install_dep log4cxx
  install_dep rapidjson
  install_dep jemalloc
  install_dep boost
fi
if [ "$py_enable" = "1" ]; then
  LOG INFO "Installing python deps lib..."
  run_and_check pip3 install grpcio==1.46.0 grpcio-tools==1.46.0 requests mpi4py>=3.1.6
fi
if [ "$java_enable" = "1" ]; then
  LOG INFO "Installing java deps lib..."
  rm -rf protoc-gen-grpc-java-1.46.0-linux-x86_64.exe
  run_and_check wget https://repo1.maven.org/maven2/io/grpc/protoc-gen-grpc-java/1.46.0/protoc-gen-grpc-java-1.46.0-linux-x86_64.exe
  run_and_check mv protoc-gen-grpc-java-1.46.0-linux-x86_64.exe /usr/local/bin/protoc-gen-grpc-java
  run_and_check chmod a+x /usr/local/bin/protoc-gen-grpc-java
fi
