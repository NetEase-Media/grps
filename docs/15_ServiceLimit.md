# 服务限制

grps支持tf和torch显存限制功能，适用于共享gpu场景；同时也只支持服务请求的并发限制。这些限制是通过配置实现的。

## 快捷部署

通过```grpst tf_serve/torch_serve```的参数实现，如下参数：

```bash
--max_connections 100  # 设置最大连接数
--max_concurrency 10  # 设置最大推理并发数
--gpu_devices_idx 4  # GPU限制和监控的设备号，可以是多个，用逗号分隔，在服务指标页面可以看到对应的GPU卡使用率和显存大小，且会当指定了显存限制时，会对对应GPU卡的显存进行限制
--gpu_mem_limit_mib 4096  # 设置模型运行的显存限制
--gpu_mem_gc_enable  # 设置开启显存定时垃圾回收，tf_serve暂时不支持
--gpu_mem_gc_interval 60  # 设置显存定时垃圾回收的时间间隔，单位为秒
```

## 自定义模型工程

通过配置文件```server.yml```实现，见[server.yml配置](5_Customized.md#2-serveryml)。