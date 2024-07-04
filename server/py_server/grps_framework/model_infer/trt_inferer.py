# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2024/6/27
# Brief  Tensorrt deep learning model inferer. Including model load and model infer.
import ctypes
import os
import re
import threading
import time
from queue import Empty, Queue

import numpy as np
import pycuda.driver as cuda
import tensorrt as trt

from grps_framework.apis.grps_pb2 import GrpsMessage
from grps_framework.converter.trt_tensor_converter import TrtTensorConverter
from grps_framework.logger.logger import logger
from grps_framework.model_infer.inferer import ModelInferer

g_trt_inferer_dbg = False


class TrtModelInferer(ModelInferer):
    """Tensorrt model(tensorrt engine format) infer class."""

    class TrtDeviceBinding:
        def __init__(self, name, shape, max_shape, data_type, vec_dim=-1, comps=-1, is_input_binding=False,
                     is_shape_binding=False):
            self._name = name
            self._shape = shape
            self._max_shape = max_shape
            self._true_shape = shape
            self._data_type = data_type
            self._vec_dim = vec_dim
            self._comps = comps
            self._is_input_binding = is_input_binding
            self._is_shape_binding = is_shape_binding
            self._volume = 0
            self._buffer = None
            self._buffer_size = 0
            self._buffer_capacity = 0

        @property
        def name(self):
            return self._name

        @property
        def shape(self):
            return self._shape

        @property
        def max_shape(self):
            return self._max_shape

        @property
        def true_shape(self):
            return self._true_shape

        @property
        def data_type(self):
            return self._data_type

        @property
        def vec_dim(self):
            return self._vec_dim

        @property
        def comps(self):
            return self._comps

        @property
        def is_input_binding(self):
            return self._is_input_binding

        @property
        def is_shape_binding(self):
            return self._is_shape_binding

        @property
        def volume(self):
            return self._volume

        @property
        def buffer(self):
            return self._buffer

        @property
        def buffer_size(self):
            return self._buffer_size

        @property
        def buffer_capacity(self):
            return self._buffer_capacity

        @staticmethod
        def _round_up(m, n):
            return ((m + n - 1) // n) * n

        def _calc_volume(self, shape, vec_dim, comps):
            if vec_dim != -1:
                shape[vec_dim] = self._round_up(shape[vec_dim], comps)
            return trt.volume(shape)

        def allocate(self, true_shape):
            self._true_shape = true_shape
            self._volume = self._calc_volume(true_shape, self._vec_dim, self._comps)
            new_buffer_size = self._volume * self._data_type.itemsize
            if new_buffer_size > self._buffer_capacity:
                self._buffer = cuda.mem_alloc(new_buffer_size)
                self._buffer_capacity = new_buffer_size
            self._buffer_size = new_buffer_size

        def from_host(self, host_np, stream):
            # Check shape.
            if len(host_np.shape) != len(self._shape):
                raise ValueError('Shape not match, binding name: {}, host shape: {}, binding shape: {}.'.format(
                    self._name, host_np.shape, self._shape))
            for i in range(len(self._shape)):
                if self._shape[i] != -1 and host_np.shape[i] != self._shape[i]:
                    raise ValueError('Shape not match, binding name: {}, host shape: {}, binding shape: {}.'.format(
                        self._name, host_np.shape, self._shape))
                elif host_np.shape[i] > self._max_shape[i]:
                    raise ValueError('Shape not match, binding name: {}, host shape: {}, binding max shape: {}.'.format(
                        self._name, host_np.shape, self._max_shape))

            # Check dtype.
            if host_np.dtype != trt.nptype(self._data_type):
                raise ValueError('Data type not match, binding name: {}, host dtype: {}, binding dtype: {}.'.format(
                    self._name, host_np.dtype, self._data_type))

            # Allocate buffer if not enough.
            self.allocate(host_np.shape)

            # Copy host buffer to cuda buffer.
            host_np = np.ascontiguousarray(host_np)
            cuda.memcpy_htod_async(self._buffer, host_np, stream)

        def to_host(self, stream):
            # host_np = np.empty(self._true_shape, dtype=trt.nptype(self._data_type))
            host_np = cuda.pagelocked_empty(list(self._true_shape), dtype=trt.nptype(self._data_type))
            cuda.memcpy_dtoh_async(host_np, self._buffer, stream)
            return host_np

    class InferJob:
        def __init__(self, inputs):
            self._inputs = inputs
            self._outputs = None
            self._err = None
            self._complete_condition = threading.Condition()

        def wait(self, timeout=None):
            with self._complete_condition:
                ret = self._complete_condition.wait(timeout)
            if ret:
                return self._outputs
            else:
                return None

        def job_done(self):
            with self._complete_condition:
                self._complete_condition.notify()

        @property
        def err(self):
            return self._err

        @err.setter
        def err(self, err):
            self._err = err

        @property
        def inputs(self):
            return self._inputs

        @property
        def outputs(self):
            return self._outputs

        @outputs.setter
        def outputs(self, outputs):
            self._outputs = outputs

    def __init__(self):
        super().__init__()
        self._model = None
        self._converter = TrtTensorConverter()  # used when `no converter mode`
        self._device_id = None
        self._streams = None
        self._customized_op_paths = None
        self._trt_logger = trt.Logger(trt.Logger.INFO)
        self._started_workers_count = 0  # Started workers count used to wait inferer start completely.
        self._started_workers_count_lock = threading.Lock()  # Lock to protect __started_workers_count.
        self._queues = []  # Each worker has one queue.
        self._worker_threads = []  # All worker threads.
        self._queue_max_size = 0  # Queue max size.
        self._stop = False
        self._current_submit_idx = 0  # The current worker idx to submit new infer job.
        self._submit_idx_lock = threading.Lock()  # Lock to protect __submit_idx_lock.

    def init(self, path, device=None, args=None):
        """
        Initiate model inferer class with model path and device.

        Args:
            path: Model path, it can be a file path or a directory path.
            device: Device to run model.
            args: More args.

        Raises:
            Exception: If init failed, raise exception. Will be caught by server and show error message to user when
            start service.
        """
        super().init(path, device, args)
        if not self._device:
            raise ValueError('Device not specified.')
        elif device in ['cuda', 'gpu']:
            self._device_id = 0
        elif re.match('^gpu:\\d+$', device):
            self._device_id = int(device.split(':')[1])
        elif re.match('^cuda:\\d+$', device):
            self._device_id = int(device.split(':')[1])
        elif device == 'original':
            self._device_id = 0
        else:
            raise ValueError(
                'Invalid device: {}, should be cuda, gpu, cuda:[num], gpu:[num] or original.'.format(device))

        if args is not None:
            self._streams = args.get('streams')
            if self._streams is not None:
                if type(self._streams) is not int:
                    raise ValueError('Invalid streams: {}, should be int.'.format(self._streams))
                if self._streams < 0:
                    raise ValueError('Invalid streams: {}, should be greater than 0.'.format(self._streams))

            self._customized_op_paths = args.get('customized_op_paths')
            if self._customized_op_paths is not None:
                if type(self._customized_op_paths) is not list:
                    raise ValueError(
                        'Invalid customized_op_paths: {}, should be list.'.format(self._customized_op_paths))
                for path in self._customized_op_paths:
                    if not os.path.exists(path):
                        raise ValueError('Customized op path not exists: {}.'.format(path))
                    ctypes.CDLL(path)
                    logger.info('Load customized op library: {}.'.format(path))
                trt.init_libnvinfer_plugins(self._trt_logger, '')

        if self._streams is None:
            self._streams = 1

        cuda.init()

    def load(self):
        """
        Load model from model path.

        Returns:
            True if load model successfully, otherwise False.

        Raises:
            Exception: If load failed, raise exception and exception will be caught by server and show error message
            to user when start service.
        """
        logger.info('Load tensorrt engine from {} to device {}.'.format(self._path, self._device))

        for i in range(self._streams):
            self._queues.append(Queue(self._queue_max_size))
            new_worker_thread = threading.Thread(target=self._worker_func, args=[i])
            new_worker_thread.setDaemon(True)
            new_worker_thread.start()
            self._worker_threads.append(new_worker_thread)

        # Wait all workers start completely.
        while True:
            with self._started_workers_count_lock:
                if self._started_workers_count == self._streams:
                    break
            time.sleep(0.1)
        logger.info('Multi streams trt inferer worker started completely, streams: {}.'.format(self._streams))
        return True

    def _worker_func(self, idx):
        worker_cuda_ctx = cuda.Device(self._device_id).make_context()  # Create current worker cuda ctx.
        worker_stream = cuda.Stream()  # Create current worker cuda stream.

        trt_runtime = trt.Runtime(self._trt_logger)
        # Deserialize engine from binary file.
        if not os.path.exists(self._path):
            raise FileNotFoundError('Model path not exists: {}.'.format(self._path))
        with open(self._path, "rb") as f:
            serialized_engine = f.read()
        engine = trt_runtime.deserialize_cuda_engine(serialized_engine)
        worker_trt_ctx = engine.create_execution_context()  # Create current worker tensorrt context.

        # Load binding.
        bindings = []
        for i in range(engine.num_bindings):
            name = engine.get_binding_name(i)
            dtype = engine.get_binding_dtype(i)
            shape = engine.get_binding_shape(i)
            is_input = engine.binding_is_input(i)
            is_shape = engine.is_shape_binding(i)
            vec_dim = engine.get_binding_vectorized_dim(i)
            comps = engine.get_binding_components_per_element(i)
            max_shape = engine.get_profile_shape(0, i)[-1] if is_input else shape
            bindings.append(self.TrtDeviceBinding(name, shape, max_shape, dtype, vec_dim, comps, is_input, is_shape))
            logger.info(
                'Trt inferer worker({}) add binding: {}, shape: {}, max_shape: {}, dtype: {}, vec_dim: {}, comps: {}, '
                'is_input: {}, is_shape: {}.'.format(idx, name, shape, max_shape, dtype, vec_dim, comps,
                                                     is_input, is_shape))

        with self._started_workers_count_lock:
            self._started_workers_count += 1

        logger.info('Trt inferer worker({}) started.'.format(idx))

        while not self._stop:
            try:
                infer_job = self._queues[idx].get(block=True)
                worker_cuda_ctx.push()  # Make current worker cuda contex as active contex.
                try:
                    infer_job.outputs = self._trt_infer(infer_job.inputs,
                                                        bindings,
                                                        worker_trt_ctx,
                                                        worker_stream,
                                                        idx)
                except Exception as e:
                    infer_job.err = e
                worker_cuda_ctx.pop()  # Make current worker contex not active.
                infer_job.job_done()
            except Empty:
                continue
        worker_cuda_ctx.pop()

    @staticmethod
    def _trt_infer(h_inputs, bindings, trt_ctx, stream, idx):
        """Perform trt infer

        Args:
            h_inputs: Inputs data in host mem.
            bindings: Tensorrt bindings.
            trt_ctx: Tensorrt context.
            stream: Cuda stream.
            idx: Worker idx.

        Returns:
            output.
        """
        if g_trt_inferer_dbg:
            # Create cuda events.
            inp_start_event, inp_end_event, compute_end_event, out_end_event \
                = (cuda.Event() for _ in range(4))

        # 1. Prepare bindings.
        if g_trt_inferer_dbg:
            inp_start_event.record(stream)
        dev_ptrs = []
        for i, binding in enumerate(bindings):
            if binding.is_input_binding:
                h_input = None
                if type(h_inputs) is dict:
                    h_input = h_inputs.get(binding.name)
                    if h_input is None:
                        raise ValueError('Input binding not found: {}.'.format(binding.name))
                elif type(h_inputs) is list or type(h_inputs) is tuple:
                    if i >= len(h_inputs):
                        raise ValueError('Input binding not found: {}.'.format(binding.name))
                    h_input = h_inputs[i]
                elif type(h_inputs) is np.ndarray:
                    h_input = h_inputs
                else:
                    raise ValueError('Invalid input type: {}, should be dict(str, np.ndarray), list(np.ndarray),\
                     tuple(np.ndarray) or np.ndarray.'.format(type(h_inputs)))

                # Set shape binding value.
                if binding.is_shape_binding:
                    if h_input.dtype != np.int32:
                        raise ValueError('Shape binding data type should be INT32, binding idx: {}, binding name: {}'
                                         .format(i, binding.name))
                    if len(binding.shape) > 1:
                        raise ValueError('Shape binding should be a scalar or 1D tensor, but got: {}, binding idx: {}, '
                                         'binding name: {}.'.format(binding.shape, i, binding.name))
                    elif len(binding.shape) == 0 and h_input.size != 1:
                        raise ValueError(
                            'Shape binding should be a scalar, but got: {}, binding idx: {}, binding name: {}.'
                            .format(h_input, i, binding.name))
                    elif len(binding.shape) == 1 and binding.shape[0] != h_input.size:
                        raise ValueError(
                            'Shape binding should be a 1D vector with size {}, but got: {}, binding idx: {}, '
                            'binding name: {}.'.format(binding.shape[0], h_input, i, binding.name))
                    trt_ctx.set_shape_input(i, h_input.tolist())

                binding.from_host(h_input, stream)
                trt_ctx.set_binding_shape(i, trt.Dims(binding.true_shape))
            else:
                true_shape = trt_ctx.get_binding_shape(i)
                binding.allocate(true_shape)
            dev_ptrs.append(int(binding.buffer))

        if g_trt_inferer_dbg:
            inp_end_event.record(stream)

        # 2. Execute.
        trt_ctx.execute_async_v2(bindings=dev_ptrs, stream_handle=stream.handle)

        if g_trt_inferer_dbg:
            compute_end_event.record(stream)

        # 3. Output to host.
        outs = {}
        for binding in bindings:
            if not binding.is_input_binding:
                out = binding.to_host(stream)
                outs[binding.name] = out

        if g_trt_inferer_dbg:
            out_end_event.record(stream)

        # 4. Sync.
        stream.synchronize()

        if g_trt_inferer_dbg:
            logger.info('Trt inferer worker({}), H2D: {}, Compute: {}, D2H: {}.'.format(
                idx, inp_start_event.time_since(inp_start_event), compute_end_event.time_since(inp_end_event),
                out_end_event.time_since(compute_end_event)))

        return outs

    def infer(self, tensors, context):
        """
        The inference function is used to make a prediction call on the given input request.

        Args:
            context: grps context
            tensors: Model infer input, which is output of converter preprocess function. When in `no converter mode`,
            will skip converter preprocess and directly use GrpsMessage as input.

        Returns:
            Model infer output, which will be input of converter postprocess function. When in `no converter mode`, it
            will skip converter postprocess and should directly use GrpsMessage as output.

        Raises:
            Exception: If infer failed, raise exception and exception will be caught by server and return error
            message to client.
        """
        if str(type(tensors)) == str(GrpsMessage):
            # No converter mode.
            tensors = self._converter.preprocess(tensors, context)
            tensors = self.infer(tensors, context)
            return self._converter.postprocess(tensors, context)

        with self._submit_idx_lock:
            submit_idx = self._current_submit_idx
            self._current_submit_idx = (self._current_submit_idx + 1) % self._streams

        infer_job = self.InferJob(tensors)
        self._queues[submit_idx].put(infer_job, block=True, timeout=10)

        infer_job.wait()

        if infer_job.err:
            raise infer_job.err

        return infer_job.outputs

    def batch_infer(self, inp, contexts: list):
        """
        Batch infer.

        Args:
            inp: Model infer input, which is output of converter batch_preprocess function. When in `no converter mode`,
            will skip converter batch_preprocess and directly use GrpsMessage list as input.
            contexts: Grps context list.

        Returns:
            Model infer output, which will be input of converter batch_postprocess function. When in `no converter mode`,
            it will skip converter batch_postprocess and should directly use GrpsMessage list as output.

        Raises:
            Exception: If batch infer failed, can raise exception and exception will be caught by server and return
            error message to client.
        """
        if type(inp) is list and str(type(inp[0])) == str(GrpsMessage):
            # No converter mode.
            tensors = self._converter.batch_preprocess(inp, contexts)
            tensors = self.batch_infer(tensors, contexts)
            return self._converter.batch_postprocess(tensors, contexts)
        else:
            return self.infer(inp, contexts[0])  # contexts are useless.
