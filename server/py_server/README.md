# grps py server

## 1. 项目结构

```
|-- conf                            # 配置文件
|-- data                            # 数据文件
|   |-- inference.yml               # 推理配置
|   |-- server.yml                  # 服务配置
|-- grps_framework                  # grps框架
|   |-- apis                        # 接口
|   |-- batching                    # 批处理请求调度
|   |-- conf                        # 配置加载
|   |-- context                     # 请求上下文
|   |-- converters                  # 前后处理转换器
|   |-- dag                         # 推理dag
|   |-- executor                    # 执行器
|   |-- handler                     # 请求处理器
|   |-- logger                      # 日志系统
|   |-- mem_manager                 # 内存管理
|   |-- model_infer                 # 模型推理器
|   |-- monitor                     # 监控功能
|   |-- service                     # 服务实现
|   |-- system_monitor              # 系统监控
|   |-- constant.py                 # 常量
|   |-- main.py                     # 服务入口
|   |-- test.py                     # grps本地单元测试
|-- src
|   |-- customized_converter.py     # 自定义前后处理转换器
|   |-- customized_inferer.py       # 自定义推理器
|-- test                            # 单元测试
|-- requirements.txt                # 依赖
|-- setup.py                        # grps_framework打包
|-- start_server.sh                 # 启动脚本
|-- stop_server.sh                  # 关闭脚本
```