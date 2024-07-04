# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/9/4
# Brief  Tensorflow tensor converter. Converter grps msg to tf tensor when preprocess, and convert tf tensor to grps
#        msg when postprocess.
import tensorflow as tf

from grps_framework.apis.grps_pb2 import GrpsMessage, GenericTensor, DataType
from grps_framework.context.context import GrpsContext
from grps_framework.converter.converter import Converter


class TfTensorConverter(Converter):
    """Tensorflow tensor converter."""

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
    def __gtensor_type_2_tf_type(dtype):
        if dtype == DataType.DT_UINT8:
            return tf.uint8
        elif dtype == DataType.DT_INT8:
            return tf.int8
        elif dtype == DataType.DT_INT16:
            return tf.int16
        elif dtype == DataType.DT_INT32:
            return tf.int32
        elif dtype == DataType.DT_INT64:
            return tf.int64
        elif dtype == DataType.DT_FLOAT16:
            return tf.float16
        elif dtype == DataType.DT_FLOAT32:
            return tf.float32
        elif dtype == DataType.DT_FLOAT64:
            return tf.float64
        else:
            raise Exception('Unsupported data type: {}'.format(dtype))

    @staticmethod
    def __gtensor_2_tf_tensor(gtensor, name):
        shape = gtensor.shape
        tensor_size = 1
        for dim in shape:
            tensor_size *= dim

        if gtensor.dtype == DataType.DT_UINT8:
            g_flat_tensor = gtensor.flat_uint8
            dtype = tf.uint8
        elif gtensor.dtype == DataType.DT_INT8:
            g_flat_tensor = gtensor.flat_int8
            dtype = tf.int8
        elif gtensor.dtype == DataType.DT_INT16:
            g_flat_tensor = gtensor.flat_int16
            dtype = tf.int16
        elif gtensor.dtype == DataType.DT_INT32:
            g_flat_tensor = gtensor.flat_int32
            dtype = tf.int32
        elif gtensor.dtype == DataType.DT_INT64:
            g_flat_tensor = gtensor.flat_int64
            dtype = tf.int64
        elif gtensor.dtype == DataType.DT_FLOAT16:
            g_flat_tensor = gtensor.flat_float16
            dtype = tf.float16
        elif gtensor.dtype == DataType.DT_FLOAT32:
            g_flat_tensor = gtensor.flat_float32
            dtype = tf.float32
        elif gtensor.dtype == DataType.DT_FLOAT64:
            g_flat_tensor = gtensor.flat_float64
            dtype = tf.float64
        elif gtensor.dtype == DataType.DT_STRING:
            g_flat_tensor = gtensor.flat_string
            dtype = tf.string
        else:
            raise RuntimeError('tf tensor converter preprocess failed, unsupported data type: {}'.format(gtensor.dtype))

        if not g_flat_tensor:
            raise RuntimeError('tf tensor converter preprocess failed, {} data is empty.'.format(dtype))

        if tensor_size != len(g_flat_tensor):
            raise RuntimeError('generic tensor to tensorflow tensor error, tensor: {} size not match, shape: {}, '
                               'expected size: {}, actual size: {}.'.format(name, shape, tensor_size,
                                                                            len(g_flat_tensor)))

        tf_tensor = tf.constant(g_flat_tensor, shape=shape, dtype=dtype)
        return tf_tensor

    @staticmethod
    def __tf_tensor_2_gtensor(tf_tensor, name):
        gtensor = GenericTensor(name=name)
        gtensor.shape.extend(tf_tensor.shape)

        tensor_list = tf_tensor.numpy().flatten().tolist()
        if tf_tensor.dtype == tf.uint8:
            gtensor.dtype = DataType.DT_UINT8
            gtensor.flat_uint8.extend(tensor_list)
        elif tf_tensor.dtype == tf.int8:
            gtensor.dtype = DataType.DT_INT8
            gtensor.flat_int8.extend(tensor_list)
        elif tf_tensor.dtype == tf.int16:
            gtensor.dtype = DataType.DT_INT16
            gtensor.flat_int16.extend(tensor_list)
        elif tf_tensor.dtype == tf.int32:
            gtensor.dtype = DataType.DT_INT32
            gtensor.flat_int32.extend(tensor_list)
        elif tf_tensor.dtype == tf.int64:
            gtensor.dtype = DataType.DT_INT64
            gtensor.flat_int64.extend(tensor_list)
        elif tf_tensor.dtype == tf.float16:
            gtensor.dtype = DataType.DT_FLOAT16
            gtensor.flat_float16.extend(tensor_list)
        elif tf_tensor.dtype == tf.float32:
            gtensor.dtype = DataType.DT_FLOAT32
            gtensor.flat_float32.extend(tensor_list)
        elif tf_tensor.dtype == tf.float64:
            gtensor.dtype = DataType.DT_FLOAT64
            gtensor.flat_float64.extend(tensor_list)
        elif tf_tensor.dtype == tf.string:
            gtensor.dtype = DataType.DT_STRING
            gtensor.flat_string.extend(tensor_list)
        else:
            raise RuntimeError('tf tensor converter postprocess failed, unsupported data type.')

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
            raise RuntimeError('tf tensor converter preprocess failed, input has no gtensors.')

        # check size
        if len(inp.gtensors.tensors) == 0:
            raise RuntimeError('tf tensor converter preprocess failed, input has no tensor.')

        # check names.
        has_name = inp.gtensors.tensors[0].name != ''
        for tensor in inp.gtensors.tensors:
            if (has_name and tensor.name == '') or (not has_name and tensor.name != ''):
                raise RuntimeError(
                    'tf tensor converter preprocess failed, gtensors tensors should all have name or'
                    ' all have no name(will use default name).')

        if has_name:
            tensor_map = dict()
            for tensor in inp.gtensors.tensors:
                name = tensor.name
                if tensor_map.get(name):
                    raise RuntimeError('tf tensor converter preprocess failed, tensor name {} duplicated.'.format(name))
                tensor_map[name] = self.__gtensor_2_tf_tensor(tensor, name)
            return tensor_map
        else:
            tensor_list = []
            for idx, tensor in enumerate(inp.gtensors.tensors):
                tensor_list.append(self.__gtensor_2_tf_tensor(tensor, str(idx)))
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
        if isinstance(inp, tf.Tensor):
            out.gtensors.tensors.append(self.__tf_tensor_2_gtensor(inp, 'output'))
        elif isinstance(inp, list) or isinstance(inp, tuple):
            for idx, tensor in enumerate(inp):
                name = 'output_{}'.format(idx)
                if isinstance(tensor, tf.Tensor):
                    out.gtensors.tensors.append(self.__tf_tensor_2_gtensor(tensor, name))
                else:
                    raise RuntimeError('invalid type in tensor list, should be tensor, but got {}'.format(type(tensor)))
        elif isinstance(inp, dict):
            for name, tensor in inp.items():
                if isinstance(tensor, tf.Tensor):
                    out.gtensors.tensors.append(self.__tf_tensor_2_gtensor(tensor, name))
                else:
                    raise RuntimeError('invalid type in tensor dict, should be tensor, but got {}'.format(type(tensor)))
        else:
            raise RuntimeError('invalid type in postprocess, should be tensor or tensor list or tensor dict, '
                               'but got {}'.format(type(inp)))
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
            raise RuntimeError('tf tensor converter batch preprocess failed, inputs size not match with contexts.')

        tensor_names = []
        tensor_dtypes = []
        tensor_shapes = []
        tensors = []
        has_name = False
        for i, inp in enumerate(inps):
            g_tensors = inp.gtensors
            # Check size.
            if len(g_tensors.tensors) == 0:
                raise RuntimeError('tf tensor converter batch preprocess error, some one gtensors tensors size is 0.')

            # Check names and shape of each tensor.
            has_name = not g_tensors.tensors[0].name == ''
            cur_batch_size = g_tensors.tensors[0].shape[0]
            for g_tensor in g_tensors.tensors:
                if (has_name and g_tensor.name == '') or (not has_name and g_tensor.name != ''):
                    raise RuntimeError(
                        'tf tensor converter batch preprocess error, gtensors tensors should all have name or all'
                        ' have no name(will use default name).')
                if len(g_tensor.shape) <= 1:
                    raise RuntimeError(
                        'tf tensor converter batch preprocess error, tensor shape size should be greater than 1.')
                if cur_batch_size != g_tensor.shape[0]:
                    raise RuntimeError(
                        'tf tensor converter batch preprocess error, batch size of each tensor not match.')

            contexts[i].put_user_data('batch_size', cur_batch_size)
            if i == 0:  # Initialize tensors names, dtypes and shapes use first input.
                for j, g_tensor in enumerate(g_tensors.tensors):
                    tensor_names.append(g_tensor.name if has_name else '')
                    tensor_dtypes.append(self.__gtensor_type_2_tf_type(g_tensor.dtype))
                    tensor_shapes.append(g_tensor.shape)
                    # Converter tensor.
                    tensors.append([self.__gtensor_2_tf_tensor(g_tensor, tensor_names[j])])
            else:  # Compare names, dtypes and shapes of follow inputs with first input.
                if len(g_tensors.tensors) != len(tensor_names):
                    raise RuntimeError('tf tensor converter batch preprocess error, tensor size not match.')
                for j, g_tensor in enumerate(g_tensors.tensors):
                    if tensor_names[j] != (g_tensor.name if has_name else ''):
                        raise RuntimeError('tf tensor converter batch preprocess error, tensor names not match.')
                    if tensor_dtypes[j] != self.__gtensor_type_2_tf_type(g_tensor.dtype):
                        raise RuntimeError('tf tensor converter batch preprocess error, tensor dtypes not match.')
                    if len(tensor_shapes[j]) != len(g_tensor.shape):
                        raise RuntimeError('tf tensor converter batch preprocess error, tensor shapes not match.')
                    for k in range(1, len(g_tensor.shape)):
                        if tensor_shapes[j][k] != g_tensor.shape[k]:
                            raise RuntimeError('tf tensor converter batch preprocess error, tensor shapes not match.')
                    # Converter tensor.
                    tensors[j].append(self.__gtensor_2_tf_tensor(g_tensor, tensor_names[j]))
        # Concat
        for i in range(len(tensors)):
            batch_tensor = tf.concat(tensors[i], axis=0)
            tensors[i] = batch_tensor

        if has_name:
            tensors_map = dict(zip(tensor_names, tensors))
            if len(tensors_map) != len(tensor_names):
                raise RuntimeError('tf tensor converter batch preprocess error, tensor names duplicated.')
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
        if isinstance(inp, tf.Tensor):
            tensor_cpus.append(inp)
            names.append('output')
        elif isinstance(inp, list) or isinstance(inp, tuple):
            for idx, tensor in enumerate(inp):
                if isinstance(tensor, tf.Tensor):
                    tensor_cpus.append(tensor)
                    names.append('output_{}'.format(idx))
                else:
                    raise RuntimeError('invalid type in tensor list, should be tensor, but got {}'.format(type(tensor)))
        elif isinstance(inp, dict):
            for key, value in inp.items():
                if isinstance(value, tf.Tensor):
                    tensor_cpus.append(value)
                    names.append(key)
                else:
                    raise RuntimeError(
                        'invalid type in tensor map, should be tensor, but got {}'.format(type(value)))
        else:
            raise RuntimeError(
                'tf tensor converter batch postprocess failed, unsupported input type: {},'
                ' should be tensor, tensor list or tensor map'.format(type(inp)))

        # Split batch tensor and convert to gtensor.
        outs = []
        offsets = [0 for j in range(len(tensor_cpus))]
        for i, ctx in enumerate(contexts):
            batch_size = ctx.get_user_data('batch_size')
            out = GrpsMessage()
            for j in range(len(tensor_cpus)):
                tensor = tensor_cpus[j][offsets[j]:(offsets[j] + batch_size)]
                out.gtensors.tensors.append(self.__tf_tensor_2_gtensor(tensor, names[j]))
                offsets[j] += batch_size
            outs.append(out)

        return outs
