# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/10/17
# Brief  main
import os
import re
import sys
import threading
import traceback
from concurrent import futures
from constant import GRPS_VERSION

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Dump PID.
with open('PID', 'w') as f:
    # f.write(str(os.getpid()) + ' ' + str(os.getppid()))
    f.write(str(os.getpid()))
    f.flush()

# Dump VERSION.
with open('VERSION', 'w') as f:
    f.write(GRPS_VERSION)
    f.flush()

try:
    from grps_framework.conf.conf import global_conf
    from grps_framework.logger.logger import logger
    from grps_framework.constant import QPS, REQ_FAIL_RATE, REQ_LATENCY_CDF, REQ_LATENCY_AVG, REQ_LATENCY_MAX, \
        CPU_USAGE_AVG, MEM_USAGE_AVG
    from grps_framework.monitor.monitor import app_monitor
    from grps_framework.system_monitor.system_monitor import SystemMonitor
    from grps_framework.service.flask_app import GrpsFlaskApp
    from grps_framework.service.grpc_server import GrpsGrpcServer
    import src.customized_converter
    import src.customized_inferer
except Exception as e:
    trace_back = traceback.format_exc()
    logger.critical(trace_back)
    logger.critical('Start server failed: {}'.format(e))
    exit(-1)

# Check interface conf.
interface_conf = global_conf.server_conf.get('interface')
if interface_conf is None:
    logger.fatal('Interface conf not found.')
    exit(-1)

# framework.
framework = interface_conf.get('framework')
if framework is None:
    logger.critical('Framework not found.')
    exit(-1)

# host.
host = interface_conf.get('host')
if host is None:
    logger.critical('Host not found.')
    exit(-1)
if re.match(r'^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$', host) is None:
    logger.critical('Invalid host: {}'.format(host))
    exit(-1)

# port.
port_conf = interface_conf.get('port')
if not port_conf:
    logger.critical('Port not found.')
    exit(-1)
port_conf = str(port_conf).replace(' ', '')
ports = port_conf.split(',')
if framework == 'http' and len(ports) == 1:
    if not ports[0].isdigit():
        logger.critical('Invalid port: {}'.format(port_conf))
        exit(-1)
    http_port = int(ports[0])
    rpc_port = -1
elif framework == 'http+grpc' and len(ports) == 2:
    if not ports[0].isdigit() or not ports[1].isdigit():
        logger.critical('Invalid port: {}'.format(port_conf))
        exit(-1)
    http_port = int(ports[0])
    rpc_port = int(ports[1])
    if http_port == rpc_port:
        logger.critical('Http port and rpc port must be different.')
        exit(-1)
else:
    logger.critical('Invalid framework({}) or port({}).'.format(framework, port_conf))
    exit(-1)

# max connections.
max_connections = global_conf.server_conf.get('max_connections')
if max_connections is None or type(max_connections) is not int or max_connections <= 0:
    logger.critical('max_connections not found or invalid.')
    exit(-1)

# max_concurrency.
max_concurrency = global_conf.server_conf.get('max_concurrency')
if max_concurrency is None or type(max_concurrency) is not int or max_concurrency <= 0:
    logger.critical('max_concurrency not found or invalid.')
    exit(-1)

try:
    # Start global monitor.
    app_monitor.start()
    logger.info('Start global monitor success.')

    # Start system monitor.
    gpu_conf = global_conf.server_conf.get('gpu')
    system_monitor = SystemMonitor(gpu_conf, interval_step=0.98, stat_step=1)
    system_monitor.start()

    # Init executor.
    from grps_framework.executor.executor import Executor

    executor = Executor()

    # Init system metrics monitor.
    app_monitor.inc(QPS, 0)
    app_monitor.avg(REQ_FAIL_RATE, 0)
    app_monitor.cdf(REQ_LATENCY_CDF, 0)
    app_monitor.avg(REQ_LATENCY_AVG, 0)
    app_monitor.max(REQ_LATENCY_MAX, 0)
    app_monitor.avg(CPU_USAGE_AVG, 0)
    app_monitor.avg(MEM_USAGE_AVG, 0)

    # Start predict threadpool.
    tp = futures.ThreadPoolExecutor(max_workers=max_concurrency)

    # Start grpc server.
    flask_app = GrpsFlaskApp('grps', executor, host, http_port, tp, max_connections)
    if framework == 'http':
        flask_app.start()
    if framework == 'http+grpc':
        (threading.Thread(target=flask_app.start).start())
        GrpsGrpcServer('grps', executor, host, rpc_port, tp, max_connections).start()

except Exception as e:
    trace_back = traceback.format_exc()
    logger.critical(trace_back)
    logger.critical('Start server failed: {}'.format(e))
    exit(-1)
