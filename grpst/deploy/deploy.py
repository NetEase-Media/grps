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
        parser_deploy.add_argument('--conf_path', type=str,
                                   help='server conf path, will use server.yml in mar(mar_path arg) if not set',
                                   required=False)
        parser_deploy.add_argument('--timeout', type=int, help='server start timeout, default is 300s', default=300)
        parser_deploy.add_argument('mar_path', type=str, help='model server archive path')
        parser_deploy.set_defaults(func=self.deploy)  # Set default function.

    @staticmethod
    def __check_conf(server_conf):
        """Check run conf."""

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
                      ' --conf_path.'.format(port))
                return False
        # Check customized_predict_http.
        customized_predict_http_conf = interface_conf.get('customized_predict_http')
        if customized_predict_http_conf:
            customized_predict_http_path = customized_predict_http_conf.get('path')
            if not customized_predict_http_path:
                print('[server.yml] Customized predict http path conf not exists.')
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
            if type(customized_body) != bool:
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

    def __prepare_project(self, mar_path, conf_path):
        """
        Prepare project.
        Args:
            mar_path: mar file path.
            conf_path: run conf path.

        Returns:
            True for success, False for failure.
        """
        shutil.rmtree(self.__grps_server_work_dir, ignore_errors=True)
        os.makedirs(self.__grps_server_work_dir)
        # Extract mar file.
        tar = tarfile.open(mar_path)
        tar.extractall(path=self.__grps_server_work_dir)
        tar.close()

        # Merge run conf.
        run_conf_path_dst = os.path.join(self.__grps_server_work_dir, 'conf/server.yml')
        if conf_path:
            if not os.path.exists(conf_path):
                print('run conf file not exists.')
                return False
            shutil.copyfile(conf_path, run_conf_path_dst)
        with open(run_conf_path_dst, 'r', encoding='utf-8') as reader:
            server_conf = yaml.load(reader, Loader=yaml.FullLoader)
        if not self.__check_conf(server_conf):
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

        print('>>>> Preparing grps project, mar_path: {}, conf_path: {}'
              .format(args.mar_path,
                      args.conf_path if args.conf_path else '(will use server.yml conf in {})'.format(args.mar_path)))
        if not self.__prepare_project(args.mar_path, args.conf_path):
            print('Prepare grps project failed')
            utils.file_unlock(lock_file_fd, lock_file)
            return -1
        print('>>>> Preparing grps project finished')

        print('>>>> Starting grps server({})...'.format(args.name))
        if not self.__start_server(args.timeout):
            print('Start grps server({}) failed'.format(args.name))
            utils.file_unlock(lock_file_fd, lock_file)
            return -1
        print('>>>> Start grps server({}) finished'.format(args.name))

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
        ret = os.read(read_fd, 1)
        if ret == b'0':
            return -1

        # Show server logs.
        print('\n\n>>>> Showing server logs, stop showing by Ctrl+C.')
        signal.signal(signal.SIGINT, signal.SIG_DFL)
        os.system('grpst logs {}'.format(args.name))
        return 0
