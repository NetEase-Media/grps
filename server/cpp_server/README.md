# grps c++ server

## 1. 项目结构

```
|-- conf                            # 配置文件
|   |-- inference.yml               # 推理配置
|   |-- server.yml                  # 服务配置
|-- data                            # 数据文件
|-- second_party                    # 第二方依赖
|   |-- grps_apis                   # 接口定义
|-- src                             # 源码
|   |-- batching                    # 批处理请求调度
|   |-- common                      # 公共库
|   |-- config                      # 配置加载
|   |-- context                     # 请求上下文
|   |-- converter                   # 前后处理转换器
|   |-- customized                  # 初始化用户自定义库
|   |-- dag                         # 推理dag
|   |-- executor                    # 执行器
|   |-- handler                     # 请求处理器
|   |-- logger                      # 日志系统
|   |-- mem_manager                 # 内存管理
|   |-- model                       # 模型定义
|   |-- model_infer                 # 模型推理器
|   |-- monitor                     # 监控功能
|   |-- service                     # 服务实现
|   |-- system_monitor              # 系统监控
|   |-- CMakelists.txt              # 构建文件
|   |-- constant.h                  # 常量定义
|   |-- main.cc                     # 服务入口
|-- test                            # 单元测试
|-- third_party                     # 第三方依赖
|-- build.sh                        # 构建脚本
|-- CMakelists.txt                  # 工程构建文件
|-- start_server.sh                 # 启动脚本
|-- stop_server.sh                  # 关闭脚本
```
