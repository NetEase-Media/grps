#!/bin/bash
# Copyright 2023 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/8/18
# Brief  Environment install.
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

cuda_enable=                                  # If setup cuda.
py_enable=                                    # If setup python support.
cpp_enable=                                   # If setup cpp support.
cpp_torch_enable=                             # If setup cpp torch support.
cpp_tf_enable=                                # If setup cpp tensorflow support.
cpp_trt_enable=                               # If setup cpp tensorrt support.

if [ "$cuda_enable" = "" ]; then
  echo "Select if setup CUDA, yes or no, default is yes?"
  read cuda_enable
fi
if [ "$cuda_enable" = "yes" -o "$cuda_enable" = "y" -o "$cuda_enable" = "" ]; then
  cuda_path="/usr/local/cuda"
  if [ ! -d "$cuda_path" ]; then
    LOG ERROR "CUDA path $cuda_path not exist."
    exit 1
  fi
  LOG INFO "CUDA is setup, path is $cuda_path."
  cuda_enable="1"
else
  LOG INFO "CUDA is not setup."
  cuda_enable="0"
  cuda_path=""
fi

if [ "$py_enable" = "" ]; then
  echo "Select if setup python support, yes or no, default is yes?"
  read py_enable
fi
if [ "$py_enable" = "yes" -o "$py_enable" = "y" -o "$py_enable" = "" ]; then
  LOG INFO "python support is setup."
  py_enable="1"
else
  LOG INFO "python support is not setup."
  py_enable="0"
fi

if [ "$cpp_enable" = "" ]; then
  echo "Select if setup cpp support, yes or no, default is yes?"
  read cpp_enable
fi
if [ "$cpp_enable" = "yes" -o "$cpp_enable" = "y" -o "$cpp_enable" = "" ]; then
  LOG INFO "cpp support is setup."
  cpp_enable="1"
else
  LOG INFO "cpp support is not setup."
  cpp_enable="0"
fi

if [ "$cpp_enable" = "1" ]; then
  if [ "$cpp_torch_enable" = "" ]; then
    echo "Select if setup cpp torch support, yes or no, default is yes?"
    read cpp_torch_enable
  fi
  if [ "$cpp_torch_enable" = "yes" -o "$cpp_torch_enable" = "y" -o "$cpp_torch_enable" = "" ]; then
    libtorch_path="/usr/local/libtorch"
    if [ ! -d "$libtorch_path" ]; then
      LOG ERROR "libtorch path $libtorch_path not exist."
      exit 1
    fi
    if [ ! -d "$libtorch_path/lib" ]; then
      LOG ERROR "libtorch path $libtorch_path/lib not exist."
      exit 1
    fi
    LOG INFO "cpp torch is setup, lib path is $libtorch_path."
    cpp_torch_enable="1"
  else
    LOG INFO "cpp torch is not setup."
    cpp_torch_enable="0"
    libtorch_path=""
  fi

  if [ "$cpp_tf_enable" = "" ]; then
    echo "Select if setup cpp tensorflow support, yes or no, default is yes?"
    read cpp_tf_enable
  fi
  if [ "$cpp_tf_enable" = "yes" -o "$cpp_tf_enable" = "y" -o "$cpp_tf_enable" = "" ]; then
    libtensorflow_path="/usr/local/libtensorflow"
    if [ ! -d "$libtensorflow_path" ]; then
      LOG ERROR "libtensorflow path $libtensorflow_path not exist."
      exit 1
    fi
    if [ ! -d "$libtensorflow_path/lib" ]; then
      LOG ERROR "libtensorflow path $libtensorflow_path/lib not exist."
      exit 1
    fi
    LOG INFO "cpp tensorflow is setup, lib path is $libtensorflow_path."
    cpp_tf_enable="1"
  else
    LOG INFO "cpp tensorflow is not setup."
    cpp_tf_enable="0"
    libtensorflow_path=""
  fi

  if [ "$cpp_trt_enable" = "" ]; then
    echo "Select if setup cpp tensorrt support, yes or no, default is yes?"
    read cpp_trt_enable
  fi
  if [ "$cpp_trt_enable" = "yes" -o "$cpp_trt_enable" = "y" -o "$cpp_trt_enable" = "" ]; then
    libtensorrt_path="/usr/local/libtensorrt"
    if [ ! -d "$libtensorrt_path" ]; then
      LOG ERROR "libtensorrt path $libtensorrt_path not exist."
      exit 1
    fi
    if [ ! -d "$libtensorrt_path/lib" ]; then
      LOG ERROR "libtensorrt path $libtensorrt_path/lib not exist."
      exit 1
    fi
    LOG INFO "cpp tensorrt is setup, lib path is $libtensorrt_path."
    cpp_trt_enable="1"
  else
    LOG INFO "cpp tensorrt is not setup."
    cpp_trt_enable="0"
    libtensorrt_path=""
  fi
else
  cpp_torch_enable="0"
  libtorch_path=""
  cpp_tf_enable="0"
  libtensorflow_path=""
  cpp_trt_enable="0"
  libtensorrt_path=""
fi

LOG INFO "cuda_enable=$cuda_enable py_enable=$py_enable cpp_enable=$cpp_enable cpp_torch_enable=$cpp_torch_enable \
libtorch_path=$libtorch_path cpp_tf_enable=$cpp_tf_enable libtensorflow_path=$libtensorflow_path \
cpp_trt_enable=$cpp_trt_enable libtensorrt_path=$libtensorrt_path"

echo "cuda_enable=$cuda_enable" >.config
echo "py_enable=$py_enable" >>.config
echo "cpp_enable=$cpp_enable" >>.config
echo "cpp_torch_enable=$cpp_torch_enable" >>.config
echo "libtorch_path=$libtorch_path" >>.config
echo "cpp_tf_enable=$cpp_tf_enable" >>.config
echo "libtensorflow_path=$libtensorflow_path" >>.config
echo "cpp_trt_enable=$cpp_trt_enable" >>.config
echo "libtensorrt_path=$libtensorrt_path" >>.config