# Copyright 2022 netease. All rights reserved.
# Author jiangwenxuan01@corp.netease.com
# Date   2023/9/25
# Brief  system monitor
import os
import re
import resource
import threading
import time
import traceback
from pynvml import *
from grps_framework.logger.logger import logger
from grps_framework.monitor.monitor import app_monitor
from grps_framework.constant import CPU_USAGE_AVG, MEM_USAGE_AVG, MIB


def get_total_cpu_time():
    """Get total cpu time."""
    with open('/proc/stat', 'r') as fp:
        buf = fp.readline()
        user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice = map(int, buf.split()[1:])
    # do not include guest and guest_nice since they are already included in user and nice
    return user + nice + system + idle + iowait + irq + softirq + steal


def get_pid_info(pid):
    proc = {}
    path = f'/proc/{pid}/stat'
    with open(path, 'r') as fp:
        buf = fp.readline()
        fields = buf.split()
        proc['utime'] = int(fields[13])
        proc['stime'] = int(fields[14])
        proc['vm_rss'] = int(fields[23]) * (resource.getpagesize() / 1024)  # convert from pages to kB
    return proc


class SystemMonitor(object):
    def __init__(self, gpu_conf, interval_step=1, stat_step=1):
        """
        Init system monitor.
        Args:
            interval_step: Interval(second) of per step.
            stat_step: System statistic interval steps count. That is to say, the statistic interval is
            interval_step * stat_step.
            gc_steps: System garbage collection interval steps count. That is to say, the gc interval is
            interval_step * gc_steps. 0 means no gc.
        """
        self.__interval_step = interval_step
        self.__stat_step = stat_step
        self.__gpu_conf = gpu_conf
        self.__gc_steps = 0
        self.__mem_manager_enable = False
        if gpu_conf is not None:
            cuda_visible_devices = os.environ.get('CUDA_VISIBLE_DEVICES')
            if cuda_visible_devices is not None:
                if not re.match(r'^\d+(,\d+)*$', cuda_visible_devices):
                    raise ValueError('Invalid CUDA_VISIBLE_DEVICES: {}'.format(cuda_visible_devices))
                self.__cuda_visible_devices = cuda_visible_devices.split(',')
                self.__cuda_visible_devices = [int(i) for i in self.__cuda_visible_devices]
            else:
                self.__cuda_visible_devices = None

            self.__devices = gpu_conf.get('devices')
            gpu_mem_manager_type = gpu_conf.get('mem_manager_type')
            if not gpu_mem_manager_type or type(gpu_mem_manager_type) is not str:
                raise ValueError('Invalid gpu memory manager type: {}'.format(gpu_mem_manager_type))
            self.__devices = gpu_conf.get('devices')
            if not self.__devices or type(self.__devices) is not list:
                raise ValueError('Invalid server gpu devices: {}, should be int list.'.format(self.__devices))
            for device in self.__devices:
                if type(device) is not int:
                    raise ValueError('Invalid server gpu devices: {}, should be int list.'.format(self.__devices))

            if gpu_mem_manager_type == 'torch':
                self.__mem_manager_enable = True
                from grps_framework.mem_manager.torch_gpu_mem_mgr import TorchGpuMemManager
                self.__gpu_mem_manager = TorchGpuMemManager(self.__devices)
            elif gpu_mem_manager_type == 'tensorflow':
                self.__mem_manager_enable = True
                from grps_framework.mem_manager.tf_gpu_mem_mgr import TfGpuMemManager
                self.__gpu_mem_manager = TfGpuMemManager(self.__devices)
            elif gpu_mem_manager_type == 'none':
                self.__mem_manager_enable = False
            else:
                raise ValueError('Gpu memory manager type {} not supported.'.format(gpu_mem_manager_type))

            if self.__mem_manager_enable:
                mem_gc_enable = gpu_conf.get('mem_gc_enable')
                if mem_gc_enable is True:
                    mem_gc_interval = gpu_conf.get('mem_gc_interval')
                    if mem_gc_interval is None or type(mem_gc_interval) is not int or mem_gc_interval < 1:
                        raise ValueError(
                            'Invalid gpu memory gc interval: {}, should be int and not less than 1.'.format(
                                mem_gc_interval))
                    self.__gc_steps = mem_gc_interval // self.__stat_step

                gpu_mem_limit_mib = gpu_conf.get('mem_limit_mib')
                if not gpu_mem_limit_mib or type(gpu_mem_limit_mib) is not int or (
                        gpu_mem_limit_mib <= 0 and gpu_mem_limit_mib != -1):
                    raise ValueError('Invalid gpu memory limit: {}, should be int and greater than 0 or equal to -1 '
                                     '(not limit).'.format(gpu_mem_limit_mib))
                self.__gpu_mem_manager.set_mem_limit(gpu_mem_limit_mib)
                logger.info('Gpu memory monitor init, interval_step: {}, stat_step: {}, gc_enable: {}, gc_steps: {},'
                            ' mem_limit_mib: {}, gpu_mem_manager_type: {}.'.format(interval_step, stat_step,
                                                                                   mem_gc_enable,
                                                                                   self.__gc_steps,
                                                                                   gpu_mem_limit_mib,
                                                                                   gpu_mem_manager_type))
            else:
                logger.info(
                    'Gpu memory monitor init, interval_step: {}, stat_step: {}, gpu_mem_manager_type: {}.'.format(
                        interval_step, stat_step, gpu_mem_manager_type))

            nvmlInit()
            for i in self.__devices:
                usage = self.__get_gpu_usage(i)
                app_monitor.avg(f'*gpu{i}_usage(%)', usage)
                mem_usage = self.__get_gpu_mem_usage(i)
                app_monitor.avg(f'*gpu{i}_mem_usage(MIB)', mem_usage)
            nvmlShutdown()

    def __get_gpu_usage(self, gpu_idx):
        """Get gpu usage."""
        if self.__cuda_visible_devices:
            gpu_idx = self.__cuda_visible_devices[gpu_idx]

        device_count = nvmlDeviceGetCount()
        if gpu_idx >= device_count:
            # raise ValueError('Invalid gpu index: {}, device count: {}.'.format(gpu_idx, device_count))
            return 0
        handle = nvmlDeviceGetHandleByIndex(gpu_idx)
        usage = nvmlDeviceGetUtilizationRates(handle).gpu
        return usage

    def __get_gpu_mem_usage(self, gpu_idx):
        """Get gpu memory usage."""
        if self.__cuda_visible_devices:
            gpu_idx = self.__cuda_visible_devices[gpu_idx]

        device_count = nvmlDeviceGetCount()
        if gpu_idx >= device_count:
            # raise ValueError('Invalid gpu index: {}, device count: {}.'.format(gpu_idx, device_count))
            return 0
        handle = nvmlDeviceGetHandleByIndex(gpu_idx)
        mem_info = nvmlDeviceGetMemoryInfo(handle)
        return mem_info.used / MIB

    def __system_monitor_func(self):
        logger.info('System monitor started.')
        step = 1
        pid = os.getpid()
        prev_total_cpu_time = 0
        curr_total_cpu_time = 0
        prev_proc = {}
        curr_proc = {}
        nvmlInit()
        while True:
            begin = time.time()
            try:
                if step % self.__stat_step == 0:
                    # cpu info stat
                    # cpu usage
                    curr_total_cpu_time = get_total_cpu_time()
                    curr_proc = get_pid_info(pid)
                    if prev_total_cpu_time != 0 and prev_proc != {}:  # not first time
                        cpu_time = curr_proc['utime'] + curr_proc['stime'] - prev_proc['utime'] - prev_proc['stime']
                        num_cpus = os.cpu_count()
                        cpu_usage = float(cpu_time) / float(curr_total_cpu_time - prev_total_cpu_time) * float(
                            num_cpus) * 100.0
                        app_monitor.avg(CPU_USAGE_AVG, cpu_usage)
                    prev_total_cpu_time = curr_total_cpu_time
                    prev_proc = curr_proc
                    # cpu mem
                    with open('/proc/meminfo', 'r') as fp:
                        buf = fp.readline()
                        mem_total = int(buf.split()[1])
                    mem_usage = curr_proc['vm_rss'] / mem_total * 100.0
                    app_monitor.avg(MEM_USAGE_AVG, mem_usage)

                    if self.__gpu_conf is not None:
                        # gpu info stat
                        for i in self.__devices:
                            usage = self.__get_gpu_usage(i)
                            app_monitor.avg(f'*gpu{i}_usage(%)', usage)
                            mem_usage = self.__get_gpu_mem_usage(i)
                            app_monitor.avg(f'*gpu{i}_mem_usage(MIB)', mem_usage)

                        # gpu mem gc
                        if self.__mem_manager_enable:
                            if self.__gc_steps > 0 and step % self.__gc_steps == 0:
                                self.__gpu_mem_manager.gpu_mem_gc()
            except Exception:
                trace_back = traceback.format_exc()
                logger.error(trace_back)
            end = time.time()
            true_interval = max(self.__interval_step - (end - begin), 0)
            time.sleep(true_interval)
            step += 1

    def start(self):
        """Start system monitor."""
        logger.info('Start system monitor.')
        p = threading.Thread(target=self.__system_monitor_func)
        p.setDaemon(True)
        p.start()
