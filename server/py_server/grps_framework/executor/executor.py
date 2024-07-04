# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  Executor to run complete infer, including outlier, converter, dag, model infer and etc.
import time

from grps_framework.apis.grps_pb2 import GrpsMessage
from grps_framework.batching.batcher import DynamicBatcher
from grps_framework.conf.conf import global_conf
from grps_framework.context.context import GrpsContext
from grps_framework.converter.converter import converter_register
from grps_framework.dag.dag import SequentialDag
from grps_framework.logger.logger import logger
from grps_framework.model_infer.inferer import inferer_register


class Executor(object):
    def __init__(self):
        self.__outliers = {}
        self.__model_dict = {}
        self.__init_models()
        self.__dag = None
        self.__init_dag()

    def __init_models(self):
        """Init models."""
        logger.info('Init models.')
        used_customized_inferer = set()
        used_customized_converter = set()
        for model_desc in global_conf.inference_conf['models']:
            model_name = model_desc.get('name', '')
            if not model_name:
                logger.critical('Model name not specified.')
                raise ValueError('Model name not specified.')
            model_version = model_desc.get('version', '')
            if not model_version:
                logger.critical('Model version not specified.')
                raise ValueError('Model version not specified.')
            true_model_name = model_name + '-' + model_version

            if true_model_name in self.__model_dict:
                logger.critical('Model {} already exists.'.format(true_model_name))
                raise ValueError('Model {} already exists.'.format(true_model_name))

            model_path = model_desc.get('inferer_path', '')
            model_device = str(model_desc.get('device', ''))
            inp_device = str(model_desc.get('inp_device', ''))

            inferer_type = model_desc.get('inferer_type', '')
            if not inferer_type:
                logger.critical('Model inferer type not specified.')
                raise ValueError('Model inferer type not specified.')

            # Get model inferer.
            if inferer_type == 'torch':
                from grps_framework.model_infer.torch_inferer import TorchModelInferer
                model_infer = TorchModelInferer()
                inferer_name = 'torch'
            elif inferer_type == 'tensorflow':
                from grps_framework.model_infer.tf_inferer import TfModelInferer
                model_infer = TfModelInferer()
                inferer_name = 'tensorflow'
            elif inferer_type == 'tensorrt':
                from grps_framework.model_infer.trt_inferer import TrtModelInferer
                model_infer = TrtModelInferer()
                inferer_name = 'tensorrt'
            elif inferer_type == 'customized':
                inferer_name = model_desc.get('inferer_name')
                if not inferer_name:
                    logger.critical('Inferer name must be specified when model type is customized.')
                    raise ValueError('Inferer name must be specified when model type is customized.')
                if inferer_name not in inferer_register.model_inferer_dict:
                    logger.critical('Inferer name {} not found, but bound with {} model.'
                                    .format(inferer_name, true_model_name))
                    raise ValueError('Inferer name {} not found, but bound with {} model.'
                                     .format(inferer_name, true_model_name))
                if inferer_name in used_customized_inferer:
                    # Has been used, clone a new one.
                    model_infer = type(inferer_register.get_model_inferer(inferer_name))()
                else:
                    model_infer = inferer_register.get_model_inferer(inferer_name)
                    used_customized_inferer.add(inferer_name)
            else:
                # TODO: Add more model types.
                logger.critical('Inferer type {} not supported.'.format(inferer_type))
                raise ValueError('Inferer type {} not supported.'.format(inferer_type))
            model_device = model_device.lower()
            if model_device == 'original' and inferer_type == 'torch':
                model_device = model_device + '_' + inp_device
            model_device = model_device.lower()
            inferer_args = model_desc.get('inferer_args', dict())
            model_infer.init(model_path, model_device, inferer_args)
            logger.info(
                'Init model inferer {} successfully, path: {}, device: {}, args: {}'.format(
                    inferer_name, model_path, model_device, inferer_args))
            # Load model inferer.
            if not model_infer.load():
                logger.critical('Load model inferer {} failed.'.format(inferer_name))
                raise RuntimeError('Load model inferer {} failed.'.format(inferer_name))
            logger.info('Load model inferer {} successfully.'.format(inferer_name))

            converter_type = model_desc.get('converter_type', '')
            if not converter_type:
                logger.critical('Model converter type not specified.')
                raise ValueError('Model converter type not specified.')
            converter_path = model_desc.get('converter_path', '')

            # Get converter.
            if converter_type == 'torch':
                from grps_framework.converter.torch_tensor_converter import TorchTensorConverter
                converter = TorchTensorConverter()
                converter_name = 'torch'
            elif converter_type == 'tensorflow':
                from grps_framework.converter.tf_tensor_converter import TfTensorConverter
                converter = TfTensorConverter()
                converter_name = 'tensorflow'
            elif converter_type == 'tensorrt':
                from grps_framework.converter.trt_tensor_converter import TrtTensorConverter
                converter = TrtTensorConverter()
                converter_name = 'tensorrt'
            elif converter_type == 'none':
                converter = None
                converter_name = 'none'
            elif converter_type == 'customized':
                converter_name = model_desc.get('converter_name')
                if not converter_name:
                    logger.critical('Converter name must be specified when model type is customized.')
                    raise ValueError('Converter name must be specified when model type is customized.')
                if converter_name not in converter_register.converter_dict:
                    logger.critical('Converter name {} not found, but bound with {} model.'
                                    .format(converter_name, true_model_name))
                    raise ValueError('Converter name {} not found, but bound with {} model.'
                                     .format(converter_name, true_model_name))
                if converter_name in used_customized_converter:
                    converter = type(converter_register.get_converter(converter_name))()
                else:
                    converter = converter_register.get_converter(converter_name)
                    used_customized_converter.add(converter_name)
            else:
                # TODO: Add more converter types.
                logger.critical('Converter type {} not supported.'.format(converter_type))
                raise ValueError('Converter type {} not supported.'.format(converter_type))
            if converter is not None:
                converter_args = model_desc.get('converter_args', dict())
                converter.init(converter_path, converter_args)
                logger.info('Init converter {} successfully, path: {}, args: {}'.format(converter_name,
                                                                                        converter_path,
                                                                                        converter_args))

            batcher = None
            batcher_name = ''
            max_batch_size = 0
            batch_timeout_us = 0
            batching = model_desc.get('batching', '')
            if batching:
                batching_type = batching.get('type', '')
                if batching_type == 'none':
                    batcher_name = 'none'
                elif batching_type == 'dynamic':
                    batcher = DynamicBatcher()
                    batcher_name = 'dynamic'
                else:
                    logger.critical('Batching type {} not supported.'.format(batching_type))
                    raise ValueError('Batching type {} not supported.'.format(batching_type))

                max_batch_size = batching.get('max_batch_size')
                if not max_batch_size:
                    logger.critical('Max batch size not specified.')
                    raise ValueError('Max batch size not specified.')
                if type(max_batch_size) != int:
                    logger.critical('Max batch size must be int.')
                    raise ValueError('Max batch size must be int.')
                if max_batch_size <= 0:
                    logger.critical('Max batch size must be positive.')
                    raise ValueError('Max batch size must be positive.')
                batch_timeout_us = batching.get('batch_timeout_us')
                if not batch_timeout_us:
                    logger.critical('Batch timeout not specified.')
                    raise ValueError('Batch timeout not specified.')
                if type(batch_timeout_us) != int:
                    logger.critical('Batch timeout must be int.')
                    raise ValueError('Batch timeout must be int.')
                if batch_timeout_us <= 0:
                    logger.critical('Batch timeout must be positive.')
                    raise ValueError('Batch timeout must be positive.')

            if batcher is not None:
                batcher.init(true_model_name, max_batch_size, batch_timeout_us, converter, model_infer)
                batcher.start()
                logger.info('Init and start batcher {} successfully, max batch size: {}, batch timeout: {} us.'.format(
                    batcher_name, max_batch_size, batch_timeout_us))
            self.__model_dict[true_model_name] = (model_infer, converter, batcher)
            logger.info('Init model {} successfully, inferer: {}, converter: {}, batcher: {}.'.format(
                true_model_name, inferer_name, converter_name, batcher_name))

    def __init_dag(self):
        """Init dag."""
        logger.info('Init dag.')
        dag_conf = global_conf.inference_conf['dag']
        dag_type = dag_conf['type']
        if dag_type == 'sequential':
            self.__dag = SequentialDag(dag_conf['name'])
            self.__dag.build_dag(dag_conf['nodes'], self.__model_dict)
            pass
        else:
            # TODO: Add more dag types.
            raise ValueError('Dag type {} not supported.'.format(dag_type))

        logger.info('Init dag successfully.')

    def infer_with_model_name(self, inp: GrpsMessage, context: GrpsContext, model_name=None):
        if model_name not in self.__model_dict:
            raise RuntimeError('Model {} not found.'.format(model_name))

        (model_inferer, converter, batcher) = self.__model_dict[model_name]
        context.set_converter(converter)
        context.set_inferer(model_inferer)

        if batcher:
            return batcher.infer(inp, context)

        begin = time.time()
        if converter is None:
            infer_in = inp
            infer_out = model_inferer.infer(infer_in, context)
            # logger.debug('Infer_input: {}, infer_out: {}'.format(infer_input, infer_out))
            infer_time = time.time()
            if context.has_err():
                return

            logger.info(
                'Model({}), model_infer time: {:.0f} us'.format(model_name, (infer_time - begin) * 1e6))
            return infer_out
        else:
            infer_input = converter.preprocess(inp, context)
            preprocess_time = time.time()
            if context.has_err():
                return

            infer_out = model_inferer.infer(infer_input, context)
            # logger.debug('Infer_input: {}, infer_out: {}'.format(infer_input, infer_out))
            infer_time = time.time()
            if context.has_err():
                return

            out = converter.postprocess(infer_out, context)
            postprocess_time = time.time()
            if context.has_err():
                return

            logger.info(
                'Model({}), preprocess time: {:.0f} us, model_infer time: {:.0f} us, postprocess time: {:.0f} us'
                .format(model_name, (preprocess_time - begin) * 1e6, (infer_time - preprocess_time) * 1e6,
                        (postprocess_time - infer_time) * 1e6))
            return out

    def infer(self, inp: GrpsMessage, context: GrpsContext, model_name=None):
        """
        Run complete infer.
        Args:
            context: grps context
            inp: request inp.
            model_name: model name(with `name-version` format) to predict. If not define, will use default model dag
            (defined in inference.yml) to predict.

        Returns:
            Output will be returned to client.
        """
        try:
            if self.__dag is None:
                raise RuntimeError('Dag not initialized.')
            if model_name is None or model_name == '':
                out = self.__dag.infer(inp, context)
            else:
                out = self.infer_with_model_name(inp, context, model_name)
            return out
        except Exception as e:
            raise
        finally:
            if context._grpc_streaming_run:
                # Stop grpc streaming generator if grpc streaming.
                context.stop_grpc_streaming_generator()
