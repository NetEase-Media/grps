# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2024/5/7
# Brief  Batching for inference.
import asyncio
import threading
import time
import traceback
from abc import ABCMeta, abstractmethod
from concurrent.futures import Future
from concurrent.futures import ThreadPoolExecutor
from queue import Empty, Queue

from grps_framework.apis.grps_pb2 import GrpsMessage
from grps_framework.conf.conf import global_conf
from grps_framework.context.context import GrpsContext
from grps_framework.logger.logger import logger


class Batcher(metaclass=ABCMeta):
    class Task:
        def __init__(self, inp, out, ctx):
            self.inp = inp
            self.out = out
            self.ctx = ctx

    def __init__(self):
        self._name = None
        self._max_batch_size = None
        self._batch_timeout_us = None
        self._converter = None
        self._inferer = None
        self._running = False

    @abstractmethod
    def init(self, name, max_batch_size, batch_timeout_us, converter, inferer):
        self._name = name
        self._max_batch_size = max_batch_size
        self._batch_timeout_us = batch_timeout_us
        self._converter = converter
        self._inferer = inferer

    @abstractmethod
    def start(self):
        pass

    @abstractmethod
    def infer(self, inp: GrpsMessage, ctx: GrpsContext) -> GrpsMessage:
        """ Infer in batching.

        Args:
            inp: Input message from client.
            ctx: Output message to client.

        Returns:
            Output message to client.

        Raises:
            Exception: If infer failed, can raise exception and exception will be caught by server and return error
            message to client.
        """
        pass

    @abstractmethod
    def stop(self):
        pass


class DynamicBatcher(Batcher):
    def __init__(self):
        super().__init__()
        self.__task_queue = None
        self.__queue_lock = None
        self.__queue_cv = None
        self.__schedule_thread = None
        self.__worker_tp = None

    def init(self, name, max_batch_size, batch_timeout_us, converter, inferer):
        super().init(name, max_batch_size, batch_timeout_us, converter, inferer)
        self.__task_queue = Queue()
        self.__queue_lock = threading.Lock()
        self.__queue_cv = threading.Condition(self.__queue_lock)
        self.__worker_tp = ThreadPoolExecutor(max_workers=global_conf.server_conf.get('max_concurrency'))
        logger.info(
            f"DynamicBatcher ({name}) init, max_batch_size: {max_batch_size}, batch_timeout_us: {batch_timeout_us}")

    @staticmethod
    def __all_err(ctxs):
        all_err = True
        for ctx in ctxs:
            if not ctx.has_err():
                all_err = False
                break
        return all_err

    def __check_and_notify(self, ctxs):
        if self.__all_err(ctxs):
            for ctx in ctxs:
                ctx.batcher_future_notify()
            return True
        return False

    def __batch_infer_process(self, tasks):
        inps = [task.inp for task in tasks]
        ctxs = [task.ctx for task in tasks]
        try:
            if self._converter is None:
                begin = time.time()
                infer_input = inps
                infer_out = self._inferer.batch_infer(infer_input, ctxs)
                if self.__check_and_notify(ctxs):
                    return
                outs = infer_out
                logger.info(
                    'DynamicBather({}), batch_size: {}, model_infer time: {:.0f} us'
                    .format(self._name, len(inps), (time.time() - begin) * 1e6))
            else:
                begin = time.time()
                infer_input = self._converter.batch_preprocess(inps, ctxs)
                if self.__check_and_notify(ctxs):
                    return
                preprocess_time = time.time()
                infer_out = self._inferer.batch_infer(infer_input, ctxs)
                if self.__check_and_notify(ctxs):
                    return
                infer_time = time.time()
                outs = self._converter.batch_postprocess(infer_out, ctxs)
                if self.__check_and_notify(ctxs):
                    return
                postprocess_time = time.time()
                logger.info(
                    'DynamicBather({}), batch_size: {}, preprocess time: {:.0f} us, model_infer time: {:.0f} us,'
                    ' postprocess time: {:.0f} us'.format(self._name, len(inps), (preprocess_time - begin) * 1e6,
                                                          (infer_time - preprocess_time) * 1e6,
                                                          (postprocess_time - infer_time) * 1e6))
            for i, out in enumerate(outs):
                tasks[i].out = out

            for ctx in ctxs:
                ctx.batcher_future_notify()
        except Exception as e:
            trace_back = traceback.format_exc()
            logger.error(f"DynamicBatcher({self._name}) batch inference process failed, error: {trace_back}")
            for ctx in ctxs:
                ctx.set_err_msg(trace_back)
                ctx.batcher_future_notify()

    def __schedule(self):
        while self._running:
            tasks = []
            with self.__queue_lock:
                self.__queue_cv.wait_for((lambda: self.__task_queue.qsize() >= 1) or (not self._running))
                if not self._running:
                    return
                try:
                    while True:
                        tasks.append(self.__task_queue.get_nowait())
                except Empty:
                    pass

                end_time = time.time_ns() + self._batch_timeout_us * 1000
                while len(tasks) < self._max_batch_size:
                    cur_time = time.time_ns()
                    if cur_time >= end_time:
                        break
                    self.__queue_cv.wait_for((lambda: self.__task_queue.qsize() >= 1) or (not self._running),
                                             timeout=(end_time - cur_time) / 1e9)
                    if not self._running:
                        return
                    try:
                        while len(tasks) < self._max_batch_size:
                            tasks.append(self.__task_queue.get_nowait())
                    except Empty:
                        pass

            if len(tasks) == 0:
                continue

            self.__worker_tp.submit(self.__batch_infer_process, tasks)

    def start(self):
        self._running = True
        self.__schedule_thread = threading.Thread(target=self.__schedule)
        self.__schedule_thread.setDaemon(True)
        self.__schedule_thread.start()

    def infer(self, inp: GrpsMessage, ctx: GrpsContext) -> GrpsMessage:
        """ Infer in batching.

        Args:
            inp: Input message from client.
            ctx: Output message to client.

        Returns:
            Output message to client.

        Raises:
            Exception: If infer failed, can raise exception and exception will be caught by server and return error
            message to client.
        """
        try:
            asyncio.get_event_loop()
        except RuntimeError:
            loop = asyncio.new_event_loop()
            asyncio.set_event_loop(loop)

        future = Future()
        ctx.set_batcher_future(future)
        task = Batcher.Task(inp, None, ctx)
        with self.__queue_lock:
            self.__task_queue.put(task)
            self.__queue_cv.notify()
        future.result()
        return task.out

    def stop(self):
        logger.info('DynamicBatcher({}) stop'.format(self._name))
        self._running = False
        self.__queue_cv.notify()

    def __del__(self):
        self.stop()
