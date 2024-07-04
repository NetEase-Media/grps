# 内置模型推理后端

如之前篇章所述，深度学习模型推理器包含模型加载和推理行为。grps服务内置了一些模型推理器可供自定义工程使用，包含torch、tensorflow以及tensorrt的模型推理器，后续会增加更多的推理器。

1. [torch inferer](#torch-inferer)
2. [tensorflow inferer](#tensorflow-inferer)
3. [tensorrt inferer](#tensorrt-inferer)
4. [注意](#注意)

## torch inferer

自定义模型工程中```inference.yml```可以直接配置使用内置的torch inferer。

```yaml
inferer_type: torch
```

* 支持torch jit script model格式，导出方法参考官方文档：[torch script](https://pytorch.org/docs/stable/jit.html)。
* python自定义工程中torch inferer支持的输入格式包含```torch.Tensor```的单值、list、tuple、dict以及torch jit script
  module支持的输入格式。直接返回torch model的输出。
* c++自定义工程中torch inferer的输入格式固定为```std::vector<std::pair<std::string, TensorWrapper>>```
  ，其中````string````代表tensor名，```TensorWrapper```是一个支持```torch::Tensor```
  的包装类，用户可以根据自己的需求进行构建和解析。返回格式也是```std::vector<std::pair<std::string, TensorWrapper>>```。

## tensorflow inferer

自定义模型工程中```inference.yml```可以直接配置使用内置的tensorflow inferer。

```yaml
inferer_type: tensorflow
inferer_args:
  customized_op_paths: # 支持加载自定义op插件库
  #- /path/to/your/customized_op.so
```

* 支持tensorflow saved
  model格式模型文件，导出方法参考官方文档：[tensorflow saved model](https://www.tensorflow.org/guide/saved_model)
  需要注意导出模型使用默认的tag(serve)以及signature_def_key(serving_default)。
* 支持加载自定义op插件库，用户可以通过配置```customized_op_paths```加载自定义op插件库。
* python自定义工程中tensorflow inferer支持的输入格式包含```tf.Tensor```的单值、list、tuple、dict以及tensorflow saved
  model支持的输入格式。直接返回tf model的输出。
* c++自定义工程中tensorflow inferer的输入格式固定为```std::vector<std::pair<std::string, TensorWrapper>>```
  ，其中````string````代表tensor名，```TensorWrapper```是一个支持```tensorflow::Tensor```
  的包装类，用户可以根据自己的需求进行构建和解析。返回格式也是```std::vector<std::pair<std::string, TensorWrapper>>```。

## tensorrt inferer

自定义模型工程中```inference.yml```可以直接配置使用内置的tensorrt inferer。

```yaml
inferer_type: tensorrt
inferer_args:
  customized_op_paths: # 支持加载自定义op插件库
  #- /path/to/your/customized_op.so
  streams: 4 # 支持配置多推理流数
```

* 支持tensorrt
  engine格式模型文件，导出方法参考官方文档：[tensorrt engine](https://docs.nvidia.com/deeplearning/tensorrt/developer-guide/index.html)
* 支持加载自定义op插件库，用户可以通过配置```customized_op_paths```加载自定义op插件库。
* 支持配置推理流数，用户可以通过配置```streams```设置推理流数。打开多流配置可以提高GPU使用率以及推理性能，相应也会增加GPU显存占用。
* python自定义工程中tensorrt inferer支持的输入格式包含```numpy.ndarray```
  的单值、list、tuple、dict。返回格式为```map(name, numpy.ndarray)```。
* c++自定义工程中tensorrt inferer的输入格式固定为```std::vector<std::pair<std::string, TensorWrapper>>```
  ，其中````string````代表tensor名，```TensorWrapper```是一个支持```TrtHostBinding```
  的包装类，用户可以根据自己的需求进行构建和解析。返回格式也是```std::vector<std::pair<std::string, TensorWrapper>>```。

## 注意

* 当tensor名字为空时，会忽略所有输入tensor名，按照输入顺序作为模型的输入。