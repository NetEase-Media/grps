# Grpst(Generic Realtime Predict Service Tool)

grpst作为grps服务的工具链，提供了一系列工具方便算法用户快速接入和集成自己的模型服务，包括创建工程、打包工程、部署工程、查看状态等。

1. [tab提示](#tab提示)
2. [命令](#命令)
    1. [创建工程](#创建工程)
    2. [打包工程](#打包工程)
    3. [部署工程](#部署工程)
    4. [快速部署tensorflow模型服务](#快速部署tensorflow模型服务)
    5. [快速部署torch模型服务](#快速部署torch模型服务)
    6. [快速部署tensorrt模型](#快速部署tensorrt模型服务)
    7. [查看状态](#查看状态)
    8. [停止服务](#停止服务)
    9. [查看日志](#查看日志)

## tab提示

grpst支持输入tab键提示命令和参数，默认安装完毕后bash环境自动使能。fish环境可以将如下内容添加到/etc/fish/config.fish中并重启fish。

```
export GRPST_HOME=/usr/local/grpst
source $GRPST_HOME/grpst-completion-fish.sh
```

## 命令

```
usage: grpst [-h] [-v] {create,archive,start,tf_serve,torch_serve,trt_serve,stop,ps,logs} ...

Generic Realtime Prediction Service Tools.

positional arguments:
  {create,archive,start,tf_serve,torch_serve,trt_serve,stop,ps,logs}
                        sub-command help
    create              create project
    archive             archive project
    start               start project
    tf_serve            quick deploy tensorflow model grps server without create, archive
    torch_serve         quick deploy torch model grps server without create, archive
    trt_serve           quick deploy tensorrt model grps server without create, archive
    stop                stop project
    ps                  print status
    logs                show server logs

optional arguments:
  -h, --help            show this help message and exit
  -v, --version         show program's version number and exit
```

### 创建工程

```
usage: grpst create [-h] [--project_type PROJECT_TYPE] project_path

positional arguments:
  project_path          project path will be created

optional arguments:
  -h, --help            show this help message and exit
  --project_type PROJECT_TYPE
                        proj type support "py", "cpp", "cpp_torch", "cpp_tf".
                        "py": python project with pytorch and tf support.
                        "cpp": c++ project without libtorch and libtf support.
                        "cpp_torch": c++ project with libtorch support.
                        "cpp_tf": c++ project with libtf support.
                        "cpp_trt": c++ project with libtensorrt support.
```

### 打包工程

```
usage: grpst archive [-h] [--output_path OUTPUT_PATH] project_path

positional arguments:
  project_path          project path

optional arguments:
  -h, --help            show this help message and exit
  --skip_unittest       skip unit test, default is False
  --output_path OUTPUT_PATH
                        output path, default is "./server.mar"
```

### 部署工程

```
usage: grpst start [-h] [--name NAME] [--conf_path CONF_PATH]
                   [--timeout TIMEOUT]
                   mar_path

positional arguments:
  mar_path              model server archive path

optional arguments:
  -h, --help            show this help message and exit
  --name NAME           server name, default is "my_grps"
  --conf_path CONF_PATH
                        server conf path, will use server.yml in mar(mar_path
                        arg) if not set
  --timeout TIMEOUT     server start timeout, default is 300s
```

### 快速部署tensorflow模型服务

```
usage: grpst tf_serve [-h] [--name NAME] [--interface_framework {http,http+grpc,http+brpc}] [--port PORT] [--customized_predict_http_path CUSTOMIZED_PREDICT_HTTP_PATH] [--device DEVICE]
                      [--batching_type BATCHING_TYPE] [--max_batch_size MAX_BATCH_SIZE] [--batch_timeout_us BATCH_TIMEOUT_US] [--max_connections MAX_CONNECTIONS] [--max_concurrency MAX_CONCURRENCY]
                      [--gpu_devices_idx GPU_DEVICES_IDX] [--gpu_mem_limit_mib GPU_MEM_LIMIT_MIB] [--customized_op_paths CUSTOMIZED_OP_PATHS] [--log_dir LOG_DIR] [--log_backup_count LOG_BACKUP_COUNT]
                      [--output_path OUTPUT_PATH] [--timeout TIMEOUT]
                      model_path

positional arguments:
  model_path            tensorflow saved model path, only support saved model format

optional arguments:
  -h, --help            show this help message and exit
  --name NAME           server name, default is "my_grps"
  --interface_framework {http,http+grpc,http+brpc}
                        interface framework, should be one of "http", "http+grpc", "http+brpc", default is "http+grpc"
  --port PORT           http port and rpc port split by comma, default is "7080,7081"
  --customized_predict_http_path CUSTOMIZED_PREDICT_HTTP_PATH
                        customized predict http path, default is empty and will not use default path
  --device DEVICE       device of model, should be one of "cpu", "gpu"(same as cuda:0), "cuda"(same as cuda:0), "cuda:[num]", "gpu:[num]" or "original"(original device specified when exported model),
                        default is "cuda"
  --batching_type BATCHING_TYPE
                        batching type, should be one of "none", or "dynamic", default is "none"
  --max_batch_size MAX_BATCH_SIZE
                        batching max batch size, default is 16. Useless when batching_type is "none"
  --batch_timeout_us BATCH_TIMEOUT_US
                        timeout(us) to assemble batch, default is 1000. Useless when batching_type is "none"
  --max_connections MAX_CONNECTIONS
                        server max connections limit, default is 1000
  --max_concurrency MAX_CONCURRENCY
                        server max concurrency limit, default is 32
  --gpu_devices_idx GPU_DEVICES_IDX
                        devices idx(split by comma) that will be limited and monitored, cannot be none when "gpu_mem_limit_mib" is not -1, default is "0"
  --gpu_mem_limit_mib GPU_MEM_LIMIT_MIB
                        server gpu memory limit(MIB), default is -1, means no limit
  --customized_op_paths CUSTOMIZED_OP_PATHS
                        customized op paths split by comma, default is empty
  --log_dir LOG_DIR     log dir. will be subdir of deploy path if is relative path
  --log_backup_count LOG_BACKUP_COUNT
                        log backup count, default is 7
  --output_path OUTPUT_PATH
                        project will be archived to this path, default is empty and will not archive
  --timeout TIMEOUT     server start timeout, default is 300s
```

### 快速部署torch模型服务

```
usage: grpst torch_serve [-h] [--name NAME] [--interface_framework {http,http+grpc,http+brpc}] [--port PORT] [--customized_predict_http_path CUSTOMIZED_PREDICT_HTTP_PATH] [--device DEVICE]
                         [--inp_device INP_DEVICE] [--batching_type BATCHING_TYPE] [--max_batch_size MAX_BATCH_SIZE] [--batch_timeout_us BATCH_TIMEOUT_US] [--max_connections MAX_CONNECTIONS]
                         [--max_concurrency MAX_CONCURRENCY] [--gpu_devices_idx GPU_DEVICES_IDX] [--gpu_mem_limit_mib GPU_MEM_LIMIT_MIB] [--gpu_mem_gc_enable] [--gpu_mem_gc_interval GPU_MEM_GC_INTERVAL]
                         [--customized_op_paths CUSTOMIZED_OP_PATHS] [--log_dir LOG_DIR] [--log_backup_count LOG_BACKUP_COUNT] [--output_path OUTPUT_PATH] [--timeout TIMEOUT]
                         model_path

positional arguments:
  model_path            torch saved model path, only support torch script model format

optional arguments:
  -h, --help            show this help message and exit
  --name NAME           server name, default is "my_grps"
  --interface_framework {http,http+grpc,http+brpc}
                        interface framework, should be one of "http", "http+grpc", "http+brpc", default is "http+grpc"
  --port PORT           http port and rpc port split by comma, default is "7080,7081"
  --customized_predict_http_path CUSTOMIZED_PREDICT_HTTP_PATH
                        customized predict http path, default is empty and will not use default path
  --device DEVICE       device of model, should be one of "cpu", "gpu"(same as cuda:0), "cuda"(same as cuda:0), "cuda:[num]", "gpu:[num]" or "original"(original device specified when exported model),
                        default is "cuda"
  --inp_device INP_DEVICE
                        device of input, should be one of "cpu", "gpu"(same as cuda:0), "cuda"(same as cuda:0), "cuda:[num]" or "gpu:[num]". Cannot be none when "--device" is "original"
  --batching_type BATCHING_TYPE
                        batching type, should be one of "none", or "dynamic", default is "none"
  --max_batch_size MAX_BATCH_SIZE
                        batching max batch size, default is 16. Useless when batching_type is "none"
  --batch_timeout_us BATCH_TIMEOUT_US
                        timeout(us) to assemble batch, default is 1000. Useless when batching_type is "none"
  --max_connections MAX_CONNECTIONS
                        server max connections limit, default is 1000
  --max_concurrency MAX_CONCURRENCY
                        server max concurrency limit, default is 32
  --gpu_devices_idx GPU_DEVICES_IDX
                        devices idx(split by comma) that will be limited and monitored, cannot be none when "gpu_mem_limit_mib" is not -1, default is "0"
  --gpu_mem_limit_mib GPU_MEM_LIMIT_MIB
                        server gpu memory limit(MIB), default is -1, means no limit
  --gpu_mem_gc_enable   set this flag to enable server gpu memory gc, default not enable
  --gpu_mem_gc_interval GPU_MEM_GC_INTERVAL
                        server gpu memory gc interval, default is 60s
  --customized_op_paths CUSTOMIZED_OP_PATHS
                        customized op paths split by comma, default is empty
  --log_dir LOG_DIR     log dir. will be subdir of deploy path if is relative path
  --log_backup_count LOG_BACKUP_COUNT
                        log backup count, default is 7
  --output_path OUTPUT_PATH
                        project will be archived to this path, default is empty and will not archive
  --timeout TIMEOUT     server start timeout, default is 300s
```

### 快速部署tensorrt模型服务

```
usage: grpst trt_serve [-h] [--name NAME] [--interface_framework {http,http+grpc,http+brpc}] [--port PORT] [--customized_predict_http_path CUSTOMIZED_PREDICT_HTTP_PATH] [--device DEVICE]
                       [--streams STREAMS] [--batching_type BATCHING_TYPE] [--max_batch_size MAX_BATCH_SIZE] [--batch_timeout_us BATCH_TIMEOUT_US] [--max_connections MAX_CONNECTIONS]
                       [--max_concurrency MAX_CONCURRENCY] [--gpu_devices_idx GPU_DEVICES_IDX] [--customized_op_paths CUSTOMIZED_OP_PATHS] [--log_dir LOG_DIR] [--log_backup_count LOG_BACKUP_COUNT]
                       [--output_path OUTPUT_PATH] [--timeout TIMEOUT]
                       model_path

positional arguments:
  model_path            trt engine path.

optional arguments:
  -h, --help            show this help message and exit
  --name NAME           server name, default is "my_grps"
  --interface_framework {http,http+grpc,http+brpc}
                        interface framework, should be one of "http", "http+grpc", "http+brpc", default is "http+grpc"
  --port PORT           http port and rpc port split by comma, default is "7080,7081"
  --customized_predict_http_path CUSTOMIZED_PREDICT_HTTP_PATH
                        customized predict http path, default is empty and will not use default path
  --device DEVICE       device of model, should be one of "gpu"(same as cuda:0), "cuda"(same as cuda:0), "cuda:[num]", "gpu:[num]" or "original"(original device specified when exported model), default
                        is "cuda"
  --streams STREAMS     streams count of trt inferer that can be used for parallel inference, should be greater than 0, default is 1
  --batching_type BATCHING_TYPE
                        batching type, should be one of "none", or "dynamic", default is "none"
  --max_batch_size MAX_BATCH_SIZE
                        batching max batch size, default is 16. Useless when batching_type is "none", Should not be greater than the max batch size of the trt engine
  --batch_timeout_us BATCH_TIMEOUT_US
                        timeout(us) to assemble batch, default is 1000. Useless when batching_type is "none"
  --max_connections MAX_CONNECTIONS
                        server max connections limit, default is 1000
  --max_concurrency MAX_CONCURRENCY
                        server max concurrency limit, default is 32
  --gpu_devices_idx GPU_DEVICES_IDX
                        devices idx(split by comma) will be monitored, default is "0"
  --customized_op_paths CUSTOMIZED_OP_PATHS
                        customized op paths split by comma, default is empty
  --log_dir LOG_DIR     log dir. will be subdir of deploy path if is relative path
  --log_backup_count LOG_BACKUP_COUNT
                        log backup count, default is 7
  --output_path OUTPUT_PATH
                        project will be archived to this path, default is empty and will not archive
  --timeout TIMEOUT     server start timeout, default is 300s
```

### 查看状态

```
usage: grpst ps [-h] [-n NAME]

optional arguments:
  -h, --help            show this help message and exit
  -n NAME, --name NAME  server name
```

### 停止服务

```
usage: grpst stop [-h] server_name

positional arguments:
  server_name  server name

optional arguments:
  -h, --help   show this help message and exit
```

### 查看日志

```
usage: grpst logs [-h] server_name

positional arguments:
  server_name  server name

optional arguments:
  -h, --help   show this help message and exit
```
