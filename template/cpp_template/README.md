# grps c++ template

构建自己的grps c++自定义模型工程，实现自己的自定义模型。一个模型包含两部分：converter(前后处理转换器)、inferer(模型推理器)：

```text
                        |-------------------------------------------------------|
                        |                        Model                          |
                        |-------------------------------------------------------|
                        |                                                       |
                        |              |-----------|          |--------------|  |
input(GrpsMessage)      | preprocess   | Converter |          |    Inferer   |  |
   --------->           | -----------> |-----------|  infer   |--------------|  |
                        |              |           |  ------> |              |  |
output(GrpsMessage)     | postprocess  |           |  <------ |              |  |
   <---------           | <----------- |           |          |              |  |
                        |              |-----------|          |--------------|  |
                        |                                                       |
                        |-------------------------------------------------------|
```

主要包含下面几个章节：

1. [项目结构](#1-工程结构)
2. [自定义开发与调试](#2-自定义开发与调试)
3. [docker部署](#3-docker部署)
4. [客户端请求](#4-客户端请求)

## 1. 工程结构

```text
|-- client                              # 客户端样例
|-- conf                                # 配置文件
|   |-- inference.yml                   # 推理配置
|   |-- server.yml                      # 服务配置
|-- data                                # 数据文件
|-- docker                              # docker镜像构建
|-- second_party                        # 第二方依赖
|   |-- grps-server-framework           # grps框架依赖
|-- src                                 # 自定义源码
|   |-- customized_converter.cc/.h      # 自定义前后处理转换器
|   |-- customized_inferer.cc/.h        # 自定义推理器
|   |-- grps_server_customized.cc/.h    # 自定义库初始化
|   |-- main.cc                         # 本地单元测试
|-- third_party                         # 第三方依赖
|-- build.sh                            # 构建脚本
|-- CMakelists.txt                      # 工程构建文件
|-- .clang-format                       # 代码格式化配置文件
|-- .config                             # 工程配置文件，包含一些工程配置开关
```

## 2. 自定义开发与调试

### 2.1 创建容器并进入

从grps docker[镜像列表](https://github.com/NetEase-Media/grps/blob/master/docs/19_ImageList.md)
中选择所需的版本号镜像，选择gpu版需要注意对应的显卡驱动版本能否支持对应的cuda版本。
运行并进入docker容器内进行自定义开发与调试，创建的容器可重复使用。

```bash
# 用于保存自定义工程
mkdir grps_dev
cd grps_dev

# 创建容器
docker run -it --runtime=nvidia --rm -v $(pwd):/grps_dev -w /grps_dev registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7 bash
```

### 2.2 创建自定义工程

```bash
grpst create ./my_grps

# 选择自定义工程模板，如下提示，也可以直接使用--project_type参数指定。
Select project type.
[1] "py": python project with pytorch, tf and trt support.
[2] "cpp": c++ project without nn lib support.
[3] "cpp_torch": c++ project with libtorch support.
[4] "cpp_tf": c++ project with libtensorflow support.
[5] "cpp_trt": c++ project with libtensorrt support.
Please input number(1-5), default is "1":
2

cd my_grps
```

### 2.3 自定义

```bash
# 自定义前后处理转换器，内置前后处理器(torch、tensorflow)使用说明见docs/6_InternalConverter.md
customized_converter.cc/.h

# 自定义模型推理器，内置模型推理器(torch、 tensorflow)使用说明见docs/7_InternalInferer.md
customized_inferer.cc/.h

# 必要时可以新增第三方依赖到third_party目录，第三方so必须放到third_party/lib目录，这样构建时才会打包到mar中
third_party/include
third_party/lib

# 自定义cmake中##Build user customized lib##构建部分，必要时加入新增源码文件，链接第三方库等
src/CMakeLists.txt

# 必要时修改inference.yml配置适配新增的自定义前后处理转换器和模型推理器，也可以选择使用内置前后处理转换器和模型推理器
conf/inference.yml

# 必要时修改server.yml服务运行配置
conf/server.yml
```

### 2.4 本地单测与构建

```bash
# 补全单测输入和输出并进行单元测试，单测会自动加载inference.conf配置进行推理测试
src/main.cc

# 执行单测并打包构建mar（model archived）格式
grpst archive ./
```

### 2.5 本地部署

```bash
# 部署
# 如需修改端口、接口模式等服务参数，可以修改server.yml配置文件，跟上"--conf_path=./conf/server.yml"参数，可以跳过重新构建
# 部署成功后会自动跟踪日志，可以使用ctrl+c退出跟踪
# 访问http://host:http_port/可以查看服务指标，观测服务状态
grpst start ./server.mar

# 查看部署状态，可以看到端口（HTTP,RPC）、服务名、进程ID、部署路径
grpst ps
PORT(HTTP,RPC)      NAME                PID                 DEPLOY_PATH
7080,7081           my_grps             ***                 /root/.grps/my_grps
```

### 2.6 模拟请求

```bash
# curl命令请求，使用http端口
curl -X POST -H "Content-Type:application/json" -d '{"str_data": "hello grps"}' http://0.0.0.0:7080/grps/v1/infer/predict
# 带上"streaming=true" query-param，使用streaming模式请求
curl --no-buffer -X POST -H "Content-Type:application/json" -d '{"str_data": "hello grps"}' 'http://0.0.0.0:7080/grps/v1/infer/predict?streaming=true'

# http python client，使用http端口
python3 client/python/http_client.py 0.0.0.0:7080

# grpc python client，使用rpc端口
python3 client/python/grpc_client.py 0.0.0.0:7081

# grpc c++ client，使用rpc端口
cd client/cpp
bash build.sh clean
bash build.sh
./build/RelWithDebInfo_install/bin/grpc_client --server=0.0.0.0:7081

# brpc c++ client，使用rpc端口（需要将conf/server.yml framework改为"http+brpc"并使用新的配置重启服务）
# ./build/RelWithDebInfo_install/bin/brpc_client --server=0.0.0.0:7081

# 清理
bash build.sh clean
```

### 2.7 关闭

```bash
grpst stop my_grps

# 退出开发环境容器
exit
```

## 3. docker部署

```bash
cd my_grps

# 构建自定义工程docker镜像
# 注意可以修改Dockerfile中的基础镜像版本，选择自己所需的版本号，默认为grps_gpu:base镜像
# 启动命令grpst start可以跟上--timeout参数指定服务启动超时时间，默认为300s
docker build -t my_grps:1.0.0 -f docker/Dockerfile .

# 使用上面构建好的镜像启动docker容器
# 注意映射端口号
docker run -itd --runtime=nvidia --name="my_grps" -p 7080:7080 -p 7081:7081 my_grps:1.0.0

# 使用docker logs可以跟踪服务日志
docker logs -f my_grps
```

## 4. 客户端请求

客户端需要安装对应的依赖，目前支持c++、python、java客户端。python客户端环境最简单，安装对应[grps_apis pip依赖](https://github.com/NetEase-Media/grps/blob/master/apis/grps_apis/python_gens)
即可。c++和java客户端环境可以参考[从源码构建](https://github.com/NetEase-Media/grps/blob/master/docs/17_BuildFromSources.md)
构建对应的客户端环境。同时我们提供了一个支持python、c++、java客户端的docker环境，可以直接使用，见[grps client镜像](https://github.com/NetEase-Media/grps/blob/master/docs/19_ImageList.md#客户端镜像)。

### 4.1 python客户端

```bash
# http python client，使用http端口
pip3 install requests
python3 client/python/http_client.py 0.0.0.0:7080

# grpc python client，使用rpc端口
pip3 install grps_apis-*-py3-none-any.whl
python3 client/python/grpc_client.py 0.0.0.0:7081
```

### 4.2 c++客户端

```bash
# 这里使用已构建好的grps client容器环境，这里指定复用主机网络
docker run -it --rm -v $(pwd):/my_grps -w /my_grps --network=host opengrps/client:1.1.0 bash

# 构建client
cd client/cpp
bash build.sh clean
bash build.sh

# 运行grpc c++ client，使用rpc端口
./build/RelWithDebInfo_install/bin/grpc_client --server=0.0.0.0:7081

# brpc c++ client，使用rpc端口（需要将conf/server.yml framework改为"http+brpc"并使用新的配置重启服务）
# ./build/RelWithDebInfo_install/bin/brpc_client --server=0.0.0.0:7081

# 清理并退出客户端容器
bash build.sh clean
exit
```

### 4.3 java客户端

```bash
# 使用构建好的grps client容器环境，可以指定复用主机网络
docker run -it --rm -v $(pwd):/my_grps -w /my_grps --network=host opengrps/client:1.1.0 bash

# 构建client
cd client/java
mvn clean package
mvn dependency:copy-dependencies -DoutputDirectory=./maven-lib -DstripVersion=true

# 解决中文编码问题
export LC_ALL=zh_CN.UTF-8

# 运行, 使用rpc端口
java -classpath target/*:maven-lib/* com.netease.GrpsClient 0.0.0.0:7081

# 清理并退出客户端容器
rm -rf target maven-lib
exit
```

## 5. 关闭docker部署

```bash
docker rm -f my_grps
```
