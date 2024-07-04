# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  Torch deep learning model inferer. Including model load and model infer.
import os
import re

import torch

from grps_framework.apis.grps_pb2 import GrpsMessage
from grps_framework.converter.torch_tensor_converter import TorchTensorConverter
from grps_framework.logger.logger import logger
from grps_framework.model_infer.inferer import ModelInferer


class TorchModelInferer(ModelInferer):
    """Torch model(torch script model format) inferer."""

    def __init__(self):
        super().__init__()
        self._model = None
        self._inp_device = None
        self._converter = TorchTensorConverter()  # used when no converter mode

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
        if torch.__version__ < '1.8.0':
            raise RuntimeError(
                'PyTorch version should be greater than or equal 1.8.0. Current version is {}.'.format(
                    torch.__version__))
        super().init(path, device, args)

        if not device:
            raise ValueError('Device not specified.')
        elif device == 'cpu':
            self._device = 'cpu'
            self._inp_device = 'cpu'
        elif device in ['cuda', 'gpu']:
            # == cuda:0
            self._device = 'cuda:0'
            self._inp_device = 'cuda:0'
        elif re.match('^cuda:\\d+$', device):
            self._inp_device = self._device
        elif re.match('^gpu:\\d+$', device):
            self._device = 'cuda:' + self._device[4:]
            self._inp_device = self._device
        elif device[:8] == 'original':
            self._device = 'original'
            self._inp_device = device[9:]
            if self._inp_device not in ['cpu', 'cuda', 'gpu'] and \
                    not re.match('^cuda:\\d+$', self._inp_device) and \
                    not re.match('^gpu:\\d+$', self._inp_device):
                raise ValueError('Invalid inp_device: {}, should be cpu, cuda, gpu, cuda:[num] or gpu:[num].'
                                 .format(self._inp_device))
            if re.match('^gpu:\\d+$', self._inp_device):
                self._inp_device = 'cuda:' + self._inp_device[4:]
        else:
            raise ValueError(
                'Invalid device: {}, should be cpu, cuda, gpu, cuda:[num], gpu:[num] or original.'.format(self._device))

        if args is not None:
            self._customized_op_paths = args.get('customized_op_paths')
            if self._customized_op_paths is not None:
                if type(self._customized_op_paths) is not list:
                    raise ValueError(
                        'Invalid customized_op_paths: {}, should be list.'.format(self._customized_op_paths))
                for path in self._customized_op_paths:
                    if not os.path.exists(path):
                        raise ValueError('Customized op path not exists: {}.'.format(path))
                    torch.ops.load_library(path)
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
        self._model = torch.jit.load(self._path)
        if self._device != 'original':
            self._model.to(self._device)
        if not self._model:
            return False
        self._model.eval()
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
        with torch.no_grad():
            if str(type(tensors)) == str(GrpsMessage):
                # no converter mode
                tensors = self._converter.preprocess(tensors, context)
                tensors = self.infer(tensors, context)
                return self._converter.postprocess(tensors, context)

            # place tensors to device
            if isinstance(tensors, torch.Tensor):
                tensors = tensors.to(self._inp_device)
                return self._model(tensors)
            elif isinstance(tensors, list) or isinstance(tensors, tuple):
                if isinstance(tensors, tuple):
                    tensors = list(tensors)
                for idx, tensor in enumerate(tensors):
                    if isinstance(tensor, torch.Tensor):
                        tensors[idx] = tensor.to(self._inp_device)
                return self._model(*tensors)
            elif isinstance(tensors, dict):
                for key, value in tensors.items():
                    if isinstance(value, torch.Tensor):
                        tensors[key] = value.to(self._inp_device)
                return self._model(**tensors)
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
