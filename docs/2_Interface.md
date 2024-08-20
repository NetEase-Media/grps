# 接口说明

本章节详细介绍grps服务的接口规范，接口形式包括RESTful（HTTP）接口以及RPC接口两种形式。

1. [统一payload](#统一payload)
2. [RESTful接口](#restful接口)
3. [RPC接口](#rpc接口)
4. [Streaming流式访问](#Streaming流式访问)
5. [自定义HTTP格式访问](#自定义http格式访问)
6. [模型选择](#模型选择)

## 统一payload

RESTful（HTTP）接口以及RPC接口保持统一的请求和回复payload，使用protobuf来定义，RESTful（HTTP）接口使用json格式表达pb结构。

```protobuf
message GrpsMessage {
  Status status = 1; // Not need when request message
  string model = 2; // Select model(with `name-version` format) to predict. If not define, will use default model dag (defined in inference.yml) to predict.
  oneof data_oneof {
    bytes bin_data = 3;
    string str_data = 4;
    GenericTensorData gtensors = 5; // Generic tensor data
    NDArrayData ndarray = 6; // Multi-dimensional array data to represent gtensors, only used when http json request and one DT_FLOAT32 tensor
    GenericMapData gmap = 7; // Generic map data
  }
}
```

GrpsMessage用于表示请求和回复的payload，status用于表示状态，用于表示回复消息的状态；model用于指定模型名称，如果不指定则使用默认模型dag（在inference.yml中定义）进行推理；data_oneof用于表示用户数据，用户数据可以是：

* bin_data：二进制数据。
* str_data：字符串数据。
* gtensors：使用GenericTensorData表示通用的multi tensors格式。
* ndarray：使用多维数组映射gtensors，仅用于http请求，仅支持一个DT_FLOAT32类型的tensor数据。
* gmap：使用GenericMapData表示通用的map数据格式，用于传输常见的map格式数据。

完整定义见[grps.proto](../apis/grps_apis/grps.proto)。

### gtensors

gtensors用于表示多个tensor数据，使用GenericTensorData表示，具体定义如下：

```protobuf
enum DataType {
  DT_INVALID = 0;
  DT_UINT8 = 1; // byte
  DT_INT8 = 2; // char
  DT_INT16 = 3; // short
  DT_INT32 = 4; // int
  DT_INT64 = 5; // long
  DT_FLOAT16 = 6; // half
  DT_FLOAT32 = 7; // float
  DT_FLOAT64 = 8; // double
  DT_STRING = 9; // string
}

message GenericTensor {
  string name = 1;
  DataType dtype = 2;
  repeated uint32 shape = 3 [packed = true];

  // flatten data, row-major order to save multi-dimensional array data.
  repeated uint32 flat_uint8 = 4 [packed = true];
  repeated int32 flat_int8 = 5 [packed = true];
  repeated int32 flat_int16 = 6 [packed = true];
  repeated int32 flat_int32 = 7 [packed = true];
  repeated int64 flat_int64 = 8 [packed = true];
  repeated float flat_float16 = 9 [packed = true];
  repeated float flat_float32 = 10 [packed = true];
  repeated double flat_float64 = 11 [packed = true];
  repeated string flat_string = 12;
}

// Generic tensor data
message GenericTensorData {
  repeated GenericTensor tensors = 1;
}
```

### gmap

gmap用于表示常见的map数据格式，使用GenericMapData表示，具体定义如下：

```protobuf
// Generic map data
message GenericMapData {
  map<string, string> s_s = 1;
  map<string, bytes> s_b = 2; // If used in http json request or json response, the bytes data should be encoded by base64 when transferring. Grps server will automatically encode and decode.
  map<string, int32> s_i32 = 3;
  map<string, int64> s_i64 = 4;
  map<string, float> s_f = 5;
  map<string, double> s_d = 6;
}
```

需要注意的是，当使用http json请求或者回复时，s_b中的bytes数据需要使用base64编码，grps服务端会自动解码和编码，客户端需要自主进行编解码，由于base64编解码比较耗时，所以当http
json请求时不建议使用该字段。有如下建议：

* 建议这种情况使用rpc访问。
* 可以用“application/octet-stream”传输二进制数据，服务端会自动解析为bin_data，见下文RESTful（HTTP）接口说明。
* 可以完全自定义http接口，见[自定义http请求格式](./5_Customized.md#4-实现自定义http请求获取和返回)。

## RESTful接口

使用HTTP协议访问，服务根路径为“http://host:http_port/grps/v1/”，接口形式为“/grps/v1/{group}/{method}”。<br>
GrpsMessage需要以json的格式表达pb结构，并以“application/json”格式进行请求，grps服务会自动转换json为pb结构，处理完请求后也会转换为json格式返回给客户端。需要注意的是json描述枚举值时可以使用枚举int值，也可以用枚举名称字符串，例如DataType
DT_UINT8类型，即可以用“DT_UINT8”，也可以用1来表示。
另外有下面一点特殊约定：

* 当用户数据是bin_data，为了避免base64编解码导致较差接口访问性能，这里不使用“application/json”格式传输，而是使用“application/octet-stream”格式传输request
  payload，直接将bin_data部分放到http request
  payload中即可，grps服务会自动识别为bin_data。同样，当返回用户数据为bin_data时，也直接使用“application/octet-stream”格式传输response
  payload。

### Health接口

#### 上线

* endpoint: GET /grps/v1/health/online
* request payload example:
    ```
    {}
    ```
* response payload example:
    ```
    {
      "status": {
        "code": 200,
        "msg": "OK",
        "status": "SUCCESS"
      }
    }
    ```

#### 下线

* endpoint: GET /grps/v1/health/offline
* request payload example:
    ```
    {}
    ```
* response payload example:
    ```
    {
      "status": {
        "code": 200,
        "msg": "OK",
        "status": "SUCCESS"
      }
    }
    ```

#### 查看服务是否存活

* endpoint: GET /grps/v1/health/live
* request payload example:
    ```
    {}
    ```
* response payload example:
    ```
    {
      "status": {
        "code": 200,
        "msg": "OK",
        "status": "SUCCESS"
      }
    }
    ```

#### 查看服务是否ready

* endpoint: GET /grps/v1/health/ready
* request payload example:
    ```
    {}
    ```
* response payload example:
    ```
    {
      "status": {
        "code": 200,
        "msg": "OK",
        "status": "SUCCESS"
      }
    }
    ```

### Infer接口

#### 推理

* endpoint: POST /grps/v1/infer/predict
* request payload example:
    ```
    # 字符串格式样例：
    {
      "str_data": "hello grps"
    }
  
    # ndarray格式样例：
    {
      "ndarray": [[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]]
    }
  
    # gtensors格式样例：
    {
      "gtensors": {
        "tensors": [
          {
            "name": "in1",
            "dtype": "DT_INT32",
            "shape": [3, 2],
            "flat_int32": [1, 2, 3, 4, 5, 6]
          }, 
          {
            "name": "in2",
            "dtype": "DT_FLOAT64",
            "shape": [3, 3],
            "flat_double": [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0]
          }
        ]
      }
    }
  
    # 特殊的，bin_data数据直接使用“application/octet-stream”传输）
    ```
* response payload example：<br>
    ```
    # 字符串格式样例：
    {
      "status": {
        "code": 200,
        "msg": "OK",
        "status": "SUCCESS"
      },
      "str_data": "hello grps"
    }
    
    # ndarray格式样例（需要设置query-param “return-ndarray=true”才会返回ndarray格式）：
    {
      "status": {
        "code": 200,
        "msg": "OK",
        "status": "SUCCESS"
      },
      "ndarray": [[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]]
    }
  
    # gtensors格式样例：
    {
      "status": {
        "code": 200,
        "msg": "OK",
        "status": "SUCCESS"
      },
      "gtensors": {
        "tensors": [
          {
            "name": "out1",
            "dtype": "DT_INT32",
            "shape": [3, 2],
            "flat_int32": [1, 2, 3, 4, 5, 6]
          }, 
          {
            "name": "out2",
            "dtype": "DT_FLOAT64",
            "shape": [3, 3],
            "flat_double": [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0]
          }
        ]
      }
    }
  
    # 特殊的，bin_data数据直接使用“application/octet-stream”传输）
    ```

### Metadata接口

用于获取服务描述信息元数据，meta元数据存放在GrpsMessage的strData属性中，数据格式为yaml格式。

#### Server level

* endpoint: GET /grps/v1/metadata/server
* request payload example:
    ```
    {}
    ```
* response payload example:
    ```
    {
      "status": {
        "code": 200,
        "msg": "OK",
        "status": "SUCCESS"
      }
      "str_data": "server level meta data description string with yaml format"
    }
    ```

#### Model level

* endpoint: POST /grps/v1/metadata/model
* request payload example:
    ```
    {
      "str_data": "model_name"
    }
    ```
* response payload example:
    ```
    {
      "status": {
        "code": 200,
        "msg": "OK",
        "status": "SUCCESS"
      }
      "str_data": "model level meta data description string with yaml format"
    }
    ```

### Metrics接口

Metrics接口主要用于观测服务指标，用户可以使用web浏览器直接访问根路径：“http://host:http_port/”。

## RPC接口

支持GRPC，BRPC访问，BRPC目前不支持py自定义模型工程。

### Health接口

#### 上线

```protobuf
rpc Online(GrpsMessage) returns (GrpsMessage) {};
```

#### 下线

```protobuf
rpc Offline(GrpsMessage) returns (GrpsMessage) {};
```

#### 查看服务是否存活

```protobuf
rpc CheckLiveness(GrpsMessage) returns (GrpsMessage) {};
```

#### 查看服务是否ready

```protobuf
rpc CheckReadiness(GrpsMessage) returns (GrpsMessage) {};
```

### Infer接口

```protobuf
rpc Predict(GrpsMessage) returns (GrpsMessage) {};
```

### Metadata接口

#### Server level

```protobuf
rpc ServerMetadata(GrpsMessage) returns (GrpsMessage) {};
```

#### Model level

```protobuf
rpc ModelMetadata(GrpsMessage) returns (GrpsMessage) {}; 
```

## Streaming流式访问

仅推理接口支持streaming模式，streaming模式可以实现模型持续推理并返回结果，具体实现方式见[自定义模型工程-streaming流式返回](./5_Customized.md#3-streaming流式返回)。

### HTTP Streaming

当使用推理接口时，根据[streaming模式的控制方式](./5_Customized.md#配置http-streaming模式控制)
进行参数设置，继而使用streaming模式请求，默认情况下使用```streaming=true```
query-param控制。grps服务端使用```http chunked transfer-encoding```
方式进行streaming响应，response content type见[streaming模式相关配置](./5_Customized.md#配置http-streaming模式控制)
，默认为```application/octet-stream```。

### GRPC Streaming

使用如下predict streaming接口进行streaming请求：

```protobuf
rpc PredictStreaming(GrpsMessage) returns (stream GrpsMessage) {};
```

### BRPC Streaming

暂不支持。

## 自定义HTTP格式访问

推理接口支持用户完全自定义http请求和返回格式，用户需要打开自定义http配置（```server.yml - interface -
customized_predict_http```
）并自行解析http请求和构建http返回，具体见[实现自定义http请求获取和返回](5_Customized.md#4-实现自定义http请求获取和返回)。
自定义HTTP同样支持streaming模式。

## 模型选择

接口支持选择模型进行推理，模型名称格式为“name-version”，例如“your_model-1.0.0”，如果不指定模型名称，则使用默认模型dag（在inference.yml中定义）进行推理。模型参数可以指定在payload中或者query-param中，同时指定时payload中的参数优先级高于query-param中的参数，另外自定义HTTP格式只能将模型参数指定在query-param中。例如：

### payload中指定模型

```
# http json payload中指定：
{
  "model": "your_model-1.0.0",
  "str_data": "hello grps"
}

# grpc GrpsMessage中指定：
{
  "model": "your_model-1.0.0",
  "str_data": "hello grps"
}
```

### query-param中指定模型

```bash
curl -X POST -H "Content-Type:application/json" -d '{"str_data": "hello grps"}' 'http://ip:port/grps/v1/infer/predict?model=your_model-1.0.0'
```