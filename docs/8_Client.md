# 客户端

如[接口说明](2_Interface.md)
篇章介绍，grps服务支持http以及rpc请求方式，客户端可以使用任意语言实现，只需要按照接口说明进行请求即可。python客户端环境比较简单，安装对应[grps_apis pip依赖](../apis/grps_apis/python_gens/)
即可。我们提供了一个支持python、c++、java客户端的docker环境，可以直接使用，见[grps client镜像](./19_ImageList.md#客户端镜像)
，也可以参考[从源码构建接口](../apis/grps_apis/README.md)
进行构建。自定义工程创建时自动创建了python、c++以及java客户端代码样例，见[自定义模型工程-客户端请求](5_Customized.md#客户端请求)
。也可以在[这里](../template/cpp_template/client)获取客户端样例。