# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  To save some constant variables.

# Version.
GRPS_MAJOR = 1
GRPS_MINOR = 1
GRPS_PATCH = 0
GRPS_VERSION = 'v' + str(GRPS_MAJOR) + '.' + str(GRPS_MINOR) + '.' + str(GRPS_PATCH)

# Interface.
API_VERSION = 'v1'
URL_ROOT_PATH = '/grps/' + API_VERSION

# inference conf path
INFERENCE_CONF_PATH = './conf/inference.yml'

# Supported interface framework type lists.
SUPPORTED_FRAMEWORK_LIST = ['http', 'http+grpc']

# server conf path.
SERVER_CONF_PATH = './conf/server.yml'

# log path.
SERVER_LOG_DIR = 'logs'  # Server log dir.
SERVER_LOG_NAME = 'grps_server.log'
USR_LOG_NAME = 'grps_usr.log'
MONITOR_LOG_NAME = 'grps_monitor.log'

# MiB
MIB = 1024 * 1024

# Metrics name

QPS = '*qps'
REQ_FAIL_RATE = '*fail_rate(%)'
REQ_LATENCY_AVG = '*latency_avg(ms)'
REQ_LATENCY_MAX = '*latency_max(ms)'
REQ_LATENCY_CDF = '*latency_cdf(ms)'
GPU_OOM_COUNT = '*gpu_oom_count'
CPU_USAGE_AVG = '*cpu_usage(%)'
MEM_USAGE_AVG = '*mem_usage(%)'

# grpc set max size
GRPC_MAX_MESSAGE_LENGTH = 1024 * 1024 * 1024
