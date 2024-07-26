# docker部署

我们在部署服务到线上生产环境时很多时候都是使用docker进行部署，如前文所述，grps提供了docker基础镜像可以直接使用：[镜像列表](19_ImageList.md)。
对于自定义模型工程，可以基于基础镜像构建自己的镜像，自定义工程中的docker/Dockerfile已经给出构建自定义工程镜像的过程，如下：

c++自定义工程：

```dockerfile
# --------Building stage.--------
FROM registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7 AS build

# grps archive.
RUN mkdir -p /my_grps
ADD conf /my_grps/conf
ADD data /my_grps/data
ADD second_party /my_grps/second_party
ADD src /my_grps/src
ADD third_party /my_grps/third_party
ADD build.sh /my_grps/build.sh
ADD CMakeLists.txt /my_grps/CMakeLists.txt
ADD .config /my_grps/.config
RUN cd /my_grps && \
    grpst archive . --skip_unittest --output_path server.mar

# --------Release stage.--------
FROM registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7

WORKDIR /my_grps
COPY --from=build /my_grps/server.mar /my_grps/server.mar
ENV LANG C.UTF-8
# grpst start server.mar可以跟上--timeout参数增加超时时间，默认为300s
CMD ["/bin/sh", "-c", "grpst start server.mar"]
```

python自定义工程：

```dockerfile
# --------Building stage.--------
# 构建阶段，将会构建出server.mar文件
# 可以修改为自己所需的基础镜像版本
# 例如：FROM registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps*_cuda11.8_cudnn8.6_torch2.1.2_py 基础镜像仅包含pytorch2.1.2和python支持，镜像相对较小
FROM registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7 AS build

# grps archive.
RUN mkdir -p /my_grps
ADD conf /my_grps/conf
ADD data /my_grps/data
ADD src /my_grps/src
ADD requirements.txt /my_grps/requirements.txt
ADD test.py /my_grps/test.py
RUN cd /my_grps && \
    grpst archive . --skip_unittest --output_path server.mar

# --------Release stage.--------
# 发布阶段，使用构建阶段构建出的server.mar文件，构建出最终的发布镜像
# 可以修改为自己所需的基础镜像版本
# 例如：FROM registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps*_cuda11.8_cudnn8.6_torch2.1.2_py 基础镜像仅包含pytorch2.1.2和python支持，镜像相对较小
FROM registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7

# Intall requirements.
ADD requirements.txt /tmp/requirements.txt
# 可以自行添加国内源，例如 -i https://pypi.mirrors.ustc.edu.cn/simple/ 
RUN pip install -r /tmp/requirements.txt

WORKDIR /my_grps
COPY --from=build /my_grps/server.mar /my_grps/server.mar
ENV LANG C.UTF-8
# grpst start server.mar可以跟上--timeout参数增加超时时间，默认为300s
CMD ["/bin/sh", "-c", "grpst start server.mar"]
```

通过如下步骤即可完成自定义工程的镜像构建和发布：

```bash
cd my_grps

# 构建自定义工程docker镜像
# 注意可以修改Dockerfile中的基础镜像版本，选择自己所需的版本号，默认为registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda10.1_cudnn7.6.5_tf2.3.0_torch1.8.1_py3.7镜像
# pip install比较慢时可以手动加上国内源
docker build -t my_grps:1.0.0 -f docker/Dockerfile .

# 使用上面构建好的镜像启动docker容器
# 注意映射端口号
docker run -itd --runtime=nvidia --name="my_grps" -p 7080:7080 -p 7081:7081 my_grps:1.0.0

# 使用docker logs可以跟踪服务日志
docker logs -f my_grps
```