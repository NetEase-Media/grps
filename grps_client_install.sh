#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/8/25
# Brief  Grps install.
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

# check if is root.
if [ "$EUID" -ne 0 ]; then
  LOG ERROR "Please run as root."
  exit
fi

arg1=$1
if [ "$arg1" = "--skip_deps" ]; then
  LOG INFO "Skip install dependencies."
else
  LOG INFO "Installing dependencies..."
  cd deps
  run_and_check bash install.sh 1 1 1
  cd ..
  if [ $? -ne 0 ]; then
    LOG ERROR "Install dependencies failed."
    exit 1
  fi
  LOG INFO "Dependencies install success."
fi

LOG INFO "Building grps apis..."
cd ./apis/grps_apis/
run_and_check bash py_install.sh
run_and_check bash cpp_install.sh
run_and_check bash java_install.sh
cd -
LOG INFO "Build grps apis done."
