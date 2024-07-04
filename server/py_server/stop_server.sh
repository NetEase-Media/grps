#!/bin/bash
COLORBLACK="\033[30m"
COLORRED="\033[31m"
COLORGREEN="\033[32m"
COLORBLUE="\033[34m"
COLORYELLOW="\033[33m"
COLORPURPLE="\033[35m"
COLORSKYBLUE="\033[36m"
COLORWHITE="\033[37m"
COLOREND="\033[0m"

if [ -e "PID" ]; then
  pids=$(cat PID)
  for pid in $pids; do
    kill -9 $pid
  done
  rm -f PID
  echo -e "${COLORGREEN}[STOP SUCCESS]${COLOREND}"
else
  echo -e "${COLORPURPLE}[NOT RUNNING]${COLOREND}"
fi
