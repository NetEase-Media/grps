# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  logger module.
# Usage:
# from grps_framework.logger.logger import logger, clogger
# # server logger.
# logger.info('')  # info log.
# logger.clogger('')  # warning log.
# logger.err('')  # error log.
# logger.crit('')  # critical log
# # user logger.
# clogger.info('')  # info log.
# clogger.clogger('')  # warning log.
# clogger.err('')  # error log.
# clogger.crit('')  # critical log


import logging
import os.path
from logging import handlers

from grps_framework.conf.conf import global_conf
from grps_framework.constant import SERVER_LOG_DIR, SERVER_LOG_NAME, USR_LOG_NAME

log = logging.getLogger('werkzeug')
log.setLevel(logging.WARNING)  # Set flask framework log level.


class TimeRotatingFileLogger(object):
    """
    Time rotating file logger.
    Attributes:
        logger: logger object.
    """
    __level_relations = {
        'debug': logging.DEBUG,
        'info': logging.INFO,
        'warning': logging.WARNING,
        'error': logging.ERROR,
        'crit': logging.CRITICAL
    }

    def __init__(self, filename, level='info', when='D', backup_count=14,
                 fmt='%(asctime)s.%(msecs)03d %(threadName)s %(filename)s:%(lineno)d %(levelname)s] %(message)s'):
        self.logger = logging.getLogger(filename)
        format_str = logging.Formatter(fmt, datefmt='%Y%m%d %H:%M:%S')
        self.logger.setLevel(self.__level_relations.get(level))
        th = handlers.TimedRotatingFileHandler(filename=filename, when=when, backupCount=backup_count,
                                               encoding='utf-8')
        th.setFormatter(format_str)
        self.logger.addHandler(th)
        self.logger.propagate = False  # Don't log to console.


def get_logger(log_dir, log_name, backup_count=7):
    if not os.path.exists(log_dir):
        os.mkdir(log_dir)
    return TimeRotatingFileLogger(os.path.join(log_dir, log_name), level='info', backup_count=backup_count).logger


logger, clogger = None, None
if global_conf.loaded:
    log_backup_count = global_conf.server_conf['log'].get('log_backup_count', 7)  # Log backup count.
    if not isinstance(log_backup_count, int):
        print('log_backup_count must be int, log_backup_count: {}.'.format(log_backup_count))
        exit(-1)
    if log_backup_count < 1:
        print(
            'log_backup_count must not be less than 1, log_backup_count: {}.'.format(log_backup_count))
        exit(-1)

    log_dir = global_conf.server_conf['log']['log_dir']  # Log dir.
    logger = get_logger(log_dir, SERVER_LOG_NAME, log_backup_count)  # Server logger.
    clogger = get_logger(log_dir, USR_LOG_NAME, log_backup_count)  # User logger.
    logger.info('Daily logger initialized, sys_log_path: {}, user_log_path: {}, user_log_backup_count: {}.'.format(
        SERVER_LOG_DIR + '/' + SERVER_LOG_NAME, log_dir + '/' + USR_LOG_NAME, log_backup_count))
