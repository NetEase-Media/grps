# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/10/24
# Brief  Tensorflow deep learning model inferer. Including model load and model infer.
import os
import re

import tensorflow as tf

from grps_framework.apis.grps_pb2 import GrpsMessage
from grps_framework.converter.tf_tensor_converter import TfTensorConverter
from grps_framework.logger.logger import logger
from grps_framework.model_infer.inferer import ModelInferer


class TfModelInferer(ModelInferer):
    """Tensorflow model(tf saved model format) infer class."""

    def __init__(self):
        super().__init__()
        self._model = None
        self._converter = TfTensorConverter()  # used when `no converter mode`

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
        if tf.__version__ < '2.0':
            raise RuntimeError(
                'Tensorflow version must be greater than or equal 2.0, current version is {}.'.format(tf.__version__))
        super().init(path, device, args)
        if not self._device:
            raise ValueError('Device not specified.')
        elif device in ['cuda', 'gpu']:
            # == gpu:0
            self._device = 'gpu:0'
        elif device == 'cpu':
            pass
        elif re.match('^gpu:\\d+$', device):
            pass
        elif re.match('^cuda:\\d+$', device):
            self._device = device.replace('cuda', 'gpu')
        elif device == 'original':
            pass
        else:
            raise ValueError(
                'Invalid device: {}, should be cpu, cuda, gpu, cuda:[num], gpu:[num] or original.'.format(device))

        if args is not None:
            self._customized_op_paths = args.get('customized_op_paths')
            if self._customized_op_paths is not None:
                if type(self._customized_op_paths) is not list:
                    raise ValueError(
                        'Invalid customized_op_paths: {}, should be list.'.format(self._customized_op_paths))
                for path in self._customized_op_paths:
                    if not os.path.exists(path):
                        raise ValueError('Customized op path not exists: {}.'.format(path))
                    tf.load_op_library(path)
                    logger.info('Load customized op library: {}.'.format(path))

    def load(self):
        """
        Load model from model path.

        Returns:
            True if load model successfully, otherwise False.

        Raises:
            Exception: If load failed, raise exception and exception will be caught by server and show error message
            to user when start service.
        """
        logger.info('Load model from {} to device {}.'.format(self._path, self._device))
        if self._device == 'original':
            self._model = tf.saved_model.load(self._path)
        else:
            with tf.device(self._device):
                self._model = tf.saved_model.load(self._path)
        if not self._model:
            return False
        logger.info('Load model successfully.')
        return True

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

        if self._device == 'original':
            if type(tensors) is dict:
                return self._model(**tensors)
            elif type(tensors) is list or type(tensors) is tuple:
                return self._model(*tensors)
            else:
                return self._model(tensors)
        else:
            with tf.device(self._device):
                if type(tensors) is dict:
                    return self._model(**tensors)
                elif type(tensors) is list or type(tensors) is tuple:
                    return self._model(*tensors)
                else:
                    return self._model(tensors)

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
