# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/9/5
# Brief  Customized converter of model, including pre-process and post-process.
from grps_framework.apis.grps_pb2 import GrpsMessage, GenericTensor, DataType
from grps_framework.context.context import GrpsContext
from grps_framework.converter.converter import Converter, converter_register
from grps_framework.logger.logger import clogger


class YourConverter(Converter):
    """Your converter."""

    def init(self, path=None, args=None):
        """
        Init converter.

        Args:
            path: Path.
            args: More args.

        Raises:
            Exception: If init failed, can raise exception and exception will be caught by server and show error message
            to user when start service.
        """
        super().init(path, args)
        clogger.info('your converter init, path: {}, args: {}'.format(path, args))

    def preprocess(self, inp: GrpsMessage, context: GrpsContext):
        """
        Preprocess.

        Args:
            inp: Input message from client or previous model(multi model sequential mode).
            context: Grps context of current request.

        Returns:
            Pre-processed data which is input of model inferer.

        Raises:
            Exception: If preprocess failed, can raise exception and exception will be caught by server and return error
            message to client.
        """
        # your can set context for current request like，value can be any type:
        context.put_user_data('key', 'value')

        # preprocess request and convert to tensors.
        # add your codes here.
        return {}

    def postprocess(self, inp, context: GrpsContext) -> GrpsMessage:
        """
        Postprocess.

        Args:
            inp: Input to be post-processed, which is output of model inferer.
            context: Grps context of current request.

        Returns:
            Post-processed data with GrpsMessage format to client or next model(multi model sequential mode).

        Raises:
            Exception: If postprocess failed, can raise exception and exception will be caught by server and return error
            message to client.
        """
        # you can get context of current request like:
        clogger.info('context: {}'.format(context.get_user_data('key')))

        out = GrpsMessage()
        # postprocess tensors and convert to response.
        # add your codes here like:
        out.str_data = 'hello grps.'  # add string data.

        # add generic tensor([[1.0, 2.0, 3.0]] like:
        # gtensor = GenericTensor(name='tensor_name', dtype=DataType.DT_FLOAT32, shape=[1, 3], flat_float32=[1, 2, 3])
        # out.gtensors.tensors.append(gtensor)
        return out


converter_register.register('your_converter', YourConverter())
