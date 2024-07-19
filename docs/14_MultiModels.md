# 多模型支持

grps自定义工程支持部署多个模型，多个模型可以组合成一个服务或者单独提供服务。

## 配置

grps模型推理配置定义在```inference.yml```文件中，详细介绍见[inference.yml](./5_Customized.md#1-inferenceyml)。
如下样例我们定义了两个模型分别为```your_model-1.0.0```和```your_model2-1.0.0```，分别部署到```gpu:0```以及```gpu:2```
上，并定义串联调用关系：

```yaml
models:
  - name: your_model
    version: 1.0.0
    device: cuda:0 # device of model inferer. like `cpu`, `cuda`(==`cuda:0`), `gpu`(==`cuda:0`), `cuda:0`, `gpu:0`, `original`(original device specified when exported model).
    inp_device: # when `inferer_type` is `torch` and `device` is `original`, should specify device of model inferer input.
    inferer_type: customized # only support `torch` (torch script model format), `tensorflow` (saved model format), `tensorrt` (tensorrt engine) or `customized` now.
    inferer_name: your_inferer # customized model inferer name that has registered in src/customized_inferer.h. Not none when inferer_type is `customized`.
    inferer_path: # path of model inferer.
    inferer_args: # more args of model inferer.
    converter_type: customized # only support `torch` (torch tensor converter), `tensorflow` (tf tensor converter), `tensorrt` (trt tensor converter), `customized`  or `none`(no converter mode) now.
    converter_name: your_converter # converter name that has registered in src/customized_converter.h. Not none when converter_type is `customized`.
    converter_path: # path of converter.
    converter_args: # more args of converter.
  - name: your_model2
    version: 1.0.0
    device: cuda:2 # device of model inferer. like `cpu`, `cuda`(==`cuda:0`), `gpu`(==`cuda:0`), `cuda:0`, `gpu:0`, `original`(original device specified when exported model).
    inp_device: # when `inferer_type` is `torch` and `device` is `original`, should specify device of model inferer input.
    inferer_type: customized # only support `torch` (torch script model format), `tensorflow` (saved model format), `tensorrt` (tensorrt engine) or `customized` now.
    inferer_name: your_inferer # customized model inferer name that has registered in src/customized_inferer.h. Not none when inferer_type is `customized`.
    inferer_path: # path of model inferer.
    inferer_args: # more args of model inferer.
    converter_type: customized # only support `torch` (torch tensor converter), `tensorflow` (tf tensor converter), `tensorrt` (trt tensor converter), `customized`  or `none`(no converter mode) now.
    converter_name: your_converter # converter name that has registered in src/customized_converter.h. Not none when converter_type is `customized`.
    converter_path: # path of converter.
    converter_args: # more args of converter.

dag:
  type: sequential # only support `sequential` now.
  name: your_dag # dag name.
  nodes: # sequential mode will run node in the order of nodes.
    - name: node-1
      type: model # only support `model` now.
      model: your_model-1.0.0  # model(name-version format) that has been declared in models.
    - name: node-2
      type: model # only support `model` now.
      model: your_model2-1.0.0  # model(name-version format) that has been declared in models.
```

## 调用

当我们调用predict接口时，默认没有指定模型名的情况下会使用默认模型dag进行推理，如果我们想使用某一个模型进行推理，可以在payload或query-param中指明，具体见[模型选择](./2_Interface.md#模型选择)。