# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/14
# Brief  Converter base definition of model, including pre-process and post-process. Customized converter should
#        inherit this class.

from abc import ABCMeta, abstractmethod

from grps_framework.apis.grps_pb2 import GrpsMessage
from grps_framework.context.context import GrpsContext


class Converter(metaclass=ABCMeta):
    """Converter base class."""

    def __init__(self):
        self._path = None
        self._args = None

    @abstractmethod
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
        self._path = path
        self._args = args

    @abstractmethod
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
        pass

    @abstractmethod
    def postprocess(self, inp, context: GrpsContext) -> GrpsMessage:
        """
        Postprocess.

        Args:
            inp: Input to be post-processed, which is output of model inferer.
            context: Grps context of current request.

        Returns:
            Post-processed data with GrpsMessage format to client or next model(multi model sequential mode).

        Raises:
            Exception: If postprocess failed, can raise exception and exception will be caught by server and return
            error message to client.
        """
        pass

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
        pass

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
        pass


class ConverterRegister:
    """Converter register."""

    def __init__(self):
        self.__converter_dict = {}

    def register(self, converter_name, converter_obj):
        """
        Register converter. If converter_name already exists, it will be overwritten.

        Args:
            converter_name: Converter name.
            converter_obj: Converter object.

        Raises:
            Exception: If register failed, raise exception and exception will be caught by server and show error
            message to user when start service.
        """
        self.__converter_dict[converter_name] = converter_obj

    def get_converter(self, converter_name):
        """
        Get converter.

        Args:
            converter_name: Converter name.

        Returns:
            Converter object.

        Raises:
            Exception: If get failed, raise exception and exception will be caught by server and show error message
            to user when start service.
        """
        if converter_name not in self.__converter_dict:
            raise ValueError('Converter {} not registered.'.format(converter_name))
        return self.__converter_dict[converter_name]

    @property
    def converter_dict(self):
        return self.__converter_dict


converter_register = ConverterRegister()
