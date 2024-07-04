# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  The gpu memory manager base class define.

from abc import ABCMeta, abstractmethod


class GpuMemManager(metaclass=ABCMeta):
    """Gpu memory manager base class."""

    @abstractmethod
    def __init__(self, devices):
        """Init gpu memory manager."""
        self._devices = devices  # device index list
        pass

    def get_devices(self):
        """
        Get gpu devices.
        Returns:
            Gpu devices.
        """
        return self._devices

    @abstractmethod
    def get_mem_usage(self):
        """
        Get gpu memory usage.
        Returns:
            Gpu memory usage with MiB of all devices.
        """
        pass

    @abstractmethod
    def set_mem_limit(self, limit):
        """
        Set gpu memory limit.
        Args:
            limit: gpu memory limit threshold with MiB.

        Returns:
            True for success, False for failure.
        """
        pass

    @abstractmethod
    def gpu_mem_gc(self):
        """
        Gpu memory garbage collection.
        Returns:
            True for success, False for failure.
        """
        pass
