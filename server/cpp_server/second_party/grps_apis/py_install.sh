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
rm -rf python_gens/grps_apis/grps_pb2*
python3 -m grpc_tools.protoc -I./ --python_out=./python_gens/grps_apis --grpc_python_out=./python_gens/grps_apis grps.proto

# 2. build grps_apis pip dependency and install.
cd python_gens
rm -rf build dist grps_apis.egg-info
run_and_check python3 setup.py bdist_wheel
run_and_check pip3 uninstall grps_apis -y
run_and_check pip3 install dist/grps_apis-*-py3-none-any.whl
rm ./grps_apis-*-py3-none-any.whl
cp dist/grps_apis-*-py3-none-any.whl ./
rm -rf build dist grps_apis.egg-info