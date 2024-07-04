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
rm -rf java_gens
mkdir java_gens
protoc -I./ --java_out=./java_gens --grpc_out=./java_gens --plugin=protoc-gen-grpc=`which protoc-gen-grpc-java` grps.proto

# 2. mvn install grps-api-{version}.jar
run_and_check mvn install
run_and_check mvn dependency:copy-dependencies -DoutputDirectory=./maven-lib
rm -rf ./maven-lib