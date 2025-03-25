# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/5/15
# Brief  Grps tensorflow or torch serve tool for tf or torch model quick deploy without create, archive.
import os
import re
import shutil
import signal
import socket
import tarfile

import yaml
from common import utils
from common.utils import restore_permission
from const import GRPS_TF_SERVE_PROJECT_NAME, GRPS_TORCH_SERVE_PROJECT_NAME, GRPS_TRT_SERVE_PROJECT_NAME


class GrpsQuickServe(object):
    """Grps tensorflow or torch serve tool for tf or torch model quick deploy without create, archive."""

    def __init__(self):
        """Init."""
        self.__grps_server_work_dir = None

    def build_parser(self, subparsers):
        """
        Build parser for tf_serve sub command.

        Args:
            subparsers: sub parser.
        """
        parser_deploy = subparsers.add_parser('tf_serve',
                                              help='quick deploy tensorflow model grps server without create, archive')
        parser_deploy.add_argument('--name', type=str, help='server name, default is \"my_grps\"', default='my_grps')
        parser_deploy.add_argument('--interface_framework', type=str,
                                   help='interface framework, should be one of \"http\", \"http+grpc\", \"http+brpc\",'
                                        ' default is \"http+grpc\"',
                                   default='http+grpc',
                                   choices=['http', 'http+grpc', 'http+brpc'])
        parser_deploy.add_argument('--port', type=str,
                                   help='http port and rpc port split by comma, default is \"7080,7081\"',
                                   default="7080,7081")
        parser_deploy.add_argument('--customized_predict_http_path', type=str,
                                   help='customized predict http path, default is empty and will not use default path',
                                   default='')
        parser_deploy.add_argument('model_path', type=str,
                                   help='tensorflow saved model path, only support saved model format')
        parser_deploy.add_argument('--device', type=str,
                                   help='device of model, should be one of "cpu", "gpu"(same as cuda:0), "cuda"(same as'
                                        ' cuda:0), "cuda:[num]", "gpu:[num]" or "original"(original device specified'
                                        ' when exported model), default is "cuda"',
                                   default='cuda')
        parser_deploy.add_argument('--batching_type', type=str,
                                   help='batching type, should be one of "none", or "dynamic", default is "none"',
                                   default='none')
        parser_deploy.add_argument('--max_batch_size', type=int,
                                   help='batching max batch size, default is 16. Useless when batching_type is "none"',
                                   default=16)
        parser_deploy.add_argument('--batch_timeout_us', type=int,
                                   help='timeout(us) to assemble batch, default is 1000. Useless when batching_type'
                                        ' is "none"',
                                   default=1000)
        parser_deploy.add_argument('--max_connections', type=int, help='server max connections limit, default is 1000',
                                   default=1000)
        parser_deploy.add_argument('--max_concurrency', type=int, help='server max concurrency limit, default is 32',
                                   default=32)
        parser_deploy.add_argument('--gpu_devices_idx', type=str,
                                   help='devices idx(split by comma) that will be limited and monitored, cannot be none'
                                        ' when "gpu_mem_limit_mib" is not -1, default is "0"',
                                   default='0')
        parser_deploy.add_argument('--gpu_mem_limit_mib', type=int,
                                   help='server gpu memory limit(MIB), default is -1, means no limit',
                                   default=-1)
        parser_deploy.add_argument('--customized_op_paths', type=str,
                                   help='customized op paths split by comma, default is empty',
                                   default='')
        parser_deploy.add_argument('--log_dir', type=str,
                                   help='log dir. will be subdir of deploy path if is relative path',
                                   default='./logs')
        parser_deploy.add_argument('--log_backup_count', type=int, help='log backup count, default is 7', default=7)
        parser_deploy.add_argument('--output_path', type=str,
                                   help='project will be archived to this path, default is empty and will not archive',
                                   default='')
        parser_deploy.add_argument('--timeout', type=int, help='server start timeout, default is 300s', default=300)
        parser_deploy.add_argument('--no_logging', action='store_true', help='not trace logs after start.')
        parser_deploy.set_defaults(func=self.tf_serve)  # Set default function.

        parser_deploy = subparsers.add_parser('torch_serve',
                                              help='quick deploy torch model grps server without create, archive')
        parser_deploy.add_argument('--name', type=str, help='server name, default is \"my_grps\"', default='my_grps')
        parser_deploy.add_argument('--interface_framework', type=str,
                                   help='interface framework, should be one of \"http\", \"http+grpc\", \"http+brpc\",'
                                        ' default is \"http+grpc\"',
                                   default='http+grpc',
                                   choices=['http', 'http+grpc', 'http+brpc'])
        parser_deploy.add_argument('--port', type=str,
                                   help='http port and rpc port split by comma, default is \"7080,7081\"',
                                   default="7080,7081")
        parser_deploy.add_argument('--customized_predict_http_path', type=str,
                                   help='customized predict http path, default is empty and will not use default path',
                                   default='')
        parser_deploy.add_argument('model_path', type=str,
                                   help='torch saved model path, only support torch script model format')
        parser_deploy.add_argument('--device', type=str,
                                   help='device of model, should be one of "cpu", "gpu"(same as cuda:0), "cuda"(same as'
                                        ' cuda:0), "cuda:[num]", "gpu:[num]" or "original"(original device specified'
                                        ' when exported model), default is "cuda"',
                                   default='cuda')
        parser_deploy.add_argument('--inp_device', type=str,
                                   help='device of input, should be one of "cpu", "gpu"(same as cuda:0), "cuda"(same as'
                                        ' cuda:0), "cuda:[num]" or "gpu:[num]". Cannot be none when "--device" is'
                                        ' "original"',
                                   default='')
        parser_deploy.add_argument('--batching_type', type=str,
                                   help='batching type, should be one of "none", or "dynamic", default is "none"',
                                   default='none')
        parser_deploy.add_argument('--max_batch_size', type=int,
                                   help='batching max batch size, default is 16. Useless when batching_type is "none"',
                                   default=16)
        parser_deploy.add_argument('--batch_timeout_us', type=int,
                                   help='timeout(us) to assemble batch, default is 1000. Useless when batching_type'
                                        ' is "none"',
                                   default=1000)
        parser_deploy.add_argument('--max_connections', type=int, help='server max connections limit, default is 1000',
                                   default=1000)
        parser_deploy.add_argument('--max_concurrency', type=int, help='server max concurrency limit, default is 32',
                                   default=32)
        parser_deploy.add_argument('--gpu_devices_idx', type=str,
                                   help='devices idx(split by comma) that will be limited and monitored, cannot be none'
                                        ' when "gpu_mem_limit_mib" is not -1, default is "0"',
                                   default='0')
        parser_deploy.add_argument('--gpu_mem_limit_mib', type=int,
                                   help='server gpu memory limit(MIB), default is -1, means no limit',
                                   default=-1)
        parser_deploy.add_argument('--gpu_mem_gc_enable', action='store_const', const=True,
                                   help='set this flag to enable server gpu memory gc, default not enable',
                                   default=False)
        parser_deploy.add_argument('--gpu_mem_gc_interval', type=int,
                                   help='server gpu memory gc interval, default is 60s',
                                   default=60)
        parser_deploy.add_argument('--customized_op_paths', type=str,
                                   help='customized op paths split by comma, default is empty',
                                   default='')
        parser_deploy.add_argument('--log_dir', type=str,
                                   help='log dir. will be subdir of deploy path if is relative path',
                                   default='./logs')
        parser_deploy.add_argument('--log_backup_count', type=int, help='log backup count, default is 7', default=7)
        parser_deploy.add_argument('--output_path', type=str,
                                   help='project will be archived to this path, default is empty and will not archive',
                                   default='')
        parser_deploy.add_argument('--timeout', type=int, help='server start timeout, default is 300s', default=300)
        parser_deploy.add_argument('--no_logging', action='store_true', help='not trace logs after start.')
        parser_deploy.set_defaults(func=self.torch_serve)  # Set default function.

        parser_deploy = subparsers.add_parser('trt_serve',
                                              help='quick deploy tensorrt model grps server without create, archive')
        parser_deploy.add_argument('--name', type=str, help='server name, default is \"my_grps\"', default='my_grps')
        parser_deploy.add_argument('--interface_framework', type=str,
                                   help='interface framework, should be one of \"http\", \"http+grpc\", \"http+brpc\",'
                                        ' default is \"http+grpc\"',
                                   default='http+grpc',
                                   choices=['http', 'http+grpc', 'http+brpc'])
        parser_deploy.add_argument('--port', type=str,
                                   help='http port and rpc port split by comma, default is \"7080,7081\"',
                                   default="7080,7081")
        parser_deploy.add_argument('--customized_predict_http_path', type=str,
                                   help='customized predict http path, default is empty and will not use default path',
                                   default='')
        parser_deploy.add_argument('model_path', type=str,
                                   help='trt engine path.')
        parser_deploy.add_argument('--device', type=str,
                                   help='device of model, should be one of "gpu"(same as cuda:0), "cuda"(same as'
                                        ' cuda:0), "cuda:[num]", "gpu:[num]" or "original"(original device specified'
                                        ' when exported model), default is "cuda"',
                                   default='cuda')
        parser_deploy.add_argument('--streams', type=int,
                                   help='streams count of trt inferer that can be used for parallel inference,'
                                        ' should be greater than 0, default is 1',
                                   default=1)
        parser_deploy.add_argument('--batching_type', type=str,
                                   help='batching type, should be one of "none", or "dynamic", default is "none"',
                                   default='none')
        parser_deploy.add_argument('--max_batch_size', type=int,
                                   help='batching max batch size, default is 16. Useless when batching_type is "none", '
                                        'Should not be greater than the max batch size of the trt engine',
                                   default=16)
        parser_deploy.add_argument('--batch_timeout_us', type=int,
                                   help='timeout(us) to assemble batch, default is 1000. Useless when batching_type'
                                        ' is "none"',
                                   default=1000)
        parser_deploy.add_argument('--max_connections', type=int, help='server max connections limit, default is 1000',
                                   default=1000)
        parser_deploy.add_argument('--max_concurrency', type=int, help='server max concurrency limit, default is 32',
                                   default=32)
        parser_deploy.add_argument('--gpu_devices_idx', type=str,
                                   help='devices idx(split by comma) will be monitored, default is "0"',
                                   default='0')
        parser_deploy.add_argument('--customized_op_paths', type=str,
                                   help='customized op paths split by comma, default is empty',
                                   default='')
        parser_deploy.add_argument('--log_dir', type=str,
                                   help='log dir. will be subdir of deploy path if is relative path',
                                   default='./logs')
        parser_deploy.add_argument('--log_backup_count', type=int, help='log backup count, default is 7', default=7)
        parser_deploy.add_argument('--output_path', type=str,
                                   help='project will be archived to this path, default is empty and will not archive',
                                   default='')
        parser_deploy.add_argument('--timeout', type=int, help='server start timeout, default is 300s', default=300)
        parser_deploy.add_argument('--no_logging', action='store_true', help='not trace logs after start.')
        parser_deploy.set_defaults(func=self.trt_serve)  # Set default function.

    def __gen_conf(self, args, model_type, build_conf_path, run_conf_path):
        """Generate build and run conf."""
        with open(build_conf_path, 'r') as f:
            build_conf = yaml.load(f, Loader=yaml.FullLoader)
        with open(run_conf_path, 'r') as f:
            run_conf = yaml.load(f, Loader=yaml.FullLoader)

        # modify build conf from args.
        if not os.path.isabs(args.model_path):  # to absolute path if is relative path.
            args.model_path = os.path.abspath(args.model_path)
        build_conf['models'][0]['inferer_path'] = args.model_path
        if model_type == 'torch':
            build_conf['models'][0]['device'] = args.device
            build_conf['models'][0]['inp_device'] = args.inp_device
            build_conf['models'][0]['inferer_type'] = 'torch'
            build_conf['models'][0]['converter_type'] = 'torch'
        elif model_type == 'tensorflow':
            build_conf['models'][0]['device'] = args.device
            build_conf['models'][0]['inferer_type'] = 'tensorflow'
            build_conf['models'][0]['converter_type'] = 'tensorflow'
        elif model_type == 'tensorrt':
            build_conf['models'][0]['device'] = args.device
            build_conf['models'][0]['inferer_type'] = 'tensorrt'
            build_conf['models'][0]['converter_type'] = 'tensorrt'
            if args.streams <= 0:
                print('streams should be greater than 0.')
                return False
            build_conf['models'][0]['inferer_args']['streams'] = args.streams
        else:
            print('model type {} not support'.format(model_type))
            return False
        if args.customized_op_paths:
            build_conf['models'][0]['inferer_args']['customized_op_paths'] = []
            for path in args.customized_op_paths.split(','):
                build_conf['models'][0]['inferer_args']['customized_op_paths'].append(os.path.abspath(path))
        build_conf['models'][0]['batching']['type'] = args.batching_type
        build_conf['models'][0]['batching']['max_batch_size'] = args.max_batch_size
        build_conf['models'][0]['batching']['batch_timeout_us'] = args.batch_timeout_us

        # modify run conf from args.
        run_conf['interface']['framework'] = args.interface_framework
        run_conf['interface']['port'] = args.port
        if args.customized_predict_http_path:
            customized_predict_http = {
                'path': args.customized_predict_http_path,
                'customized_body': False
            }
            run_conf['interface']['customized_predict_http'] = customized_predict_http
        run_conf['max_connections'] = args.max_connections
        run_conf['max_concurrency'] = args.max_concurrency
        run_conf['gpu'] = dict()
        if model_type == 'torch':
            run_conf['gpu']['mem_manager_type'] = model_type
            run_conf['gpu']['mem_limit_mib'] = args.gpu_mem_limit_mib
            run_conf['gpu']['mem_gc_enable'] = args.gpu_mem_gc_enable
            run_conf['gpu']['mem_gc_interval'] = args.gpu_mem_gc_interval
        elif model_type == 'tensorflow':
            run_conf['gpu']['mem_manager_type'] = model_type
            run_conf['gpu']['mem_limit_mib'] = args.gpu_mem_limit_mib
            run_conf['gpu']['mem_gc_enable'] = False
            run_conf['gpu']['mem_gc_interval'] = 60
        elif model_type == 'tensorrt':
            run_conf['gpu']['mem_manager_type'] = 'none'
        # check gpu_devices_idx: 1,2 or 1
        if re.match(r'^\d+(,\d+)*$', args.gpu_devices_idx) is None:
            print('Invalid gpu_devices_idx: {}'.format(args.gpu_devices_idx))
            return False
        run_conf['gpu']['devices'] = [int(i) for i in args.gpu_devices_idx.split(',')]
        run_conf['log']['log_dir'] = args.log_dir
        run_conf['log']['log_backup_count'] = args.log_backup_count

        # write build and run conf.
        with open(build_conf_path, 'w') as f:
            yaml.dump(build_conf, f)
        with open(run_conf_path, 'w') as f:
            yaml.dump(run_conf, f)
        return True

    def __check_build_conf(self, conf_path):
        """
        Check build conf.

        Args:
            conf_path: Build conf path.

        Returns:
            True if check passed.
        """
        with open(conf_path, 'r', encoding='utf-8') as reader:
            inference_conf = yaml.load(reader, Loader=yaml.FullLoader)
            # print(inference_conf)
            if not inference_conf:
                print('inference conf is empty.')
                return False

        # 1. Check models conf.
        models = []
        models_conf = inference_conf.get('models')
        if not models_conf:
            print('models not set in inference conf.')
            return False
        if len(models_conf) == 0:
            print('models is empty in inference conf.')
            return False
        for model_conf in models_conf:
            # model name
            model_name = model_conf.get('name')
            if not model_name:
                print('model name not set in model conf.')
                return False

            # model version
            model_version = model_conf.get('version')
            if not model_version:
                print('model version not set in model conf.')
                return False

            # model type
            inferer_type = model_conf.get('inferer_type')
            if not inferer_type:
                print('model type not set in model conf.')
                return False
            if inferer_type not in ['torch', 'tensorflow', 'tensorrt', 'customized']:
                print('inferer_type {} not supported, should be torch, tensorflow, tensorrt or customized.'
                      .format(inferer_type))
                return False
            if inferer_type == 'customized':
                inferer_name = model_conf.get('inferer_name')
                if not inferer_name:
                    print('inferer_name not set in model conf.')
                    return False

            # model converter
            converter_type = model_conf.get('converter_type')
            if not converter_type:
                print('converter_type not set in model conf.')
                return False
            if converter_type not in ['torch', 'tensorflow', 'tensorrt', 'customized', 'none']:
                print('converter_type {} not supported, should be torch, tensorflow, tensorrt, '
                      'customized or none'.format(converter_type))
                return False
            if converter_type == 'customized':
                converter_name = model_conf.get('converter_name')
                if not converter_name:
                    print('converter_name not set in model conf.')
                    return False

            # model device
            model_device = model_conf.get('device')
            if not model_device:
                print('model device not set in model conf.')
                return False
            if model_device not in ['cpu', 'cuda', 'gpu', 'original'] and not re.match(r'^cuda:\d+$', model_device) \
                    and not re.match(r'^gpu:\d+$', model_device):
                print(
                    'model device {} not supported, should be cpu, cuda, gpu, cuda:[num] or original.'.format(
                        model_device))
                return False
            if model_device == 'original' and inferer_type == 'torch':
                inp_device = model_conf.get('inp_device')
                if not inp_device:
                    print('inp device not set in model conf.')
                    return False
                if inp_device not in ['cpu', 'cuda', 'gpu'] and not re.match(r'^cuda:\d+$', inp_device) \
                        and not re.match(r'^gpu:\d+$', inp_device):
                    print('inp device {} not supported, should be cpu, cuda, gpu or cuda:[num].'.format(
                        inp_device))
                    return False

            # batching config
            batching = model_conf.get('batching')
            if batching:
                batch_type = batching.get('type')
                if not batch_type:
                    print('batching type not set in model conf.')
                    return False
                if batch_type not in ['none', 'dynamic']:
                    print('batching type {} not supported, should be none or dynamic.'.format(batch_type))
                    return False
                max_batch_size = batching.get('max_batch_size')
                if not max_batch_size:
                    print('batching max_batch_size not set in model conf.')
                    return False
                if type(max_batch_size) != int:
                    print('batching max_batch_size should be int.')
                    return False
                if max_batch_size <= 0:
                    print('batching max_batch_size should be greater than 0.')
                    return False
                batch_timeout_us = batching.get('batch_timeout_us')
                if not batch_timeout_us:
                    print('batching batch_timeout_us not set in model conf.')
                    return False
                if type(batch_timeout_us) != int:
                    print('batching batch_timeout_us should be int.')
                    return False
                if batch_timeout_us <= 0:
                    print('batching batch_timeout_us should be greater than 0.')
                    return False

            models.append(model_name + '-' + model_version)

        # 2. Check dag conf.
        dag_conf = inference_conf.get('dag')
        if not dag_conf:
            print('dag not set in inference conf.')
            return False
        dag_type = dag_conf.get('type')
        if not dag_type:
            print('dag type not set in dag conf.')
            return False
        # TODO: Add more dag type.
        if dag_type not in ['sequential']:
            print('dag type {} not supported, only support sequential now.'.format(dag_type))
            return False
        nodes = dag_conf.get('nodes')
        if len(nodes) == 0:
            print('nodes is empty in dag conf.')
            return False
        for node in nodes:
            node_name = node.get('name')
            if not node_name:
                print('node name not set in node conf.')
                return False
            node_type = node.get('type')
            if not node_type:
                print('node type not set in node conf.')
                return False
            # TODO: Add more node type.
            if node_type not in ['model']:
                print('node type {} not supported, only support model now.'.format(node_type))
                return False
            if node_type == 'model':
                model = node.get('model')
                if not model:
                    print('model not set in node conf.')
                    return False
                if model not in models:
                    print('node model {} not in models({}).'.format(model, models))
                    return False

        return True

    def __check_run_conf(self, run_conf_path):
        """Check run conf."""
        with open(run_conf_path, 'r', encoding='utf-8') as reader:
            server_conf = yaml.load(reader, Loader=yaml.FullLoader)

        def if_occupied(ip, port):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                s.connect((ip, int(port)))
                s.shutdown(2)
                return True
            except:
                return False

        # 1. Check interface conf
        interface_conf = server_conf.get('interface')
        if not interface_conf:
            print('interface conf not exists.')
            return False
        interface_fw_conf = interface_conf.get('framework')
        if not interface_fw_conf:
            print('interface framework conf not exists.')
            return False
        if interface_fw_conf not in ['http', 'http+brpc', 'http+grpc']:
            print('interface framework({}) not in supported framework({}).'
                  .format(interface_fw_conf, ', '.join(['http', 'http+brpc', 'http+grpc'])))
            return False
        interface_host_conf = interface_conf.get('host')
        if not interface_host_conf:
            print('interface host conf not exists.')
            return False
        # Check ip format
        if re.match(r'^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$', interface_host_conf) is None:
            print('interface host({}) format error.'.format(interface_host_conf))
            return False
        # Check port.
        interface_port_conf = interface_conf.get('port')
        if not interface_port_conf:
            print('interface port conf not exists.')
            return False
        port_conf = str(interface_port_conf).replace(' ', '')
        ports = port_conf.split(',')
        for port in ports:
            if not port.isdigit():
                print('Invalid port: {}'.format(port))
                return False
            # Check port if occupied
            if if_occupied(interface_host_conf, port):
                print('Port({}) is occupied.'.format(port))
                return False

        # Check customized_predict_http.
        customized_predict_http_conf = interface_conf.get('customized_predict_http')
        if customized_predict_http_conf:
            customized_predict_http_path = customized_predict_http_conf.get('path')
            if not customized_predict_http_path:
                print('Customized predict http path conf not exists.')
                return False
            if not re.match(r'^/[a-zA-Z0-9_\-/]+$', customized_predict_http_path):
                print('Invalid customized predict http path: {}'.format(
                    customized_predict_http_path))
                return False
            if customized_predict_http_path in ['/grps/v1/infer/predict', '/grps/v1/health/online',
                                                '/grps/v1/health/offline', '/grps/v1/health/live',
                                                '/grps/v1/health/ready', '/grps/v1/metadata/server',
                                                '/grps/v1/metadata/model', '/grps/v1/js/jquery_min',
                                                '/grps/v1/js/flot_min', '/grps/v1/monitor/series',
                                                '/grps/v1/monitor/metrics', '/']:
                print('Invalid customized predict http path: {}, cannot use internal path.'.format(
                    customized_predict_http_path))
                return False
            customized_body = customized_predict_http_conf.get('customized_body')
            if type(customized_body) != bool:
                print('Customized predict http customized_body conf should be bool.')
                return False

        # 2. Check max_concurrency.
        max_connections_conf = server_conf.get('max_connections')
        if not max_connections_conf:
            print('max_connections conf not exists.')
            return False
        if type(max_connections_conf) != int:
            print('max_connections conf should be int.')
            return False
        max_concurrency_conf = server_conf.get('max_concurrency')
        if not max_concurrency_conf:
            print('max_concurrency conf not exists.')
            return False
        if type(max_concurrency_conf) != int:
            print('max_concurrency conf should be int.')
            return False
        if max_concurrency_conf > max_connections_conf:
            print('max_connections({}) should be greater than or equal to max_concurrency({}).'
                  .format(max_connections_conf, max_concurrency_conf))
            return False

        # 3. Check gpu conf.
        gpu_conf = server_conf.get('gpu')
        if gpu_conf:
            gpu_mem_manager_type = gpu_conf.get('mem_manager_type')
            if not gpu_mem_manager_type:
                print('[server.yml] gpu mem manager type conf not exists.')
                return False
            elif gpu_mem_manager_type not in ['torch', 'tensorflow', 'none']:
                print('[server.yml] gpu mem manager type({}) not in supported type({}).'
                      .format(gpu_mem_manager_type, ', '.join(['torch', 'tensorflow', 'none'])))
                return False
            elif gpu_mem_manager_type in ['torch', 'tensorflow']:
                gpu_mem_limit_mib = gpu_conf.get('mem_limit_mib')
                if not gpu_mem_limit_mib:
                    print('[server.yml] gpu mem limit conf not exists.')
                    return False
                if type(gpu_mem_limit_mib) != int:
                    print('[server.yml] gpu mem limit conf should be int.')
                    return False
                mem_gc_enable = gpu_conf.get('mem_gc_enable')
                if type(mem_gc_enable) != bool:
                    print('gpu mem gc enable conf should be bool.')
                    return False
                mem_gc_interval = gpu_conf.get('mem_gc_interval')
                if mem_gc_enable and (not mem_gc_interval or type(mem_gc_interval) != int or mem_gc_interval < 1):
                    print('gpu mem gc interval conf should be int and not less than 1.')
                    return False
            devices = gpu_conf.get('devices')
            if not devices:
                print('gpu limit devices idx not exists.')
                return False
            if type(devices) != list:
                print('gpu limit devices idx should be int list.')
                return False
            for device in devices:
                if type(device) != int:
                    print('gpu limit devices idx should be int list.')
                    return False

        # 4. Check log conf.
        log_conf = server_conf.get('log')
        if not log_conf:
            print('log conf not exists.')
            return False
        log_log_dir = log_conf.get('log_dir')
        if not log_log_dir:
            print('log log_dir conf not exists.')
            return False
        if os.path.exists(log_log_dir) and os.path.isfile(log_log_dir):
            print('log log_dir conf should be dir.')
            return False
        log_backup_count = log_conf.get('log_backup_count')
        if not log_backup_count:
            print('log log_backup_count conf not exists.')
            return False
        if type(log_backup_count) != int or log_backup_count < 1:
            print('log log_backup_count conf should be int and not less than 1.')
            return False

        return True

    def __prepare_project(self, args, model_type):
        """Prepare grps project."""
        shutil.rmtree(self.__grps_server_work_dir, ignore_errors=True)
        os.makedirs(self.__grps_server_work_dir)
        # Extract mar file.
        if model_type == 'torch':
            grps_quick_project_name = GRPS_TORCH_SERVE_PROJECT_NAME
        elif model_type == 'tensorflow':
            grps_quick_project_name = GRPS_TF_SERVE_PROJECT_NAME
        elif model_type == 'tensorrt':
            grps_quick_project_name = GRPS_TRT_SERVE_PROJECT_NAME
        else:
            print('Model type {} not support'.format(model_type))
            return False
        internal_mar_path = os.path.join(os.environ['GRPST_HOME'], grps_quick_project_name + '.mar')
        if not os.path.exists(internal_mar_path):
            print('Not support in current env because not installed.')
            return False
        tar = tarfile.open(internal_mar_path)
        tar.extractall(path=self.__grps_server_work_dir)
        tar.close()

        build_conf_path = os.path.join(self.__grps_server_work_dir, 'conf/inference.yml')
        run_conf_path = os.path.join(self.__grps_server_work_dir, 'conf/server.yml')

        # generate build and run conf from args.
        if not self.__gen_conf(args, model_type, build_conf_path, run_conf_path):
            return False
        if not self.__check_build_conf(build_conf_path):
            return False
        if not self.__check_run_conf(run_conf_path):
            return False
        return True

    def __start_server(self, timeout):
        """
        Start server.

        Args:
            timeout: start timeout.

        Returns
            True for success, False for failed.
        """
        os.chdir(self.__grps_server_work_dir)
        cmd_str = 'bash start_server.sh ' + str(timeout)
        ret = os.system(cmd_str)
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        return ret == 0

    def __serve(self, args, model_type):
        grps_worker_root = os.path.join(os.environ['HOME'], '.grps')
        self.__grps_server_work_dir = os.path.join(grps_worker_root, args.name)
        if not os.path.exists(self.__grps_server_work_dir):
            os.makedirs(self.__grps_server_work_dir)

        # Lock work dir.
        lock_file = self.__grps_server_work_dir + '.lock'
        lock_file_fd = utils.file_lock(lock_file)
        if not lock_file_fd:
            print('Server({}) has been locked. You can start another grps server with your server name defined'
                  ' by --name arg.'.format(args.name))
            return -1

        # Check server if has been started.
        if os.path.exists(os.path.join(self.__grps_server_work_dir, 'PID')):
            print('Server({}) has been started. You can start another grps server with your server name defined'
                  ' by --name arg. Also, you can stop current server with \"grpst stop\" cmd.<<<<'.format(args.name))
            utils.file_unlock(lock_file_fd, lock_file)
            return -1

        print('>>>> Starting grps server({})...'.format(args.name))

        print('>>>> Preparing grps project')
        if not self.__prepare_project(args, model_type):
            print('Prepare grps project failed')
            utils.file_unlock(lock_file_fd, lock_file)
            return -1
        print('>>>> Preparing grps project finished')

        if args.output_path:
            if os.path.exists(args.output_path):
                print('Output path({}) exists. Please remove it first.'.format(args.output_path))
                utils.file_unlock(lock_file_fd, lock_file)
                return -1
            print('>>>> Archiving project to ({})...'.format(args.output_path))
            os.system('tar zcvf {} -C {} .'.format(args.output_path, self.__grps_server_work_dir))
            print('>>>> Archive project to ({}) finished'.format(args.output_path))
            restore_permission(args.output_path)

        print('>>>> Starting grps server({})...'.format(args.name))
        if not self.__start_server(args.timeout):
            print('Start grps server({}) failed'.format(args.name))
            utils.file_unlock(lock_file_fd, lock_file)
            return -1
        # print('>>>> Start grps server({}) finished'.format(args.name))

        # Unlock work dir.
        utils.file_unlock(lock_file_fd, lock_file)
        return 0

    def tf_serve(self, args):
        read_fd, write_fd = os.pipe()

        # Start child process to deploy project to skip parent process SIGINT.
        pid = os.fork()
        if pid == 0:
            os.setsid()
            ret = self.__serve(args, 'tensorflow')
            os.write(write_fd, b'1' if ret == 0 else b'0')
            return ret

        # Wait child process to finish.
        try:
            ret = os.read(read_fd, 1)
            if ret == b'0':
                return -1
        except KeyboardInterrupt as e:
            return 0

        # Show server logs.
        # print('\n\n>>>> Showing server logs, stop showing by Ctrl+C.')
        signal.signal(signal.SIGINT, signal.SIG_DFL)
        if args.no_logging:
            return 0
        os.system('grpst logs {}'.format(args.name))
        return 0

    def torch_serve(self, args):
        read_fd, write_fd = os.pipe()

        # Start child process to deploy project to skip parent process SIGINT.
        pid = os.fork()
        if pid == 0:
            os.setsid()
            ret = self.__serve(args, 'torch')
            os.write(write_fd, b'1' if ret == 0 else b'0')
            return ret

        # Wait child process to finish.
        try:
            ret = os.read(read_fd, 1)
            if ret == b'0':
                return -1
        except KeyboardInterrupt as e:
            return 0

        # Show server logs.
        # print('\n\n>>>> Showing server logs, stop showing by Ctrl+C.')
        signal.signal(signal.SIGINT, signal.SIG_DFL)
        if args.no_logging:
            return 0
        os.system('grpst logs {}'.format(args.name))
        return 0

    def trt_serve(self, args):
        read_fd, write_fd = os.pipe()

        # Start child process to deploy project to skip parent process SIGINT.
        pid = os.fork()
        if pid == 0:
            os.setsid()
            ret = self.__serve(args, 'tensorrt')
            os.write(write_fd, b'1' if ret == 0 else b'0')
            return ret

        # Wait child process to finish.
        try:
            ret = os.read(read_fd, 1)
            if ret == b'0':
                return -1
        except KeyboardInterrupt as e:
            return 0

        # Show server logs.
        # print('\n\n>>>> Showing server logs, stop showing by Ctrl+C.')
        signal.signal(signal.SIGINT, signal.SIG_DFL)
        if args.no_logging:
            return 0
        os.system('grpst logs {}'.format(args.name))
        return 0
