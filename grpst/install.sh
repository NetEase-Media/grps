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

# build grpst cmd.
LOG INFO "Building grpst..."
run_and_check pip install pyinstaller pyyaml
rm -rf dist build grpst.spec
install_cmd="pyinstaller -D grpst.py -n grpst"
if [ $py_enable -eq 1 ]; then
  install_cmd="$install_cmd --add-binary ./releases/py_server.tar:releases"
  install_cmd="$install_cmd --add-binary ./releases/py_template.tar:releases"
fi
if [ $cpp_enable -eq 1 ]; then
  install_cmd="$install_cmd --add-binary ./releases/cpp_server.tar:releases"
  install_cmd="$install_cmd --add-binary ./releases/cpp_template.tar:releases"
fi
if [ $cpp_enable -eq 1 -a $cpp_torch_enable -eq 1 ]; then
  install_cmd="$install_cmd --add-binary ./releases/cpp_template_with_torch.tar:releases"
fi
if [ $cpp_enable -eq 1 -a $cpp_tf_enable -eq 1 ]; then
  install_cmd="$install_cmd --add-binary ./releases/cpp_template_with_tf.tar:releases"
fi
if [ $cpp_enable -eq 1 -a $cpp_trt_enable -eq 1 ]; then
  install_cmd="$install_cmd --add-binary ./releases/cpp_template_with_trt.tar:releases"
fi
run_and_check $install_cmd

# build default project.
cd ./dist/grpst
if [ $cpp_enable -eq 1 -a $cpp_torch_enable -eq 1 ]; then
  run_and_check ./grpst create --project_type cpp_torch ./torch_serve_project
  run_and_check ./grpst archive --skip_unittest --output_path ./torch_serve_project.mar ./torch_serve_project
  rm -rf ./torch_serve_project
fi
if [ $cpp_enable -eq 1 -a $cpp_tf_enable -eq 1 ]; then
  run_and_check ./grpst create --project_type cpp_tf ./tf_serve_project
  run_and_check ./grpst archive --skip_unittest --output_path ./tf_serve_project.mar ./tf_serve_project
  rm -rf ./tf_serve_project
fi
if [ $cpp_enable -eq 1 -a $cpp_trt_enable -eq 1 ]; then
  run_and_check ./grpst create --project_type cpp_trt ./trt_serve_project
  run_and_check ./grpst archive --skip_unittest --output_path ./trt_serve_project.mar ./trt_serve_project
  rm -rf ./trt_serve_project
fi
# copy completion script.
cp ../../grpst-completion-bash.sh ./grpst-completion-bash.sh
cp ../../grpst-completion-fish.sh ./grpst-completion-fish.sh
LOG INFO "Build grpst done."

# install
LOG INFO "Installing grpst..."
cd ../
GRPST_HOME=/usr/local/grpst
rm -rf $GRPST_HOME
cp -r ./grpst $GRPST_HOME
if grep -q "export GRPST_HOME=/usr/local/grpst" /etc/bash.bashrc; then
  LOG INFO "Home already set."
else
  echo "export GRPST_HOME=/usr/local/grpst" >>/etc/bash.bashrc
fi

if grep -q "export PATH=\$PATH:\$GRPST_HOME" /etc/bash.bashrc; then
  LOG INFO "Path already set."
else
  echo "export PATH=\$PATH:\$GRPST_HOME" >>/etc/bash.bashrc
fi

if grep -q "source \$GRPST_HOME/grpst-completion-bash.sh" /etc/bash.bashrc; then
  LOG INFO "Completion already set."
else
  echo "source \$GRPST_HOME/grpst-completion-bash.sh" >>/etc/bash.bashrc
fi

if [ -e /etc/fish/config.fish ]; then
  if grep -q "export GRPST_HOME=/usr/local/grpst" /etc/fish/config.fish; then
    LOG INFO "Home already set."
  else
    echo "export GRPST_HOME=/usr/local/grpst" >>/etc/fish/config.fish
  fi
  if grep -q "source \$GRPST_HOME/grpst-completion-fish.sh" /etc/fish/config.fish; then
    LOG INFO "Completion already set."
  else
    echo "source \$GRPST_HOME/grpst-completion-fish.sh" >>/etc/fish/config.fish
  fi
fi

# clean
cd ../
rm -rf dist build grpst.spec

LOG INFO "Install grpst done, use \"source /etc/bash.bashrc\" to enable env, use \"grpst -h\" to get help."
