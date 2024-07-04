# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/9/5
# Brief  Customized deep learning model inferer. Including model load and model infer.
import time

from grps_framework.apis import grps_pb2
from grps_framework.context.context import GrpsContext
from grps_framework.logger.logger import clogger
from grps_framework.model_infer.inferer import ModelInferer, inferer_register


class YourInferer(ModelInferer):

    def init(self, path, device=None, args=None):
        """
        Initiate model inferer class with model path and device.

        Args:
            path: Model path, it can be a file path or a directory path.
            device: Device to run model.
            args: More args.

        Raises:
            Exception: If init failed, can raise exception. Will be caught by server and show error message to user when
            start service.
        """
        super(YourInferer, self).init(path, device, args)
        clogger.info('your infer init, path: {}, device: {}, args: {}.'.format(path, device, args))

    def load(self):
        """
        Load model from model path.

        Returns:
            True if load model successfully, otherwise False.

        Raises:
            Exception: If load failed, can raise exception and exception will be caught by server and show error message
            to user when start service.
        """
        clogger.info('your inferer load.')
        return True

    def infer(self, inp, context: GrpsContext):
        """
        The inference function is used to make a prediction call on the given input request.

        Args:
            context: grps context
            inp: Model infer input, which is output of converter preprocess function. When in `no converter mode`, will
            skip converter preprocess and directly use GrpsMessage as input.

        Returns:
            Model infer output, which will be input of converter postprocess function. When in `no converter mode`, it
            will skip converter postprocess and should directly use GrpsMessage as output.

        Raises:
            Exception: If infer failed, can raise exception and exception will be caught by server and return error
            message to client.
        """
        # add your codes here.

        # If using streaming rpc, you can respond multi msg to client as follows:
        if context.if_streaming():
            context.stream_respond(grps_pb2.GrpsMessage(str_data='stream data 1'))
            time.sleep(0.1)  # Simulate the process of model infer.
            context.stream_respond(grps_pb2.GrpsMessage(str_data='stream data 2'))
            time.sleep(0.1)  # Simulate the process of model infer.

        return {}


# Register
inferer_register.register('your_inferer', YourInferer())
