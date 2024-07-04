# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/14
# Brief  Torch gpu memory manager.
import time

import torch

from grps_framework.constant import MIB
from grps_framework.logger.logger import logger
from grps_framework.mem_manager.gpu_mem_mgr import GpuMemManager


class TorchGpuMemManager(GpuMemManager):
    def __init__(self, devices):
        """Init gpu memory manager."""
        super().__init__(devices)
        if torch.__version__ < '1.8.0':
            logger.warning(
                'Pytorch version is less than 1.8, gpu memory manager may not work properly.')
        if not torch.cuda.is_available():
            raise RuntimeError('No gpu device found. Cannot use gpu memory manager.')
        logger.info('Init torch gpu memory manager, version: {}.'.format(torch.__version__))

        gpu_count = torch.cuda.device_count()
        for i in self._devices:
            if i >= gpu_count or i < 0:
                raise RuntimeError('Invalid gpu device index: {}, device count: {}.'.format(i, gpu_count))
        logger.info('Init torch gpu memory manager success, devices: {}.'.format(self._devices))

    def get_mem_usage(self):
        """
        Get gpu memory usage.
        Returns:
            Gpu memory usage with MiB.
        """
        if torch.__version__ < '1.8.0':
            logger.warning('Pytorch version is less than 1.8, cannot get gpu memory usage.')
            return []
        mem_usage = []
        for i in self._devices:
            mem_occupied = torch.cuda.memory_allocated(i) / MIB
            mem_cache = torch.cuda.memory_reserved(i) / MIB
            mem_total = mem_occupied + mem_cache
            mem_usage.append(mem_total)
        return mem_usage

    def set_mem_limit(self, limit):
        """
        Set gpu memory limit.
        Args:
            limit: gpu memory limit threshold with MiB.

        Returns:
            True for success, False for failure.

        """
        if torch.__version__ < '1.8.0':
            logger.warning('Pytorch version is less than 1.8, cannot set gpu memory limit.')
            return False

        if limit == -1:
            logger.info('Gpu memory limit is -1, will not limit gpu memory.')
            return True

        for i in self._devices:
            mem_total = torch.cuda.get_device_properties(i).total_memory / MIB
            limit_ratio = limit / mem_total
            if limit_ratio > 1:
                limit_ratio = 1
            logger.info('Set gpu{} memory limit, mem_total: {} MiB , limit: {}MiB, limit_ratio: {}...'
                        .format(i, mem_total, limit, limit_ratio))
            torch.cuda.set_per_process_memory_fraction(limit_ratio, i)
        return True

    def gpu_mem_gc(self):
        """
        Gpu memory garbage collection.
        Returns:
            True for success, False for failure.
        """
        if torch.__version__ < '1.8.0':
            logger.warning('Pytorch version is less than 1.8, cannot do gpu memory gc.')
            return False
        begin_time = time.time()
        torch.cuda.empty_cache()
        logger.info('Gpu memory garbage collection, cost: {} ms'.format((time.time() - begin_time) * 1000))
        return True
