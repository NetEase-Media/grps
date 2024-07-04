#!/bin/bash
# build project

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

# Build cpp client.
CURRENT_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-${CURRENT_DIR}/build}
BUILD_TYPE=${BUILD_TYPE:-RelWithDebInfo}
INSTALL_DIR=${INSTALL_DIR:-${BUILD_DIR}/${BUILD_TYPE}_install}

if [ "$1"x = "clean"x ]; then
  LOG WARNING "rm -rf ${BUILD_DIR}"
  rm -rf ${BUILD_DIR}
  exit 0
fi

mkdir -p ${BUILD_DIR}

cmake -H${CURRENT_DIR} -B${BUILD_DIR} \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
cmake --build ${BUILD_DIR} -- -j8
cmake --build ${BUILD_DIR} --target install

if [ $? -ne 0 ]; then
  LOG WARNING "build with cmake failed."
  exit 1
fi
