# docker镜像构建

## 打包源码

```bash
# 执行如下脚本，产生打包文件
bash archive.sh
```

## 构建grps

### tensorrt-llm

#### Dockerfile.grps1.1.0_cuda12.4_cudnn9.1_trtllm0.11.0_py3.10

grps1.1.0 + cuda12.4 + cudnn9.1 + trtllm0.11.0 + python3.10

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda12.4_cudnn9.1_trtllm0.11.0_py3.10 -f Dockerfile.grps1.1.0_cuda12.4_cudnn9.1_trtllm0.11.0_py3.10 .
```

#### Dockerfile.grps1.1.0_cuda12.4_cudnn8.9_trtllm0.10.0_py3.10

grps1.1.0 + cuda12.4 + cudnn8.9 + trtllm0.10.0 + python3.10

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda12.4_cudnn8.9_trtllm0.10.0_py3.10 -f Dockerfile.grps1.1.0_cuda12.4_cudnn8.9_trtllm0.10.0_py3.10 .
```

### vllm

#### Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_vllm0.5.5_py3.10

grps1.1.0 + cuda11.8 + cudnn8.6 + vllm0.5.5 + python3.10

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_vllm0.5.5_py3.10 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_vllm0.5.5_py3.10 .
```

#### Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_vllm0.4.3_py3.10

grps1.1.0 + cuda11.8 + cudnn8.6 + vllm0.4.3 + python3.10

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_vllm0.4.3_py3.10 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_vllm0.4.3_py3.10 .
```

### tensorrt

#### Dockerfile.grps1.1.0_cuda12.4_cudnn9.1_trt8.6.3_py3.10

grps1.1.0 + cuda12.4 + cudnn9.1 + trt8.6.3 + python3.10

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda12.4_cudnn9.1_trt8.6.3_py3.10 -f Dockerfile.grps1.1.0_cuda12.4_cudnn9.1_trt8.6.3_py3.10 .
```

#### Dockerfile.grps1.1.0_cuda12.1_cudnn8.9_trt8.6.1_py3.8

grps1.1.0 + cuda12.1 + cudnn8.9 + trt8.6.1 + python3.8

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda12.1_cudnn8.9_trt8.6.1_py3.8 -f Dockerfile.grps1.1.0_cuda12.1_cudnn8.9_trt8.6.1_py3.8 .
```

#### Dockerfile.grps1.1.0_cuda11.3_cudnn8.2_trt7.2.3_py3.8

grps1.1.0 + cuda11.3 + cudnn8.2 + trt7.2.3 + python3.8

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.3_cudnn8.2_trt7.2.3_py3.8 -f Dockerfile.grps1.1.0_cuda11.3_cudnn8.2_trt7.2.3_py3.8 .
```

### torch

#### Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_torch2.4.0_py3.10

grps1.1.0 + cuda11.8 + cudnn8.6 + torch2.4.0 + python3.10

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
# --build-arg PY_ENABLE=0或1参数用于设置是否开启python支持
# --build-arg CPP_ENABLE=0或1参数用于设置是否开启c++支持，注意关闭后grpst不支持torch_serve命令
# 可以按需构建仅含部分支持的版本（默认所有参数都为1，即打开状态），例如：
# 构建完整版，包含python、c++、torch支持版本，完整版构建时间较长
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_torch2.4.0_py3.10 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_torch2.4.0_py3.10 .
# 构建torch + python支持版本
docker build --build-arg PY_ENABLE=1 \
  --build-arg CPP_ENABLE=0 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_torch2.4.0_py3.10 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_torch2.4.0_py3.10 .
# 构建torch + c++支持版本，支持grpst torch_serve命令
docker build --build-arg PY_ENABLE=0 \
  --build-arg CPP_ENABLE=1 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_torch2.4.0_cpp -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_torch2.4.0_py3.10 .
```

### tensorflow + torch

#### Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.10

