# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/14
# Brief  Node of DAG.
import time
from abc import ABCMeta, abstractmethod

from grps_framework.context.context import GrpsContext
from grps_framework.logger.logger import logger


class NodeBase(metaclass=ABCMeta):
    """Node base class."""

    @abstractmethod
    def __init__(self, name):
        """Initiate node base class."""
        self._name = name

    @abstractmethod
    def process(self, data, context):
        """
        Node process.
        Args:
            context: grps context
            data: node input.

        Returns:
            node process output.
        """
        pass

    @property
    def name(self):
        return self._name

    def __str__(self):
        return self._name


class ModelNode(NodeBase):
    """Model node, which means the node is a model."""

    def __init__(self, name, model_infer, converter, batcher):
        """
        Init model node.
        Args:
            name: node name.
            model_infer: Model infer instance.
            converter: Converter instance.
            batcher: Batcher instance.
        """
        super(ModelNode, self).__init__(name)
        self.__model_infer = model_infer
        self.__converter = converter
        self.__batcher = batcher

    def process(self, data, context: GrpsContext):
        """
        Node process.
        Args:
            context: grps context
            data: node input.

        Returns:
            node process output.
        """
        begin = time.time()

        context.set_converter(self.__converter)
        context.set_inferer(self.__model_infer)

        if self.__batcher is not None:
            return self.__batcher.infer(data, context)

        if self.__converter is None:
            infer_in = data
            infer_out = self.__model_infer.infer(infer_in, context)
            # logger.debug('Infer_input: {}, infer_out: {}'.format(infer_input, infer_out))
            infer_time = time.time()
            if context.has_err():
                return

            logger.info(
                'Model({}), model_infer time: {:.0f} us'.format(self._name, (infer_time - begin) * 1e6))
            return infer_out
        else:
            infer_input = self.__converter.preprocess(data, context)
            preprocess_time = time.time()
            if context.has_err():
                return

            infer_out = self.__model_infer.infer(infer_input, context)
            # logger.debug('Infer_input: {}, infer_out: {}'.format(infer_input, infer_out))
            infer_time = time.time()
            if context.has_err():
                return

            out = self.__converter.postprocess(infer_out, context)
            postprocess_time = time.time()
            if context.has_err():
                return

            logger.info(
                'Model({}), preprocess time: {:.0f} us, model_infer time: {:.0f} us, postprocess time: {:.0f} us'.format(
                    self._name, (preprocess_time - begin) * 1e6, (infer_time - preprocess_time) * 1e6,
                                (postprocess_time - infer_time) * 1e6))

            return out


# TODO: Add splitter node.
class SplitterNode(NodeBase):
    """Splitter node, which means the node is a splitter."""

    def __init__(self, name):
        pass

    def process(self, data, context):
        pass


# TODO: Add merger node.
class MergerNode(NodeBase):
    """Merger node, which means the node is a merger."""

    def __init__(self, name):
        pass

    def process(self, data, context):
        pass
