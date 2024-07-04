# 内置前后处理转换器

如之前篇章所述，前后处理转换器包含一次推理的前后处理行为。grps服务内置了一些前后处理转换器可供自定义工程使用，当前只有torch、tensorflow以及tensorrt的tensor转换器，用于将grps接口传递的[gtensors](2_Interface.md#gtensors)
转换为torch、tf或trt的tensor，以及将torch、tf或trt的tensor转换为gtensors。后续会增加更多的转换器，比如cv图片解码、huggingface
tokenizer等。

1. [torch tensor converter](#torch-tensor-converter)
2. [tensorflow tensor converter](#tensorflow-tensor-converter)
3. [tensorrt tensor converter](#tensorrt-tensor-converter)
4. [注意](#注意)
5. [更多转换器](#更多转换器)

## torch tensor converter

自定义模型工程中```inference.yml```可以直接配置使用内置的torch tensor converter。

```yaml
converter_type: torch
```

将grps接口传递的gtensors转换为torch tensor（前处理以及批前处理）以及将torch tensor转换为gtensors（后处理以及批后处理），支持常见的dtype：

```text
py dtype       |  c++ dtype
---------------|----------------
torch.uint8    |  at::kByte
torch.int8     |  at::kChar
torch.int16    |  at::kShort
torch.int32    |  at::kInt
torch.int64    |  at::kLong
torch.float16  |  at::kHalf
torch.float32  |  at::kFloat
torch.float64  |  at::kDouble
```

## tensorflow tensor converter

自定义模型工程中```inference.yml```可以直接配置使用内置的tensorflow tensor converter。

```yaml
converter_type: tensorflow
```

将grps接口传递的gtensors转换为tensorflow tensor（前处理以及批前处理）以及将tensorflow
tensor转换为gtensors（后处理以及批后处理），支持常见的dtype：

```text
py dtype    |  c++ dtype
------------|------------
tf.uint8    |  DT_UINT8
tf.int8     |  DT_INT8
tf.int16    |  DT_INT16
tf.int32    |  DT_INT32
tf.int64    |  DT_INT64
tf.float16  |  DT_HALF
tf.float32  |  DT_FLOAT32
tf.float64  |  DT_FLOAT64
tf.string   |  DT_STRING
```

## tensorrt tensor converter

自定义模型工程中```inference.yml```可以直接配置使用内置的tensorrt tensor converter。

```yaml
converter_type: tensorrt
```

将grps接口传递的gtensors转换为tensorrt tensor（前处理以及批前处理）以及将tensorrt tensor转换为gtensors（后处理以及批后处理），支持常见的dtype：

```text
py dtype    |  c++ dtype
------------|------------
trt.uint8   |  nvinfer1::DataType::kUINT8
trt.int8    |  nvinfer1::DataType::kINT8
trt.int32   |  nvinfer1::DataType::kINT32
trt.float32 |  nvinfer1::DataType::kFLOAT
```

## 注意

* 当gtensor中名字为空时，会忽略所有tensor名，按照输入顺序进行转换并作为模型的输入。
* python自定义工程一般用于实现c++不好实现的自定义功能，不太建议直接使用torch、tensorflow、tensorrt tensor
  converter，因为性能相比较c++较差，这种情况可以直接使用c++自定义工程或[快捷部署](./4_QuickDeploy.md)，快捷部署功能由纯c++实现。

## 更多转换器

【进行中...】