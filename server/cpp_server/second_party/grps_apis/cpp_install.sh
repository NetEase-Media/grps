#!/bin/bash
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

# 1. generate proto source code.
rm -rf cpp_gens
mkdir cpp_gens
run_and_check protoc -I./ --cpp_out=./cpp_gens grps.proto --grpc_out=./cpp_gens --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`
run_and_check protoc -I./ --cpp_out=./cpp_gens grps.brpc.proto

# 2. build cpp grps_apis lib and install.
CURRENT_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-${CURRENT_DIR}/build}
BUILD_TYPE=${BUILD_TYPE:-RelWithDebInfo}
INSTALL_DIR=/usr/local/
rm -rf ${BUILD_DIR}
run_and_check cmake -H${CURRENT_DIR} -B${BUILD_DIR} \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
run_and_check cmake --build ${BUILD_DIR} -- -j
run_and_check cmake --build ${BUILD_DIR} --target install
rm -rf ${BUILD_DIR}