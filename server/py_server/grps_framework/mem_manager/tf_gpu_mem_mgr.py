# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/14
# Brief  Tensorflow gpu memory manager.

import tensorflow as tf

from grps_framework.logger.logger import logger
from grps_framework.mem_manager.gpu_mem_mgr import GpuMemManager


class TfGpuMemManager(GpuMemManager):
    def __init__(self, devices):
        """Init gpu memory manager."""
        super().__init__(devices)
        physical_devices = tf.config.list_physical_devices('GPU')
        if not physical_devices:
            raise RuntimeError('No gpu device found. Cannot use gpu memory manager.')
        logger.info('Init tensorflow gpu memory manager, version: {}.'.format(tf.__version__))

        self._devices = devices
        for i in self._devices:
            if i >= len(physical_devices) or i < 0:
                raise RuntimeError('Invalid gpu device index: {}, device count: {}.'.format(i, len(physical_devices)))
        logger.info('Init tensorflow gpu memory manager success, devices: {}.'.format(self._devices))

    def get_mem_usage(self):
        """
        Get gpu memory usage.
        Returns:
            Gpu memory usage with MiB of all devices.
        """
        # TODO: Tf do not support now. Do nothing.
        return []

    def set_mem_limit(self, limit):
        """
        Set gpu memory limit.
        Args:
            limit: gpu memory limit threshold with MiB.

        Returns:
            True for success, False for failure.
        """
        if limit == -1:
            logger.info('Gpu memory limit is -1, will not limit gpu memory.')
            return True

        physical_devices = tf.config.list_physical_devices('GPU')
        for i in self._devices:
            logger.info('Setting gpu{} memory limit, limit: {}MiB...'.format(i, limit))
            tf.config.experimental.set_memory_growth(physical_devices[i], True)
            tf.config.experimental.set_virtual_device_configuration(physical_devices[i], [
                tf.config.experimental.VirtualDeviceConfiguration(memory_limit=limit)])
        return True

    def gpu_mem_gc(self):
        """
        Gpu memory garbage collection.
        Returns:
            True for success, False for failure.
        """
        # TODO: Tf do not support now. Do nothing.
        return True
