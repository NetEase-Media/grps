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

# 0. read config.
cuda_enable=0
py_enable=0
cpp_enable=0
cpp_torch_enable=0
libtorch_path=""
cpp_tf_enable=0
libtensorflow_path=""
cpp_trt_enable=0
libtensorrt_path=""
if [ -f .config ]; then
  LOG INFO "read config from .config file"
  while read line
  do
    if [[ $line =~ ^cuda_enable= ]]; then
      cuda_enable=${line#*=}
      if [ "$cuda_enable"x != "1"x ]; then
        cuda_enable=0
      fi
    elif [[ $line =~ ^py_enable= ]]; then
      py_enable=${line#*=}
      if [ "$py_enable"x != "1"x ]; then
        py_enable=0
      fi
    elif [[ $line =~ ^cpp_enable= ]]; then
      cpp_enable=${line#*=}
      if [ "$cpp_enable"x != "1"x ]; then
        cpp_enable=0
      fi
    elif [[ $line =~ ^libtorch_path= ]]; then
      libtorch_path=${line#*=}
    elif [[ $line =~ ^libtensorflow_path= ]]; then
      libtensorflow_path=${line#*=}
    elif [[ $line =~ ^libtensorrt_path= ]]; then
      libtensorrt_path=${line#*=}
    elif [[ $line =~ ^cpp_tf_enable= ]]; then
      cpp_tf_enable=${line#*=}
      if [ "$cpp_tf_enable"x != "1"x ]; then
        cpp_tf_enable=0
      fi
    elif [[ $line =~ ^cpp_torch_enable= ]]; then
      cpp_torch_enable=${line#*=}
      if [ "$cpp_torch_enable"x != "1"x ]; then
        cpp_torch_enable=0
      fi
    elif [[ $line =~ ^cpp_trt_enable= ]]; then
      cpp_trt_enable=${line#*=}
      if [ "$cpp_trt_enable"x != "1"x ]; then
        cpp_trt_enable=0
      fi
    fi
  done < .config
else
  LOG ERROR "Not found .config file, please source grps_env.sh first."
  exit 1
fi
LOG INFO "cuda_enable=$cuda_enable py_enable=$py_enable cpp_enable=$cpp_enable \
cpp_torch_enable=$cpp_torch_enable libtorch_path=$libtorch_path \
cpp_tf_enable=$cpp_tf_enable libtensorflow_path=$libtensorflow_path \
cpp_trt_enable=$cpp_trt_enable libtensorrt_path=$libtensorrt_path"

# 1. install deps.
arg1=$1
if [ "$arg1" = "--skip_deps" ]; then
  LOG INFO "Skip install dependencies."
else
  LOG INFO "Installing dependencies..."
  cd deps
  run_and_check bash install.sh $cpp_enable 1 0
  cd ..
  if [ $? -ne 0 ]; then
    LOG ERROR "Install dependencies failed."
    exit 1
  fi
  LOG INFO "Dependencies install success."
fi

# 2. build grps apis.
LOG INFO "Building grps apis..."
cd ./apis/grps_apis/
run_and_check bash py_install.sh

if [ $cpp_enable -eq 1 ]; then
  run_and_check bash cpp_install.sh
fi
cd -
LOG INFO "Build grps apis done."

# 3. build cpp template with torch release if enable.
if [ $cpp_enable -eq 1 -a $cpp_torch_enable -eq 1 ]; then
  LOG INFO "Building cpp template with torch..."
  rm -rf ./cpp_template_with_torch
  cp -r ./template/cpp_template ./cpp_template_with_torch
  touch ./cpp_template_with_torch/.config
  echo "cuda_enable=$cuda_enable" >./cpp_template_with_torch/.config
  echo "torch_enable=1" >>./cpp_template_with_torch/.config
  echo "libtorch_path=$libtorch_path" >>./cpp_template_with_torch/.config
  cp ./cpp_template_with_torch/.config ./server/cpp_server/
  cd ./server/cpp_server
  run_and_check bash build.sh clean
  run_and_check bash build.sh grps-server-framework-install
  rm -rf ../../cpp_template_with_torch/second_party/grps-server-framework/include/* ../../cpp_template_with_torch/second_party/grps-server-framework/lib/*
  cp -r ./build/RelWithDebInfo_install/include/* ../../cpp_template_with_torch/second_party/grps-server-framework/include/
  cp ./build/RelWithDebInfo_install/lib/libgrps-server-framework.a ../../cpp_template_with_torch/second_party/grps-server-framework/lib/
  run_and_check bash build.sh clean
  cd ../../
  cd ./cpp_template_with_torch/
  run_and_check bash build.sh clean
  cd ../
  tar -cvf ./grpst/releases/cpp_template_with_torch.tar ./cpp_template_with_torch
  rm -rf ./cpp_template_with_torch
  LOG INFO "Build cpp template with torch done."
fi

# 4. build cpp template with tf release if enable.
if [ $cpp_enable -eq 1 -a $cpp_tf_enable -eq 1 ]; then
  LOG INFO "Building cpp template with tf..."
  rm -rf ./cpp_template_with_tf
  cp -r ./template/cpp_template ./cpp_template_with_tf
  touch ./cpp_template_with_tf/.config
  echo "cuda_enable=$cuda_enable" >./cpp_template_with_tf/.config
  echo "tf_enable=1" >>./cpp_template_with_tf/.config
  echo "libtensorflow_path=$libtensorflow_path" >>./cpp_template_with_tf/.config
  cp ./cpp_template_with_tf/.config ./server/cpp_server/
  cd ./server/cpp_server
  run_and_check bash build.sh clean
  run_and_check bash build.sh grps-server-framework-install
  rm -rf ../../cpp_template_with_tf/second_party/grps-server-framework/include/* ../../cpp_template_with_tf/second_party/grps-server-framework/lib/*
  cp -r ./build/RelWithDebInfo_install/include/* ../../cpp_template_with_tf/second_party/grps-server-framework/include/
  cp ./build/RelWithDebInfo_install/lib/libgrps-server-framework.a ../../cpp_template_with_tf/second_party/grps-server-framework/lib/
  run_and_check bash build.sh clean
  cd ../../
  cd ./cpp_template_with_tf/
  run_and_check bash build.sh clean
  cd ../
  tar -cvf ./grpst/releases/cpp_template_with_tf.tar ./cpp_template_with_tf
  rm -rf ./cpp_template_with_tf
  LOG INFO "Build cpp template with tf done."
fi

# 5. build cpp template with trt release if enable.
if [ $cpp_enable -eq 1 -a $cpp_trt_enable -eq 1 ]; then
  LOG INFO "Building cpp template with trt..."
  rm -rf ./cpp_template_with_trt
  cp -r ./template/cpp_template ./cpp_template_with_trt
  touch ./cpp_template_with_trt/.config
  echo "cuda_enable=$cuda_enable" >./cpp_template_with_trt/.config
  echo "trt_enable=1" >>./cpp_template_with_trt/.config
  echo "libtensorrt_path=$libtensorrt_path" >>./cpp_template_with_trt/.config
  cp ./cpp_template_with_trt/.config ./server/cpp_server/
  cd ./server/cpp_server
  run_and_check bash build.sh clean
  run_and_check bash build.sh grps-server-framework-install
  rm -rf ../../cpp_template_with_trt/second_party/grps-server-framework/include/* ../../cpp_template_with_trt/second_party/grps-server-framework/lib/*
  cp -r ./build/RelWithDebInfo_install/include/* ../../cpp_template_with_trt/second_party/grps-server-framework/include/
  cp ./build/RelWithDebInfo_install/lib/libgrps-server-framework.a ../../cpp_template_with_trt/second_party/grps-server-framework/lib/
  run_and_check bash build.sh clean
  cd ../../
  cd ./cpp_template_with_trt/
  run_and_check bash build.sh clean
  cd ../
  tar -cvf ./grpst/releases/cpp_template_with_trt.tar ./cpp_template_with_trt
  rm -rf ./cpp_template_with_trt
  LOG INFO "Build cpp template with trt done."
fi

# 6. build cpp template without torch, tf and trt release.
if [ $cpp_enable -eq 1 ]; then
  LOG INFO "Building cpp template without torch, tf and trt..."
  rm -rf ./cpp_template
  cp -r ./template/cpp_template ./cpp_template
  touch ./cpp_template/.config
  echo "cuda_enable=$cuda_enable" >./cpp_template/.config
  echo "torch_enable=0" >>./cpp_template/.config
  echo "tf_enable=0" >>./cpp_template/.config
  echo "trt_enable=0" >>./cpp_template/.config
  cp ./cpp_template/.config ./server/cpp_server/
  cd ./server/cpp_server
  run_and_check bash build.sh clean
  run_and_check bash build.sh grps-server-framework-install
  rm -rf ../../cpp_template/second_party/grps-server-framework/include/* ../../cpp_template/second_party/grps-server-framework/lib/*
  cp -r ./build/RelWithDebInfo_install/include/* ../../cpp_template/second_party/grps-server-framework/include/
  cp ./build/RelWithDebInfo_install/lib/libgrps-server-framework.a ../../cpp_template/second_party/grps-server-framework/lib/
  rm -rf ../../template/cpp_template/second_party/grps-server-framework/include/* ../../template/cpp_template/second_party/grps-server-framework/lib/*
  cp -r ./build/RelWithDebInfo_install/include/* ../../template/cpp_template/second_party/grps-server-framework/include/
  cp ./build/RelWithDebInfo_install/lib/libgrps-server-framework.a ../../template/cpp_template/second_party/grps-server-framework/lib/
  cp ./.config ../../template/cpp_template/
  run_and_check bash build.sh clean
  cd ../../
  cd ./cpp_template/
  run_and_check bash build.sh clean
  cd ../
  tar -cvf ./grpst/releases/cpp_template.tar ./cpp_template
  rm -rf ./cpp_template
  LOG INFO "Build cpp template without torch and tf done."
fi

# 7. build py template release.
if [ $py_enable -eq 1 ]; then
  LOG INFO "Building py template..."
  cp -r ./server/py_server ./py_server
  rm -rf ./py_server/grps_framework/apis/*
  cp -r ./apis/grps_apis/python_gens/grps_apis/*.py ./py_server/grps_framework/apis/
  rm -rf ./py_server/grps_framework/apis/setup.py
  rm -rf ./server/py_server/grps_framework/apis/*
  cp -r ./apis/grps_apis/python_gens/grps_apis/*.py ./server/py_server/grps_framework/apis/
  rm -rf ./server/py_server/grps_framework/apis/setup.py
  cd ./py_server
  rm -rf ./dist
  run_and_check python3 setup.py sdist bdist_wheel
  run_and_check pip3 install ./dist/grps_framework-*-py3-none-any.whl --force-reinstall
  cd ../
  rm ./template/py_template/grps_framework-*-py3-none-any.whl
  cp ./py_server/dist/grps_framework-*-py3-none-any.whl ./template/py_template
  rm -rf ./py_template
  cp -r ./template/py_template ./py_template
  tar -cvf ./grpst/releases/py_template.tar ./py_template
  rm -rf ./py_server
  rm -rf ./py_template
  LOG INFO "Build py template done."
fi

# 8. build cpp server release.
if [ $cpp_enable -eq 1 ]; then
  LOG INFO "Building cpp server..."
  cp -r ./server/cpp_server ./cpp_server
  rm -rf ./cpp_server/second_party/grps_apis
  cp -r ./apis/grps_apis ./cpp_server/second_party/
  tar -cvf ./grpst/releases/cpp_server.tar ./cpp_server
  rm -rf ./cpp_server
  LOG INFO "Build cpp server done."
fi

# 9. build py server release.
if [ $py_enable -eq 1 ]; then
  LOG INFO "Building py server..."
  cp -r ./server/py_server ./py_server
  run_and_check pip3 install -r ./py_server/requirements.txt
  tar -cvf ./grpst/releases/py_server.tar ./py_server
  rm -rf ./py_server
  LOG INFO "Build py server done."
fi

# 10. build and install grpst.
LOG INFO "Building and installing grpst..."
cp .config ./grpst/
cd ./grpst
run_and_check bash install.sh
cd ../
