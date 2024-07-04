# 日志系统

grps服务日志包含两部分，一部分是服务日志，另一部分是用户日志，用户日志是指用户在自定义工程加入的自己的日志。grps日志是按天进行轮转，用户可以指定备份的数量，也可以指定日志路径。

1. [快捷部署](#快捷部署)
2. [自定义模型工程](#自定义模型工程)
3. [日志查看](#日志查看)

## 快捷部署

通过```grpst tf_serve/torch_serve```的参数实现，如下参数：

```bash
--log_dir ./log  # 设置日志目录，如果是相对路径，则路径在模型部署路径下，使用grpst ps命令查看部署路径。
--log_backup_count 7  # 设置日志备份数量，按天进行备份，超过数量后，会自动清理
```

## 自定义模型工程

### 日志配置

grps日志配置在```server.yml``，如下：

```yaml
# Log config.
log:
  log_dir: ./logs # Log dir. Will be subdir of deploy path if is relative path.
  log_backup_count: 7 # Number of log files to keep. One log file per day.
```

日志路径可以是相对路径，也可以是绝对路径，如果是相对路径，则路径在模型部署路径下，使用```grpst ps```命令查看部署路径。

### api使用

用户可以在自定义工程中调用日志api，如下：

```python
from grps_framework.logger.logger import clogger

clogger.info('')  # info log.
clogger.clogger('')  # warning log.
clogger.err('')  # error log.
clogger.crit('')  # critical log
```

```c++
#include "logger/logger.h"

// customer logger.
CLOG4(INFO, "a = " << 1);
CLOG4(WARN, "a = " << 1);
CLOG4(ERROR, "a = " << 1);
CLOG4(FATAL, "a = " << 1);
```

## 日志查看

在日志目录中有如下几份日志文件：

* grps_server.log：服务日志。
* grps_usr.log：用户自定义日志。
* grps_monitor.log：服务监控日志，记录服务指标收集信息，如cpu、gpu、mem、qps等信息，每秒刷新一次。更详细内容见[服务指标监控说明](10_Monitor.md)。