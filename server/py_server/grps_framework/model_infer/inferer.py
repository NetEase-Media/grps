# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  Define deep learning model inferer base class for different deep learning framework. Including
#        model load and model infer. Customized model inferer class should inherit this class.
from abc import ABCMeta, abstractmethod


class ModelInferer(metaclass=ABCMeta):
    """ Model inferer base class. """

    def __init__(self):
        self._path = None
        self._device = None
        self._args = None

    @abstractmethod
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
        self._path = path
        self._device = device
        self._args = args

    @abstractmethod
    def load(self):
        """
        Load model from model path.

        Returns:
            True if load model successfully, otherwise False.

        Raises:
            Exception: If load failed, can raise exception and exception will be caught by server and show error message
            to user when start service.
        """
        pass

    @abstractmethod
    def infer(self, inp, context):
        """
        The inference function is used to make a prediction call on the given input request.

        Args:
            inp: Model infer input, which is output of converter preprocess function. When in `no converter mode`, will
            skip converter preprocess and directly use GrpsMessage as input.
            context: grps context

        Returns:
            Model infer output, which will be input of converter postprocess function. When in `no converter mode`, it
            will skip converter postprocess and should directly use GrpsMessage as output.

        Raises:
            Exception: If infer failed, can raise exception and exception will be caught by server and return error
            message to client.
        """
        pass

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
        pass


class ModelInfererRegister:
    """ Model inferer register. """

    def __init__(self):
        self.__model_inferer_dict = {}

    def register(self, inferer_name, inferer_obj):
        """
        Register model inferer. If inferer_name already exists, it will be overwritten.

        Args:
            inferer_name: Model inferer name.
            inferer_obj: Model inferer object.

        Raises:
            Exception: If register failed, raise exception and exception will be caught by server and show error
            message to user when start service.
        """
        self.__model_inferer_dict[inferer_name] = inferer_obj

    def get_model_inferer(self, inferer_name):
        """
        Get model inferer by model inferer name.

        Args:
            inferer_name: Model inferer name.

        Returns:
            Model inferer object.

        Raises:
            Exception: If get failed, raise exception and exception will be caught by server and show error message
            to user when start service.
        """
        if inferer_name not in self.__model_inferer_dict:
            raise RuntimeError('Model inferer name {} not registered.'.format(inferer_name))
        return self.__model_inferer_dict[inferer_name]

    @property
    def model_inferer_dict(self):
        return self.__model_inferer_dict


inferer_register = ModelInfererRegister()
