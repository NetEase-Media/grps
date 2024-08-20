# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/4/10
# Brief  Project deploy tools.
import os.path
import re
import shutil
import signal
import socket
import tarfile

import yaml

from common import utils


class GrpsProjectDeployer(object):
    """Grps project deployer."""

    def __init__(self):
        """Init."""
        self.__grps_server_work_dir = ''  # Grps server work dir.

    def build_parser(self, subparsers):
        """
        Build parser for deploy sub command.

        Args:
            subparsers: sub parser.
        """
        parser_deploy = subparsers.add_parser('start', help='start project')
        parser_deploy.add_argument('--name', type=str, help='server name, default is \"my_grps\"', default='my_grps')
        parser_deploy.add_argument('--inference_conf', type=str,
                                   help='inference conf path, will use inference.yml in mar(mar_path arg) if not set',
                                   required=False)
        parser_deploy.add_argument('--server_conf', type=str,
                                   help='server conf path, will use server.yml in mar(mar_path arg) if not set',
                                   required=False)
        parser_deploy.add_argument('--timeout', type=int, help='server start timeout, default is 300s', default=300)
        parser_deploy.add_argument('--mpi_np', type=int, help='mpi process count, default is 1 and will not use mpi.',
                                   default=1)
        parser_deploy.add_argument('mar_path', type=str, help='model server archive path')
        parser_deploy.set_defaults(func=self.deploy)  # Set default function.

    def __check_inference_conf(self, inference_conf):
        """ Check inference conf. """
        # 1. Check models conf.
        models = []
        models_conf = inference_conf.get('models')
        if not models_conf:
            print('[inference.yml] models not set in inference conf.')
            return False
        if len(models_conf) == 0:
            print('[inference.yml] models is empty in inference conf.')
            return False
        for model_conf in models_conf:
            # model name
            model_name = model_conf.get('name')
            if not model_name:
                print('[inference.yml] model name not set in model conf.')
                return False

            # model version
            model_version = model_conf.get('version')
            if not model_version:
                print('[inference.yml] model version not set in model conf.')
                return False

            # model inferer type
            inferer_type = model_conf.get('inferer_type')
            if not inferer_type:
                print('[inference.yml] model type not set in model conf.')
                return False
            if inferer_type not in ['torch', 'tensorflow', 'tensorrt', 'customized']:
                print('[inference.yml] inferer_type {} not supported, should be torch, tensorflow, tensorrt or'
                      ' customized.'.format(inferer_type))
                return False
            if inferer_type == 'customized':
                inferer_name = model_conf.get('inferer_name')
                if not inferer_name:
                    print('[inference.yml] inferer_name not set in model conf.')
                    return False

            # model converter
            converter_type = model_conf.get('converter_type')
            if not converter_type:
                print('[inference.yml] converter_type not set in model conf.')
                return False
            if converter_type not in ['torch', 'tensorflow', 'tensorrt', 'customized', 'none']:
                print('[inference.yml] converter_type {} not supported, should be torch, tensorflow, tensorrt, '
                      'customized or none.'.format(converter_type))
                return False
            if converter_type == 'customized':
                converter_name = model_conf.get('converter_name')
                if not converter_name:
                    print('[inference.yml] converter_name not set in model conf.')
                    return False

            # model inferer path
            inferer_path = model_conf.get('inferer_path')
            if inferer_type in ['torch', 'tensorflow', 'tensorrt']:
                if not inferer_path:
                    print('[inference.yml] inferer_path not set in model conf.')
                    return False

            # model device
            model_device = model_conf.get('device')
            if not model_device:
                print('[inference.yml] model device not set in model conf.')
                return False

            # batching config
            batching = model_conf.get('batching')
            if batching:
                batch_type = batching.get('type')
                if not batch_type:
                    print('[inference.yml] model batching type not set in model conf.')
                    return False
                if batch_type not in ['none', 'dynamic']:
                    print('[inference.yml] model batching type {} not supported, should be none or dynamic.'.format(
                        batch_type))
                    return False
                max_batch_size = batching.get('max_batch_size')
                if not max_batch_size:
                    print('[inference.yml] model batching max_batch_size not set in model conf.')
                    return False
                if type(max_batch_size) != int:
                    print('[inference.yml] model batching max_batch_size should be int.')
                    return False
                if max_batch_size <= 0:
                    print('[inference.yml] model batching max_batch_size should be greater than 0.')
                    return False
                batch_timeout_us = batching.get('batch_timeout_us')
                if not batch_timeout_us:
                    print('[inference.yml] model batching batch_timeout_us not set in model conf.')
                    return False
                if type(batch_timeout_us) != int:
                    print('[inference.yml] model batching batch_timeout_us should be int.')
                    return False
                if batch_timeout_us <= 0:
                    print('[inference.yml] model batching batch_timeout_us should be greater than 0.')
                    return False

            models.append(model_name + '-' + model_version)

        # 2. Check dag conf.
        dag_conf = inference_conf.get('dag')
        if not dag_conf:
            print('[inference.yml] dag not set in inference conf.')
            return False
        dag_type = dag_conf.get('type')
        if not dag_type:
            print('[inference.yml] dag type not set in dag conf.')
            return False
        # TODO: Add more dag type.
        if dag_type not in ['sequential']:
            print('[inference.yml] dag type {} not supported, only support sequential now.'.format(dag_type))
            return False
        nodes = dag_conf.get('nodes')
        if len(nodes) == 0:
            print('[inference.yml] nodes is empty in dag conf.')
            return False
        for node in nodes:
            node_name = node.get('name')
            if not node_name:
                print('[inference.yml] node name not set in node conf.')
                return False
            node_type = node.get('type')
            if not node_type:
                print('[inference.yml] node type not set in node conf.')
                return False
            # TODO: Add more node type.
            if node_type not in ['model']:
                print('[inference.yml] node type {} not supported, only support model now.'.format(node_type))
                return False
            if node_type == 'model':
                model = node.get('model')
                if not model:
                    print('[inference.yml] model not set in node conf.')
                    return False
                if model not in models:
                    print('[inference.yml] node model {} not in models({}).'.format(model, models))
                    return False

        return True

    @staticmethod
    def __check_server_conf(server_conf):
        """Check server conf."""

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
            print('[server.yml] interface conf not exists.')
            return False
        interface_fw_conf = interface_conf.get('framework')
        if not interface_fw_conf:
            print('[server.yml] interface framework conf not exists.')
            return False
        if interface_fw_conf not in ['http', 'http+brpc', 'http+grpc']:
            print('[server.yml] interface framework({}) not in supported framework({}).'
                  .format(interface_fw_conf, ', '.join(['http', 'http+brpc', 'http+grpc'])))
            return False
        interface_host_conf = interface_conf.get('host')
        if not interface_host_conf:
            print('[server.yml] interface host conf not exists.')
            return False
        # Check ip format
        if re.match(r'^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$', interface_host_conf) is None:
            print('[server.yml] interface host({}) format error.'.format(interface_host_conf))
            return False
        # Check port.
        interface_port_conf = interface_conf.get('port')
        if not interface_port_conf:
            print('[server.yml] interface port conf not exists.')
            return False
        port_conf = str(interface_port_conf).replace(' ', '')
        ports = port_conf.split(',')
        for port in ports:
            if not port.isdigit():
                print('[server.yml] Invalid port: {}'.format(port))
                return False
            # Check port if occupied
            if if_occupied(interface_host_conf, port):
                print('Port({}) is occupied. You can change port in conf/server.yml and use it without re-archive by'
                      ' --server_conf.'.format(port))
                return False
        # Check customized_predict_http.
        customized_predict_http_conf = interface_conf.get('customized_predict_http')
        if customized_predict_http_conf:
            customized_predict_http_path = customized_predict_http_conf.get('path', '')
            if (not isinstance(customized_predict_http_path, str)) or len(customized_predict_http_path) == 0:
                print('[server.yml] Customized predict http path conf not str or empty.')
                return False
            if not re.match(r'^/[a-zA-Z0-9_\-/]+$', customized_predict_http_path):
                print('[server.yml] Invalid customized predict http path: {}'.format(customized_predict_http_path))
                return False
            if customized_predict_http_path in ['/grps/v1/infer/predict', '/grps/v1/health/online',
                                                '/grps/v1/health/offline', '/grps/v1/health/live',
                                                '/grps/v1/health/ready', '/grps/v1/metadata/server',
                                                '/grps/v1/metadata/model', '/grps/v1/js/jquery_min',
                                                '/grps/v1/js/flot_min', '/grps/v1/monitor/series',
                                                '/grps/v1/monitor/metrics', '/']:
                print('[server.yml] Invalid customized predict http path: {}, cannot use internal path.'.format(
                    customized_predict_http_path))
                return False
            customized_body = customized_predict_http_conf.get('customized_body')
            if type(customized_body) is not bool:
                print('[server.yml] Customized predict http customized_body conf should be bool.')
                return False

        # 2. Check max_concurrency.
        max_connections_conf = server_conf.get('max_connections')
        if not max_connections_conf:
            print('[server.yml] max_connections conf not exists.')
            return False
        if type(max_connections_conf) != int:
            print('[server.yml] max_connections conf should be int.')
            return False
        max_concurrency_conf = server_conf.get('max_concurrency')
        if not max_concurrency_conf:
            print('[server.yml] max_concurrency conf not exists.')
            return False
        if type(max_concurrency_conf) != int:
            print('[server.yml] max_concurrency conf should be int.')
            return False
        if max_concurrency_conf > max_connections_conf:
            print('[server.yml] max_connections({}) should be greater than or equal to max_concurrency({}).'
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
                print('[server.yml] gpu devices conf not exists.')
                return False
            if type(devices) != list:
                print('[server.yml] gpu devices conf should be int list.')
                return False
            for device in devices:
                if type(device) != int:
                    print('[server.yml] gpu device conf should be int list.')
                    return False

        # 4. Check log conf.
        log_conf = server_conf.get('log')
        if not log_conf:
            print('[server.yml] log conf not exists.')
            return False
        log_log_dir = log_conf.get('log_dir')
        if not log_log_dir:
            print('[server.yml] log log_dir conf not exists.')
            return False
        if os.path.exists(log_log_dir) and os.path.isfile(log_log_dir):
            print('[server.yml] log log_dir conf should be dir.')
            return False
        log_backup_count = log_conf.get('log_backup_count')
        if not log_backup_count:
            print('[server.yml] log log_backup_count conf not exists.')
            return False
        if type(log_backup_count) != int or log_backup_count < 1:
            print('[server.yml] log log_backup_count conf should be int and not less than 1.')
            return False

        return True

    def __prepare_project(self, mar_path, inference_conf, server_conf):
        """
        Prepare project.
        Args:
            mar_path: mar file path.
            inference_conf: inference conf path.
            server_conf: server conf path.

        Returns:
            True for success, False for failure.
        """
        shutil.rmtree(self.__grps_server_work_dir, ignore_errors=True)
        os.makedirs(self.__grps_server_work_dir)
        # Extract mar file.
        tar = tarfile.open(mar_path)
        tar.extractall(path=self.__grps_server_work_dir)
        tar.close()

        # Merge inference conf.
        inference_conf_path_dst = os.path.join(self.__grps_server_work_dir, 'conf/inference.yml')
        if inference_conf:
            if not os.path.exists(inference_conf):
                print('inference conf file not exists.')
                return False
            shutil.copyfile(inference_conf, inference_conf_path_dst)
        with open(inference_conf_path_dst, 'r', encoding='utf-8') as reader:
            inference_conf = yaml.load(reader, Loader=yaml.FullLoader)
        if not self.__check_inference_conf(inference_conf):
            return False

        # Merge server conf.
        server_conf_path_dst = os.path.join(self.__grps_server_work_dir, 'conf/server.yml')
        if server_conf:
            if not os.path.exists(server_conf):
                print('run conf file not exists.')
                return False
            shutil.copyfile(server_conf, server_conf_path_dst)
        with open(server_conf_path_dst, 'r', encoding='utf-8') as reader:
            server_conf = yaml.load(reader, Loader=yaml.FullLoader)
        if not self.__check_server_conf(server_conf):
            return False
        return True

    def __start_server(self, timeout, mpi_np):
        """
        Start server.

        Args:
            timeout: start timeout.
            mpi_np: mpi np.

        Returns
            True for success, False for failed.
        """
        os.chdir(self.__grps_server_work_dir)
        cmd_str = 'bash start_server.sh ' + str(timeout) + ' ' + str(mpi_np)
        ret = os.system(cmd_str)
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        return ret == 0

    def __deploy(self, args):
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

        if not os.path.exists(args.mar_path):
            print('mar file not exists.')
            utils.file_unlock(lock_file_fd, lock_file)
            return -1

        # Check server if has been started.
        if os.path.exists(os.path.join(self.__grps_server_work_dir, 'PID')):
            print('Server({}) has been started. You can start another grps server with your server name defined'
                  ' by --name arg. Also, you can stop current server with \"grpst stop\" cmd.<<<<'.format(args.name))
            utils.file_unlock(lock_file_fd, lock_file)
            return -1

        print('>>>> Starting server({})...'.format(args.name))

        print('>>>> Preparing grps project, mar_path: {}, inference_conf: {}, server_conf: {}'
              .format(args.mar_path,
                      args.inference_conf if args.inference_conf else '(will use inference.yml conf in {})'.format(
                          args.mar_path),
                      args.server_conf if args.server_conf else '(will use server.yml conf in {})'.format(
                          args.mar_path)))
        if not self.__prepare_project(args.mar_path, args.inference_conf, args.server_conf):
            print('Prepare grps project failed')
            utils.file_unlock(lock_file_fd, lock_file)
            return -1
        print('>>>> Preparing grps project finished')

        print('>>>> Starting grps server({})...'.format(args.name))
        if not self.__start_server(args.timeout, args.mpi_np):
            print('Start grps server({}) failed'.format(args.name))
            utils.file_unlock(lock_file_fd, lock_file)
            return -1
        # print('>>>> Start grps server({}) finished'.format(args.name))

        # Unlock work dir.
        utils.file_unlock(lock_file_fd, lock_file)
        return 0

    def deploy(self, args):
        """Deploy project."""
        read_fd, write_fd = os.pipe()

        # Start child process to deploy project to skip parent process SIGINT.
        pid = os.fork()
        if pid == 0:
            os.setsid()
            ret = self.__deploy(args)
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
        os.system('grpst logs {}'.format(args.name))
        return 0
