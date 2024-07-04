# grps apis

包含grps服务访问接口定义、构建与安装。目前支持python、cpp与java。

## 接口定义

使用protobuf定义，见grps.proto。

## 构建与安装

### python

依赖requests、grpcio>=1.46.0、grpcio-tools>=1.46.0。

```
bash py_install.sh
```

### cpp

依赖grpc-cpp环境，参考[grpc-cpp官方安装文档](https://grpc.io/docs/languages/cpp/quickstart/)
。brpc环境可选择性安装，参考[brpc官方安装文档](https://brpc.apache.org/zh/docs/getting_started/)。<br>验证过的版本：

* [grpc-1.25.0](https://github.com/grpc/grpc/tree/v1.25.x)
* [brpc-1.6.0](https://github.com/apache/brpc/tree/1.6.0)

```
bash cpp_install.sh
```

### java

依赖maven、jdk、以及protoc-gen-grpc-java（安装到/usr/local/bin/protoc-gen-grpc-java）环境。<br>验证过的版本：

* [maven-3.6.3](https://archive.apache.org/dist/maven/maven-3/3.6.3)
* [jdk-1.8](https://docs.aws.amazon.com/corretto/latest/corretto-8-ug/downloads-list.html)
* [protoc-gen-grpc-java-1.46.0](https://repo1.maven.org/maven2/io/grpc/protoc-gen-grpc-java/1.46.0/protoc-gen-grpc-java-1.46.0-linux-x86_64.exe)

```
bash java_install.sh
```
