# Batching

grps目前支持dynamic batching，该模式下，对于请求不会立即处理，而是通过动态组装batch
tensor的方式合并请求进行一次模型推理，这样可以充分使用GPU计算资源，对于吞吐量的提升比较明显。

<img src="dynamic_batching.png" width="600" height="auto" alt="metrics" align=center />

## 快捷部署

通过以下参数可以快速部署支持batching的服务：

```bash
--batching_type dynamic # 打开dynamic batching模式
--max_batch_size 16 # 设置dynamic batching max batch size为16
--batch_timeout_us 2000 # 设置batching组装超时时间为2ms
```

## 自定义模型工程

通过配置文件```inference.yml```配置每一个模型推理的batching模式，如下：

```yaml
models:
  - name: your_model
    ...
    batching: # Batching config.
      type: none # `none`, `dynamic`.
      max_batch_size: 16 # Maximum batch size.
      batch_timeout_us: 1000 # Maximum waiting time for batching in milliseconds.
```

用户需要实现对应的```Converter::BatchPreProcess```、```Converter::BatchPostProcess```以及```ModelInferer::BatchInfer```
接口。

## 自定义样例
* [resnet-50-tf](https://github.com/NetEase-Media/grps_examples/tree/master/cpp_examples/resnet-50-tf)
* [resnet-50-torch](https://github.com/NetEase-Media/grps_examples/tree/master/cpp_examples/resnet-50-torch)
* [resnet-50-trt](https://github.com/NetEase-Media/grps_examples/tree/master/cpp_examples/resnet-50-trt)
* [resnet-50-tf](https://github.com/NetEase-Media/grps_examples/tree/master/py_examples/resnet-50-tf)
* [resnet-50-torch](https://github.com/NetEase-Media/grps_examples/tree/master/py_examples/resnet-50-torch)
* [resnet-50-trt](https://github.com/NetEase-Media/grps_examples/tree/master/py_examples/resnet-50-trt)