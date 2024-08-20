#!/bin/bash
# @copyright 2022 netease. All rights reserved.
# @author zhaochaochao@corp.netease.com
# @date   2023-02-06
# @brief  build project
#

COLORBLACK="\033[30m"
COLORRED="\033[31m"
COLORGREEN="\033[32m"
COLORBLUE="\033[34m"
COLORYELLOW="\033[33m"
COLORPURPLE="\033[35m"
COLORSKYBLUE="\033[36m"
COLORWHITE="\033[37m"
COLOREND="\033[0m"

function LOG() {
  date_str=$(date "+%Y-%m-%d %H:%M:%S")
  case "$1" in
  "WARNING")
    shift
    echo -e "${COLORSKYBLUE}WARNING $(date "+%Y-%m-%d %H:%M:%S") $* $COLOREND"
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

export PATH=/usr/local/bin:$PATH

CURRENT_DIR=$(pwd)
OUTPUT=$CURRENT_DIR/output
BUILD_DIR=${BUILD_DIR:-${CURRENT_DIR}/build}
BUILD_TYPE=${BUILD_TYPE:-RelWithDebInfo}
#BUILD_TYPE=${BUILD_TYPE:-Debug}
INSTALL_DIR=${INSTALL_DIR:-${BUILD_DIR}/${BUILD_TYPE}_install}

if [ "$1"x = "clean"x ]; then
  LOG WARNING "rm -rf ${BUILD_DIR}"
  rm -rf ${BUILD_DIR}
  exit 0
fi

mkdir -p ${BUILD_DIR}

cuda_enable=0
libtorch_path=""
libtensorflow_path=""
libtensorrt_path=""
tf_enable=0
torch_enable=0
trt_enable=0
if [ -f .config ]; then
  LOG INFO "read config from .config file"
  while read line
  do
    if [[ $line =~ ^cuda_enable= ]]; then
      cuda_enable=${line#*=}
      if [ "$cuda_enable"x != "1"x ]; then
        cuda_enable=0
      fi
    elif [[ $line =~ ^libtorch_path= ]]; then
      libtorch_path=${line#*=}
    elif [[ $line =~ ^libtensorflow_path= ]]; then
      libtensorflow_path=${line#*=}
    elif [[ $line =~ ^libtensorrt_path= ]]; then
      libtensorrt_path=${line#*=}
    elif [[ $line =~ ^tf_enable= ]]; then
      tf_enable=${line#*=}
      if [ "$tf_enable"x != "1"x ]; then
        tf_enable=0
      fi
    elif [[ $line =~ ^torch_enable= ]]; then
      torch_enable=${line#*=}
      if [ "$torch_enable"x != "1"x ]; then
        torch_enable=0
      fi
    elif [[ $line =~ ^trt_enable= ]]; then
      trt_enable=${line#*=}
      if [ "$trt_enable"x != "1"x ]; then
        trt_enable=0
      fi
    fi
  done < .config
else
  LOG WARNING "no .config file, use default config"
fi
LOG INFO "cuda_enable=$cuda_enable torch_enable=$torch_enable tf_enable=$tf_enable trt_enable=$trt_enable \
libtorch_path=$libtorch_path libtensorflow_path=$libtensorflow_path libtensorrt_path=$libtensorrt_path"

cmake_args="-H${CURRENT_DIR} -B${BUILD_DIR} \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DCUDA_ENABLE=${cuda_enable} \
  -DTORCH_ENABLE=${torch_enable} \
  -DTF_ENABLE=${tf_enable} \
  -DTRT_ENABLE=${trt_enable}"

if [ "$torch_enable"x = "1"x ]; then
  cmake_args="${cmake_args} -DLIBTORCH_PATH=${libtorch_path}"
fi
if [ "$tf_enable"x = "1"x ]; then
  cmake_args="${cmake_args} -DLIBTENSORFLOW_PATH=${libtensorflow_path}"
fi
if [ "$trt_enable"x = "1"x ]; then
  cmake_args="${cmake_args} -DLIBTENSORRT_PATH=${libtensorrt_path}"
fi

cmake ${cmake_args}
if [ $? -ne 0 ]; then
  LOG WARNING "cmake failed."
  exit 1
fi

if [ "$1"x != ""x ]; then
  cmake --build ${BUILD_DIR} --target $1 -j8
else
  cmake --build ${BUILD_DIR} -- -j8
  cmake --build ${BUILD_DIR} --target install
fi


if [ $? -ne 0 ]; then
  LOG WARNING "build with cmake failed."
  exit 1
fi