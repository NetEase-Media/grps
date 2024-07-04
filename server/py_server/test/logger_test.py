# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  logger module unit test.
import os
import sys
import unittest

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from grps_framework.logger.logger import logger, clogger
from grps_framework.constant import SERVER_LOG_DIR, SERVER_LOG_NAME, USR_LOG_NAME


class MyTestCase(unittest.TestCase):
    def test_server_logger(self):
        logger.info('info')
        logger.warning('warning')
        logger.error('error')
        logger.critical('critical')
        logger.fatal('fatal')
        logger.exception('exception')
        self.assertTrue(os.path.exists(os.path.join(SERVER_LOG_DIR, SERVER_LOG_NAME)))

    def test_user_logger(self):
        clogger.info('info')
        clogger.warning('warning')
        clogger.error('error')
        clogger.critical('critical')
        clogger.fatal('fatal')
        clogger.exception('exception')
        self.assertTrue(os.path.exists(os.path.join(SERVER_LOG_DIR, USR_LOG_NAME)))


if __name__ == '__main__':
    unittest.main()
