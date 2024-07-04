# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  DAG of inference.

from abc import ABCMeta, abstractmethod

from grps_framework.context.context import GrpsContext
from grps_framework.dag.node import ModelNode
from grps_framework.logger.logger import logger


class InferDag(metaclass=ABCMeta):
    """Infer DAG base class."""

    @abstractmethod
    def __init__(self, name):
        self._name = name
        pass

    @abstractmethod
    def build_dag(self, nodes_conf, model_dict):
        pass

    @abstractmethod
    def infer(self, data, context):
        pass

    @property
    def name(self):
        return self._name


class SequentialDag(InferDag):
    """Sequential DAG, which means the models will be executed in sequence."""

    def __init__(self, name):
        super().__init__(name)
        self.__sequence = []
        pass

    def build_dag(self, nodes_conf, model_dict):
        """
        Build dag.
        Args:
            nodes_conf: nodes conf.
            model_dict: model dict with key is model name and val is (model_infer, converter, batcher).

        Returns:
            True if build dag successfully, else False.
        """
        logger.info('Build sequential dag.')
        for node in nodes_conf:
            node_name = node['name']
            node_type = node['type']
            if node_type == 'model':
                node_model = node['model']
                if node_model not in model_dict:
                    raise ValueError('Model {} not found but bound with {} node.'.format(node_model, node_name))
                self.__sequence.append(ModelNode(node_name, *model_dict[node_model]))
            else:
                # TODO: Add splitter and merger and etc node type.
                raise ValueError('Node type {} not supported in sequential dag.'.format(node_type))
        seq_str = ' -> '.join([str(node) for node in self.__sequence])
        logger.info('Build sequential dag successfully, sequence is {}.'.format(seq_str))
        return True

    def infer(self, data, context: GrpsContext):
        """
        Sequential dag infer.
        Args:
            context: grps context
            data: input data.

        Returns:
            output will be request response.
        """
        logger.debug('Sequential dag infer.')
        for node in self.__sequence:
            data = node.process(data, context)
            if context.has_err():
                return
        logger.debug('Sequential dag infer successfully.')
        return data


# TODO: Add graph DAG.
class GraphDag(InferDag):
    """Graph DAG, which means the models will be executed in graph."""

    def __init__(self, name):
        super().__init__(name)
        pass

    def build_dag(self, nodes_conf, model_dict):
        pass

    def infer(self, data):
        pass