grps1.1.0 + cuda11.8 + cudnn8.6 + tf2.12.0 + torch2.1.2 + python3.10

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
# --build-arg PY_ENABLE=0或1参数用于设置是否开启python支持
# --build-arg CPP_ENABLE=0或1参数用于设置是否开启c++支持，注意关闭后grpst不支持torch_serve和tf_serve命令
# --build-arg TORCH_ENABLE=0或1参数用于设置是否开启torch支持
# --build-arg TF_ENABLE=0或1参数用于设置是否开启tensorflow支持
# 可以按需构建仅含部分支持的版本（默认所有参数都为1，即打开状态），例如：
# 构建完整版，包含python、c++、torch、tensorflow支持版本，完整版构建时间较长
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.10 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.10 .
# 构建torch + python支持版本
docker build --build-arg PY_ENABLE=1 \
  --build-arg CPP_ENABLE=0 \
  --build-arg TORCH_ENABLE=1 \
  --build-arg TF_ENABLE=0 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_torch2.1.2_py3.10 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.10 .
# 构建torch + c++支持版本，支持grpst torch_serve命令
docker build --build-arg PY_ENABLE=0 \
  --build-arg CPP_ENABLE=1 \
  --build-arg TORCH_ENABLE=1 \
  --build-arg TF_ENABLE=0 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_torch2.1.2_cpp -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.10 .
# 构建tf + python支持版本
docker build --build-arg PY_ENABLE=1 \
  --build-arg CPP_ENABLE=0 \
  --build-arg TORCH_ENABLE=0 \
  --build-arg TF_ENABLE=1 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_py3.10 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.10 .
# 构建tf + c++支持版本，支持grpst tf_serve命令
docker build --build-arg PY_ENABLE=0 \
  --build-arg CPP_ENABLE=1 \
  --build-arg TORCH_ENABLE=0 \
  --build-arg TF_ENABLE=1 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_cpp -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.10 .
```

#### Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.8

grps1.1.0 + cuda11.8 + cudnn8.6 + tf2.12.0 + torch2.1.2 + python3.8

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
# --build-arg PY_ENABLE=0或1参数用于设置是否开启python支持
# --build-arg CPP_ENABLE=0或1参数用于设置是否开启c++支持，注意关闭后grpst不支持torch_serve和tf_serve命令
# --build-arg TORCH_ENABLE=0或1参数用于设置是否开启torch支持
# --build-arg TF_ENABLE=0或1参数用于设置是否开启tensorflow支持
# 可以按需构建仅含部分支持的版本（默认所有参数都为1，即打开状态），例如：
# 构建完整版，包含python、c++、torch、tensorflow支持版本，完整版构建时间较长
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.8 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.8 .
# 构建torch + python支持版本
docker build --build-arg PY_ENABLE=1 \
  --build-arg CPP_ENABLE=0 \
  --build-arg TORCH_ENABLE=1 \
  --build-arg TF_ENABLE=0 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_torch2.1.2_py3.8 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.8 .
# 构建torch + c++支持版本，支持grpst torch_serve命令
docker build --build-arg PY_ENABLE=0 \
  --build-arg CPP_ENABLE=1 \
  --build-arg TORCH_ENABLE=1 \
  --build-arg TF_ENABLE=0 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_torch2.1.2_cpp -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.8 .
# 构建tf + python支持版本
docker build --build-arg PY_ENABLE=1 \
  --build-arg CPP_ENABLE=0 \
  --build-arg TORCH_ENABLE=0 \
  --build-arg TF_ENABLE=1 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_py3.8 -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.8 .
# 构建tf + c++支持版本，支持grpst tf_serve命令
docker build --build-arg PY_ENABLE=0 \
  --build-arg CPP_ENABLE=1 \
  --build-arg TORCH_ENABLE=0 \
  --build-arg TF_ENABLE=1 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_cpp -f Dockerfile.grps1.1.0_cuda11.8_cudnn8.6_tf2.12.0_torch2.1.2_py3.8 .
```

#### Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.8

