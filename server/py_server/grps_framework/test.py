# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/10/17
# Brief  unit test.
import re
import unittest

from .conf.conf import global_conf
from .system_monitor.system_monitor import SystemMonitor


class GrpsTest(unittest.TestCase):
    executor = None

    def test_init(self):
        # check interface.
        interface_conf = global_conf.server_conf.get('interface')
        self.assertIsNotNone(interface_conf, 'Interface conf not found.')

        # check framework.
        framework = interface_conf.get('framework')
        self.assertIsNotNone(framework, 'Interface framework conf not found.')

        # check host.
        host = interface_conf.get('host', 'Interface host conf not found.')
        self.assertIsNotNone(host)
        self.assertTrue(re.match(r'^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$', host),
                        'Invalid interface host: {}'.format(host))

        # check port.
        port_conf = interface_conf.get('port')
        self.assertIsNotNone(port_conf, 'Interface port conf not found.')
        port_conf = str(port_conf).replace(' ', '')
        ports = port_conf.split(',')
        self.assertTrue((framework == 'http' and len(ports) == 1 and ports[0].isdigit()) or
                        (framework == 'http+grpc' and len(ports) == 2 and ports[0].isdigit() and ports[1].isdigit()),
                        'Invalid framework({}) or port({}).'.format(framework, port_conf))

        # check max_concurrency.
        max_concurrency = global_conf.server_conf.get('max_concurrency')
        self.assertTrue(max_concurrency is not None and type(max_concurrency) is int and max_concurrency > 0,
                        'Invalid max_concurrency: {}.'.format(max_concurrency))

        # Init system monitor.
        gpu_conf = global_conf.server_conf.get('gpu')
        SystemMonitor(gpu_conf, interval_step=1, stat_step=1)

        # Build executor.
        from grps_framework.executor.executor import Executor

        self.executor = Executor()
        self.assertIsNotNone(self.executor)
