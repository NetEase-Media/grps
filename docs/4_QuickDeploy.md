# 快捷部署

本章节详细介绍如何使用grpst快捷部署tensorflow、torch以及tensorrt模型服务，即无需编写代码，只需准备好模型文件即可快速部署模型服务。

1. [模型格式](#模型格式)
2. [部署](#部署)
3. [客户端请求](#客户端请求)
4. [观测服务指标](#观测服务指标)
5. [关闭服务](#关闭服务)
6. [样例](#样例)

## 模型格式

### tensorflow

支持tensorflow saved
model格式模型文件，导出方法参考官方文档：[tensorflow saved model](https://www.tensorflow.org/guide/saved_model)。<br>
需要注意导出模型使用默认的tag(serve)以及signature_def_key(serving_default)。<br>

### torch

支持torch jit script model格式，导出方法参考官方文档：[torch script](https://pytorch.org/docs/stable/jit.html)。<br>

### tensorrt

支持tensorrt engine格式，导出方法参考官方文档：[tensorrt engine](https://docs.nvidia.com/deeplearning/tensorrt/developer-guide/index.html)。<br>

## 部署

### tensorflow

使用```grpst tf_serve```命令进行部署，详细参数说明见[快速部署tensorflow模型服务](../grpst/README.md#快速部署tensorflow模型服务)
，也可以执行```grpst tf_serve -h```查看。

```bash
# 使用默认参数启动
grpst tf_serve /tmp/saved_model

# 指定参数启动
grpst tf_serve /tmp/saved_model \  # 模型文件路径
    --name my_grps \  # 设置服务名称
    --interface_framework http+grpc \  # 设置接口模式，同时打开http和grpc接口模式
    --port 7080,7081 \  # 设置http和grpc接口的端口号
    --device gpu:4 \  # 设置模型运行的设备
    --customized_op_paths /tmp/customized_op.so \  # 设置自定义op插件库路径，可以是多个，用逗号分隔
    --batching_type dynamic \ # 打开dynamic batching模式
    --max_batch_size 16 \ # 设置dynamic batching max batch size为16
    --batch_timeout_us 2000 \ # 设置batching组装超时时间为2ms
    --max_connections 100 \  # 设置最大连接数
    --max_concurrency 10 \  # 设置最大推理并发数
    --gpu_devices_idx 4 \  # GPU监控的设备号，可以是多个，用逗号分隔，在服务指标页面可以看到对应的GPU卡使用率和显存大小，且会当指定了显存限制时，会对对应GPU卡的显存进行限制
    --gpu_mem_limit_mib 4096 \  # 设置模型运行的显存限制
    --log_dir /tmp/log \  # 设置日志目录，如果是相对路径，则路径在模型部署路径下，使用grpst ps命令查看部署路径
    --log_backup_count 7 \  # 设置日志备份数量，按天进行备份，超过数量后，会自动清理
    --timeout 60 # 设置启动超时时间，单位为秒
    
```

### torch

使用```grpst torch_serve```命令进行部署，详细参数说明见[快速部署torch模型服务](../grpst/README.md#快速部署torch模型服务)
，也可以执行```grpst torch_serve -h```查看。

```bash
# 使用默认参数启动
grpst torch_serve /tmp/script_model.pt

# 指定参数启动
grpst torch_serve /tmp/script_model.pt \  # 模型文件路径
    --name my_grps \  # 设置服务名称
    --interface_framework http+grpc \  # 设置接口模式，同时打开http和grpc接口模式
    --port 7080,7081 \  # 设置http和grpc接口的端口号
    --device gpu:4 \  # 设置模型运行的设备类型
    --batching_type dynamic \ # 打开dynamic batching模式
    --max_batch_size 16 \ # 设置dynamic batching max batch size为16
    --batch_timeout_us 2000 \ # 设置batching组装超时时间为2ms
    --max_connections 100 \  # 设置最大连接数
    --max_concurrency 10 \  # 设置最大推理并发数
    --gpu_devices_idx 4 \  # GPU监控的设备号，可以是多个，用逗号分隔，在服务指标页面可以看到对应的GPU卡使用率和显存大小，且会当指定了显存限制时，会对对应GPU卡的显存进行限制
    --gpu_mem_limit_mib 4096 \  # 设置模型运行的显存限制
    --gpu_mem_gc_enable \  # 设置开启显存定时垃圾回收
    --gpu_mem_gc_interval 60 \  # 设置显存定时垃圾回收的时间间隔，单位为秒
    --log_dir /tmp/log \  # 设置日志目录，如果是相对路径，则路径在模型部署路径下，使用grpst ps命令查看部署路径
    --log_backup_count 7 \  # 设置日志备份数量，按天进行备份，超过数量后，会自动清理
    --timeout 60  # 设置启动超时时间，单位为秒
```

### tensorrt

使用```grpst trt_serve```命令进行部署，详细参数说明见[快速部署tensorrt模型服务](../grpst/README.md#快速部署tensorrt模型服务)
，也可以执行```grpst trt_serve -h```查看。

```bash
# 使用默认参数启动
grpst trt_serve /tmp/engine.trt

# 指定参数启动
grpst trt_serve /tmp/engine.trt \  # 模型文件路径
    --name my_grps \  # 设置服务名称
    --interface_framework http+grpc \  # 设置接口模式，同时打开http和grpc接口模式
    --port 7080,7081 \  # 设置http和grpc接口的端口号
    --device gpu:4 \  # 设置模型运行的设备类型
    --streams 4 \  # 打开多流配置可以提高GPU使用率以及推理性能，相应也会增加GPU显存占用。
    --customized_op_paths /tmp/customized_op.so \  # 设置自定义op插件库路径，可以是多个，用逗号分隔
    --batching_type dynamic \ # 打开dynamic batching模式
    --max_batch_size 16 \ # 设置dynamic batching max batch size为16
    --batch_timeout_us 2000 \ # 设置batching组装超时时间为2ms
    --max_connections 100 \  # 设置最大连接数
    --max_concurrency 10 \  # 设置最大推理并发数
    --gpu_devices_idx 4 \  # GPU监控的设备号，可以是多个，用逗号分隔，在服务指标页面可以看到对应的GPU卡使用率和显存大小
    --log_dir /tmp/log \  # 设置日志目录，如果是相对路径，则路径在模型部署路径下，使用grpst ps命令查看部署路径
    --log_backup_count 7 \  # 设置日志备份数量，按天进行备份，超过数量后，会自动清理
    --timeout 60  # 设置启动超时时间，单位为秒
```

### 支持导出为grps mar格式

使用```--output server.mar```参数，可以导出为grps mar格式，下次可以可以直接使用```grpst start server.mar```
启动服务，参数已经被保存在mar文件中。

## 客户端请求

客户端可以通过http、grpc、brpc接口进行请求。

### http接口

```python
import numpy as np
import requests

# 可以使用gtensor格式进行访问，如下代码：
img = np.random.rand(1, 224, 224, 3).astype(np.float32)
gtensors = {
    'tensors': [{
        'name': '',  # name is not defined, will use default name.
        'dtype': 'DT_FLOAT32',
        'shape': [1, 3, 224, 224],
        'flat_float32': img.flatten().tolist()
    }]
}
# 发送请求
response = requests.post('http://127.0.0.1:7080/grps/v1/infer/predict', json={'gtensors': gtensors})
# response也为gtensor格式，可以通过response.json()['gtensors']获取返回的tensor
gtensors = response.json()['gtensors']

# 当模型输入是一个float32 tensor时，可以选择使用ndarray格式进行访问，如下代码：
img = np.random.rand(1, 224, 224, 3).astype(np.float32)
# 发送请求
response = requests.post('http://127.0.0.1:7080/grps/v1/infer/predict?return-ndarray=true',
                         json={'ndarray': img.tolist()})
# response也为ndarray格式，可以通过response.json()['ndarray']获取返回的tensor
ndarray = response.json()['ndarray']
```

### grpc接口

```python
# 仅支持gtensor格式进行访问，如下代码：
import numpy as np
import grpc
from grps_apis.grps_pb2 import DataType, GenericTensor, GrpsMessage
from grps_apis.grps_pb2_grpc import GrpsServiceStub

grpc_client = GrpsServiceStub(channel=grpc.insecure_channel('127.0.0.1:7081'))
img = np.random.rand(1, 224, 224, 3).astype(np.float32)
request = GrpsMessage()

# 插入第一个tensor
request.gtensors.tensors.append(
    GenericTensor(name='',  # name is not defined, will use default name,
                  dtype=DataType.DT_FLOAT32,
                  shape=[1, 3, 224, 224],
                  flat_float32=img.flatten().tolist()))
# 可以继续插入其他tensor

# 发送请求
response = grpc_client.Predict(request)
# response也为GrpsMessage类型，可以通过response.gtensors获取返回的tensor
gtensors = response.gtensors
```

### brpc接口

使用brpc接口时，需要指定使用brpc模式，即```--interface_framework http+brpc```。

```cpp
// 创建brpc stub
brpc::Channel channel;
brpc::ChannelOptions options;
options.protocol = brpc::PROTOCOL_BAIDU_STD;
options.connection_type = FLAGS_connection_type;
options.timeout_ms = FLAGS_timeout_ms /*milliseconds*/;
options.max_retry = FLAGS_max_retry;
if (channel.Init("0.0.0.0:7081", "", &options) != 0) {
  LOG(ERROR) << "Fail to initialize channel";
  return -1;
}
grps::protos::v1::GrpsBrpcService_Stub stub(&channel);

// 创建请求
grps::protos::v1::GrpsMessage request;
// 插入第一个gtensor([[1.0, 2.0, 3.0]]):
auto* g_tensor = output.mutable_gtensors()->add_tensors();
// name is not defined, will use default name.
g_tensor->set_name("");
// shape [1, 3]
g_tensor->add_shape(1);
g_tensor->add_shape(3);
// data type float32
g_tensor->set_dtype(::grps::protos::v1::DT_FLOAT32);
// data [1, 2, 3]
g_tensor->add_flat_float32(1);
g_tensor->add_flat_float32(2);
g_tensor->add_flat_float32(3);

// 发送请求
grps::protos::v1::GrpsMessage response;
stub.Predict(&cntl, &request, &response, nullptr);
if (cntl.Failed()) {
  LOG(ERROR) << "Fail to send predict request, " << cntl.ErrorText();
  return -1;
}

// response也为GrpsMessage类型，可以通过response.gtensors获取返回的tensor
auto& gtensors = response.gtensors();
```

## 观测服务指标

登录(http://host:7080/) 可以观测服务指标：<br>
<img src="metrics.png" width="600" height="auto" alt="metrics" align=center />

## 关闭服务

```bash
grpst stop my_grps
```

## 样例

见[quick_examples](https://github.com/NetEase-Media/grps_examples/tree/master/quick_examples)。