grps1.1.0 + cuda10.1 + cudnn7.6.5 + tf2.3.0 + torch1.8.1 + python3.8

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
# --build-arg PY_ENABLE=0或1参数用于设置是否开启python支持
# --build-arg CPP_ENABLE=0或1参数用于设置是否开启c++支持，注意关闭后grpst不支持torch_serve和tf_serve命令
# --build-arg TORCH_ENABLE=0或1参数用于设置是否开启torch支持
# --build-arg TF_ENABLE=0或1参数用于设置是否开启tensorflow支持
# 可以按需构建仅含部分支持的版本（默认所有参数都为1，即打开状态），例如：
# 构建完整版，包含python、c++、torch、tensorflow支持版本，完整版构建时间较长
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.8 -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.8 .
# 构建torch + python支持版本
docker build --build-arg PY_ENABLE=1 \
  --build-arg CPP_ENABLE=0 \
  --build-arg TORCH_ENABLE=1 \
  --build-arg TF_ENABLE=0 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_torch1.8.1_py3.8 -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.8 .
# 构建torch + c++支持版本，支持grpst torch_serve命令
docker build --build-arg PY_ENABLE=0 \
  --build-arg CPP_ENABLE=1 \
  --build-arg TORCH_ENABLE=1 \
  --build-arg TF_ENABLE=0 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_torch1.8.1_cpp -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.8 .
# 构建tf + python支持版本
docker build --build-arg PY_ENABLE=1 \
  --build-arg CPP_ENABLE=0 \
  --build-arg TORCH_ENABLE=0 \
  --build-arg TF_ENABLE=1 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_py3.8 -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.8 .
# 构建tf + c++支持版本，支持grpst tf_serve命令
docker build --build-arg PY_ENABLE=0 \
  --build-arg CPP_ENABLE=1 \
  --build-arg TORCH_ENABLE=0 \
  --build-arg TF_ENABLE=1 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_cpp -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.8 .
```

#### Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7

grps1.1.0 + cuda10.1 + cudnn7.6.5 + tf2.3.0 + torch1.8.1 + python3.7

```bash
# 可以跟上如下参数
# --build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
# --build-arg PY_ENABLE=0或1参数用于设置是否开启python支持
# --build-arg CPP_ENABLE=0或1参数用于设置是否开启c++支持，注意关闭后grpst不支持torch_serve和tf_serve命令
# --build-arg TORCH_ENABLE=0或1参数用于设置是否开启torch支持
# --build-arg TF_ENABLE=0或1参数用于设置是否开启tensorflow支持
# 可以按需构建仅含部分支持的版本（默认所有参数都为1，即打开状态），例如：
# 构建完整版，包含python、c++、torch、tensorflow支持版本，完整版构建时间较长
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7 -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7 .
# 构建torch + python支持版本
docker build --build-arg PY_ENABLE=1 \
  --build-arg CPP_ENABLE=0 \
  --build-arg TORCH_ENABLE=1 \
  --build-arg TF_ENABLE=0 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_torch1.8.1_py3.7 -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7 .
# 构建torch + c++支持版本，支持grpst torch_serve命令
docker build --build-arg PY_ENABLE=0 \
  --build-arg CPP_ENABLE=1 \
  --build-arg TORCH_ENABLE=1 \
  --build-arg TF_ENABLE=0 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_torch1.8.1_cpp -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7 .
# 构建tf + python支持版本
docker build --build-arg PY_ENABLE=1 \
  --build-arg CPP_ENABLE=0 \
  --build-arg TORCH_ENABLE=0 \
  --build-arg TF_ENABLE=1 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_py3.7 -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7 .
# 构建tf + c++支持版本，支持grpst tf_serve命令
docker build --build-arg PY_ENABLE=0 \
  --build-arg CPP_ENABLE=1 \
  --build-arg TORCH_ENABLE=0 \
  --build-arg TF_ENABLE=1 \
  -t registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_cpp -f Dockerfile.grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7 .
```

## 构建grps client

#### Dockerfile.grps_client1.1.0

```bash
# 可以跟上--build-arg https_proxy=http://ip:port --build-arg http_proxy=http://ip:port参数用于设置代理
docker build -t registry.cn-hangzhou.aliyuncs.com/opengrps/client:1.1.0 -f Dockerfile.grps_client1.1.0 .
```
