# 从源码构建

## 从源码构建docker镜像

[跳转](../docker/README.md)。

## 构建安装grps客户端环境

[跳转](../apis/grps_apis/README.md)。

## 开发容器中重新编译安装grps

有时需要在开发容器中重新编译安装grps，可以参考如下步骤：

```bash
# clone open grps源码
git clone

# 进入open grps源码目录
cd open_grps

# 配置grps，按提示打开对应的选项
source grps_env.sh

# 编译安装grps，如果开发容器中已经安装了依赖可以跟上--skip_deps参数跳过依赖安装
bash grps_install.sh
```
