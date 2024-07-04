# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/11/28
# Brief

from grps_framework.conf.conf import global_conf

interface_conf = global_conf.server_conf['interface']
bind = interface_conf['host'] + ':' + str(str(interface_conf['port']).replace(' ', '').split(',')[0])
worker_class = 'gthread'
threads = global_conf.server_conf['max_concurrency']
workers = 1
worker_connections = global_conf.server_conf['max_connections']
timeout = 0
keepalive = 0
