# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2024/6/27
# Brief  Tensorrt tensor converter. Converter grps msg to trt tensor when preprocess, and convert trt tensor to grps
#        msg when postprocess.
import time

import numpy as np
import tensorrt as trt

from grps_framework.apis.grps_pb2 import DataType, GenericTensor, GrpsMessage
from grps_framework.context.context import GrpsContext
from grps_framework.converter.converter import Converter
from grps_framework.logger.logger import logger


class TrtTensorConverter(Converter):
    """Tensorrt tensor converter."""

    def init(self, path=None, args=None):
        """
        Init converter.

        Args:
            path: Path.
            args: More args.

        Raises:
            Exception: If init failed, raise exception and exception will be caught by server and show error message
            to user when start service.
        """
        super().init(path, args)

    @staticmethod
    def __gtensor_type_2_trt_type(dtype):
        if trt.__version__ >= '8.5' and dtype == DataType.DT_UINT8:
            return np.uint8
        elif dtype == DataType.DT_INT8:
            return np.int8
        elif dtype == DataType.DT_INT32:
            return np.int32
        elif dtype == DataType.DT_FLOAT32:
            return np.float32
        else:
            raise Exception('Unsupported data type: {}'.format(dtype))

    @staticmethod
    def __gtensor_2_trt_tensor(gtensor, name):
        shape = gtensor.shape
        tensor_size = 1
        for dim in shape:
            tensor_size *= dim

        if trt.__version__ >= '8.5' and gtensor.dtype == DataType.DT_UINT8:
            g_flat_tensor = gtensor.flat_uint8
            dtype = np.uint8
        elif gtensor.dtype == DataType.DT_INT8:
            g_flat_tensor = gtensor.flat_int8
            dtype = np.int8
        elif gtensor.dtype == DataType.DT_INT32:
            g_flat_tensor = gtensor.flat_int32
            dtype = np.int32
        elif gtensor.dtype == DataType.DT_FLOAT32:
            g_flat_tensor = gtensor.flat_float32
            dtype = np.float32
        else:
            raise RuntimeError(
                'trt tensor converter preprocess failed, unsupported data type: {}'.format(gtensor.dtype))

        if not g_flat_tensor:
            raise RuntimeError('trt tensor converter preprocess failed, {} data is empty.'.format(gtensor.dtype))

        if tensor_size != len(g_flat_tensor):
            raise RuntimeError('generic tensor to trt tensor error, tensor: {} size not match, shape: {}, '
                               'expected size: {}, actual size: {}.'.format(name, shape, tensor_size,
                                                                            len(g_flat_tensor)))
        trt_tensor = np.array(g_flat_tensor, dtype=dtype).reshape(shape)
        return trt_tensor

    @staticmethod
    def __trt_tensor_2_gtensor(trt_tensor, name):
        gtensor = GenericTensor(name=name)
        gtensor.shape.extend(trt_tensor.shape)

        tensor_list = trt_tensor.flatten().tolist()
        if trt.__version__ >= '8.5' and trt_tensor.dtype == np.uint8:
            gtensor.dtype = DataType.DT_UINT8
            gtensor.flat_uint8.extend(tensor_list)
        elif trt_tensor.dtype == np.int8:
            gtensor.dtype = DataType.DT_INT8
            gtensor.flat_int8.extend(tensor_list)
        elif trt_tensor.dtype == np.int32:
            gtensor.dtype = DataType.DT_INT32
            gtensor.flat_int32.extend(tensor_list)
        elif trt_tensor.dtype == np.float32:
            gtensor.dtype = DataType.DT_FLOAT32
            gtensor.flat_float32.extend(tensor_list)
        else:
            raise RuntimeError('trt tensor converter postprocess failed, unsupported data type: {}'.format(
                trt_tensor.dtype))

        return gtensor

    def preprocess(self, inp: GrpsMessage, context: GrpsContext):
        """
        Preprocess.

        Args:
            inp: Input message from client or previous model(multi model sequential mode).
            context: Grps context of current request.

        Returns:
            Pre-processed data which is input of model inferer.

        Raises:
            Exception: If preprocess failed, raise exception and exception will be caught by server and return error
            message to client.
        """
        if not inp.gtensors or not inp.gtensors.tensors:
            raise RuntimeError('trt tensor converter preprocess failed, input has no gtensors.')

        # check size
        if len(inp.gtensors.tensors) == 0:
            raise RuntimeError('trt tensor converter preprocess failed, input has no tensor.')

        # check names.
        has_name = inp.gtensors.tensors[0].name != ''
        for tensor in inp.gtensors.tensors:
            if (has_name and tensor.name == '') or (not has_name and tensor.name != ''):
                raise RuntimeError(
                    'trt tensor converter preprocess failed, gtensors tensors should all have name or'
                    ' all have no name(will use default name).')

        if has_name:
            tensor_map = dict()
            for tensor in inp.gtensors.tensors:
                name = tensor.name
                if tensor_map.get(name):
                    raise RuntimeError(
                        'trt tensor converter preprocess failed, tensor name {} duplicated.'.format(name))
                tensor_map[name] = self.__gtensor_2_trt_tensor(tensor, name)
            return tensor_map
        else:
            tensor_list = []
            for idx, tensor in enumerate(inp.gtensors.tensors):
                tensor_list.append(self.__gtensor_2_trt_tensor(tensor, str(idx)))
            return tensor_list

    def postprocess(self, inp: dict, context: GrpsContext) -> GrpsMessage:
        """
        Postprocess.

        Args:
            inp: Input to be post-processed, which is output of model inferer.
            context: Grps context of current request.

        Returns:
            Post-processed data with GrpsMessage format to client or next model(multi model sequential mode).

        Raises:
            Exception: If postprocess failed, raise exception and exception will be caught by server and return error
            message to client.
        """
        out = GrpsMessage()
        if isinstance(inp, dict):
            for name, tensor in inp.items():
                if isinstance(tensor, np.ndarray):
                    out.gtensors.tensors.append(self.__trt_tensor_2_gtensor(tensor, name))
                else:
                    raise RuntimeError(
                        'invalid type in tensor dict, should be np.ndarray, but got {}'.format(type(tensor)))
        else:
            raise RuntimeError('invalid type in postprocess, should be np.ndarray dict, but got {}'.format(type(inp)))
        return out

    def batch_preprocess(self, inps: list, contexts: list):
        """
        Batch preprocess.

        Args:
            inps: Input messages from client or previous model(multi model sequential mode).
            contexts: Grps contexts of current requests.

        Returns:
            Pre-processed data which is input of model inferer.

        Raises:
            Exception: If preprocess failed, can raise exception and exception will be caught by server and return error
            message to client.
        """
        if len(inps) != len(contexts):
            raise RuntimeError('trt tensor converter batch preprocess failed, inputs size not match with contexts.')

        tensor_names = []
        tensor_dtypes = []
        tensor_shapes = []
        tensors = []
        has_name = False
        for i, inp in enumerate(inps):
            g_tensors = inp.gtensors
            # Check size.
            if len(g_tensors.tensors) == 0:
                raise RuntimeError('trt tensor converter batch preprocess error, some one gtensors tensors size is 0.')

            # Check names and shape of each tensor.
            has_name = not g_tensors.tensors[0].name == ''
            cur_batch_size = g_tensors.tensors[0].shape[0]
            for g_tensor in g_tensors.tensors:
                if (has_name and g_tensor.name == '') or (not has_name and g_tensor.name != ''):
                    raise RuntimeError(
                        'trt tensor converter batch preprocess error, gtensors tensors should all have name or all'
                        ' have no name(will use default name).')
                if len(g_tensor.shape) <= 1:
                    raise RuntimeError(
                        'trt tensor converter batch preprocess error, tensor shape size should be greater than 1.')
                if cur_batch_size != g_tensor.shape[0]:
                    raise RuntimeError(
                        'trt tensor converter batch preprocess error, batch size of each tensor not match.')

            contexts[i].put_user_data('batch_size', cur_batch_size)
            if i == 0:  # Initialize tensors names, dtypes and shapes use first input.
                for j, g_tensor in enumerate(g_tensors.tensors):
                    tensor_names.append(g_tensor.name if has_name else '')
                    tensor_dtypes.append(self.__gtensor_type_2_trt_type(g_tensor.dtype))
                    tensor_shapes.append(g_tensor.shape)
                    # Converter tensor.
                    tensors.append([self.__gtensor_2_trt_tensor(g_tensor, tensor_names[j])])
            else:  # Compare names, dtypes and shapes of follow inputs with first input.
                if len(g_tensors.tensors) != len(tensor_names):
                    raise RuntimeError('trt tensor converter batch preprocess error, tensor size not match.')
                for j, g_tensor in enumerate(g_tensors.tensors):
                    if tensor_names[j] != (g_tensor.name if has_name else ''):
                        raise RuntimeError('trt tensor converter batch preprocess error, tensor names not match.')
                    if tensor_dtypes[j] != self.__gtensor_type_2_trt_type(g_tensor.dtype):
                        raise RuntimeError('trt tensor converter batch preprocess error, tensor dtypes not match.')
                    if len(tensor_shapes[j]) != len(g_tensor.shape):
                        raise RuntimeError('trt tensor converter batch preprocess error, tensor shapes not match.')
                    for k in range(1, len(g_tensor.shape)):
                        if tensor_shapes[j][k] != g_tensor.shape[k]:
                            raise RuntimeError('trt tensor converter batch preprocess error, tensor shapes not match.')
                    # Converter tensor.
                    tensors[j].append(self.__gtensor_2_trt_tensor(g_tensor, tensor_names[j]))
        # Concat
        for i in range(len(tensors)):
            batch_tensor = np.concatenate(tensors[i], axis=0)
            tensors[i] = batch_tensor

        if has_name:
            tensors_map = dict(zip(tensor_names, tensors))
            if len(tensors_map) != len(tensor_names):
                raise RuntimeError('trt tensor converter batch preprocess error, tensor names duplicated.')
            return tensors_map
        else:
            return tensors

    def batch_postprocess(self, inp, contexts: list) -> list:
        """
        Batch postprocess.

        Args:
            inp: Input to be post-processed, which is output of model inferer.
            contexts: Grps contexts of current requests.

        Returns:
            Post-processed data with GrpsMessage format to client or next model(multi model sequential mode).

        Raises:
            Exception: If postprocess failed, can raise exception and exception will be caught by server and return
            error message to client.
        """
        tensor_cpus = []
        names = []
        if isinstance(inp, dict):
            for key, value in inp.items():
                if isinstance(value, np.ndarray):
                    tensor_cpus.append(value)
                    names.append(key)
                else:
                    raise RuntimeError(
                        'invalid type in tensor map, should be np.ndarray, but got {}'.format(type(value)))
        else:
            raise RuntimeError(
                'trt tensor converter batch postprocess failed, unsupported input type: {},'
                ' should be np.ndarray map'.format(type(inp)))

        # Split batch tensor and convert to gtensor.
        outs = []
        offsets = [0 for j in range(len(tensor_cpus))]
        for i, ctx in enumerate(contexts):
            batch_size = ctx.get_user_data('batch_size')
            out = GrpsMessage()
            for j in range(len(tensor_cpus)):
                tensor = tensor_cpus[j][offsets[j]:(offsets[j] + batch_size)]
                out.gtensors.tensors.append(self.__trt_tensor_2_gtensor(tensor, names[j]))
                offsets[j] += batch_size
            outs.append(out)

        return outs
