# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/4/10
# Brief  Project archive tools.
import os
import re
import shutil
import subprocess
import tarfile

import yaml

from common import utils
from common.utils import restore_permission
from const import GRPS_CPP_SERVER_NAME, GRPS_CPP_TEMPLATE_NAME, GRPS_CPP_TEMPLATE_WITH_TF_NAME, \
    GRPS_CPP_TEMPLATE_WITH_TORCH_NAME, GRPS_PY_SERVER_NAME, GRPS_PY_TEMPLATE_NAME, GRPS_CPP_TEMPLATE_WITH_TRT_NAME


class GrpsProjectArchiver(object):
    """Grps project archiver."""

    def __init__(self):
        """Init."""
        self.__cpp_server_src_path = os.path.join(utils.get_project_root(), 'releases', GRPS_CPP_SERVER_NAME + '.tar')
        self.__cpp_server_work_dir = os.path.join(GRPS_CPP_SERVER_NAME)
        self.__py_server_src_path = os.path.join(utils.get_project_root(), 'releases', GRPS_PY_SERVER_NAME + '.tar')
        self.__py_server_work_dir = os.path.join(GRPS_PY_SERVER_NAME)
        self.__py_template_path = os.path.join(utils.get_project_root(), 'releases', GRPS_PY_TEMPLATE_NAME + '.tar')
        self.__cpp_template_path = os.path.join(utils.get_project_root(), 'releases', GRPS_CPP_TEMPLATE_NAME + '.tar')
        self.__cpp_template_with_torch_path = os.path.join(utils.get_project_root(), 'releases',
                                                           GRPS_CPP_TEMPLATE_WITH_TORCH_NAME + '.tar')
        self.__cpp_template_with_tf_path = os.path.join(utils.get_project_root(), 'releases',
                                                        GRPS_CPP_TEMPLATE_WITH_TF_NAME + '.tar')
        self.__cpp_template_with_trt_path = os.path.join(utils.get_project_root(), 'releases',
                                                         GRPS_CPP_TEMPLATE_WITH_TRT_NAME + '.tar')

    def build_parser(self, subparsers):
        """
        Build parser for archive sub command.

        Args:
            subparsers: sub parser.
        """
        parser_archive = subparsers.add_parser('archive', help='archive project')
        parser_archive.add_argument('project_path', type=str, help='project path')
        parser_archive.add_argument('--skip_unittest', action='store_true', help='skip unit test')
        parser_archive.add_argument('--output_path', type=str, help='output path, default is \"./server.mar\"',
                                    default='./server.mar')
        parser_archive.set_defaults(func=self.archive)  # Set default function.

    def __cpp_build(self, project_path, template_path):
        """
        Cpp unit test.

        Args:
            project_path: Project path.
            template_path: Template path.

        Returns:
            True if unit test pass, else False.
        """
        cmd_str = f'rm -rf {project_path}/second_party/grps-server-framework/include/* \
        {project_path}/second_party/grps-server-framework/lib/* && \
        cp -r {template_path}/second_party/grps-server-framework/include/* \
        {project_path}/second_party/grps-server-framework/include/ && \
        cp -r {template_path}/second_party/grps-server-framework/lib/* \
        {project_path}/second_party/grps-server-framework/lib/ && \
        cd {project_path} && \
        bash build.sh clean && \
        bash build.sh'
        ret = os.system(cmd_str)
        return ret == 0

    def __cpp_unit_test(self, project_path, template_path):
        """
        Cpp unit test.

        Args:
            project_path: Project path.
            template_path: Template path.

        Returns:
            True if unit test pass, else False.
        """
        cmd_str = f'rm -rf {project_path}/second_party/grps-server-framework/include/* \
        {project_path}/second_party/grps-server-framework/lib/* && \
        cp -r {template_path}/second_party/grps-server-framework/include/* \
        {project_path}/second_party/grps-server-framework/include/ && \
        cp -r {template_path}/second_party/grps-server-framework/lib/* \
        {project_path}/second_party/grps-server-framework/lib/ && \
        cd {project_path} && \
        bash build.sh clean && \
        bash build.sh && \
        cd build/RelWithDebInfo_install && \
        env LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH ./bin/unit_test'
        ret = os.system(cmd_str)
        return ret == 0

    def __py_unit_test(self, project_path, template_path):
        """
        Py unit test.

        Args:
            project_path: Project path.
            template_path: Template path.

        Returns:
            True if unit test pass, else False.
        """
        cmd_str = (f'cp {template_path}/grps_framework-*-py3-none-any.whl {project_path}/ &&'
                   f'cd {project_path} && pip3 install -r requirements.txt')
        ret = os.system(cmd_str)
        if ret != 0:
            return False
        cmd_str = f'cd {project_path} && python3 -m unittest test.MyTestCase.test_infer'
        ret = os.system(cmd_str)
        return ret == 0

    def __unit_test(self, project_path, project_type, template_path):
        """
        Unit test.

        Args:
            project_path: Project path.
            project_type: Project type.
            template_path: Template path.

        Returns:
            True if unit test pass, else False.
        """
        if project_type in ['cpp', 'cpp_tf', 'cpp_torch', 'cpp_trt']:
            return self.__cpp_unit_test(project_path, template_path)
        elif project_type == 'py':
            return self.__py_unit_test(project_path, template_path)

    def __check_inference_conf(self, conf_path):
        """
        Check inference conf.

        Args:
            conf_path: Build conf path.

        Returns:
            True if check passed.
        """
        with open(conf_path, 'r', encoding='utf-8') as reader:
            inference_conf = yaml.load(reader, Loader=yaml.FullLoader)
            # print(inference_conf)
            if not inference_conf:
                print('[inference.yml] inference conf is empty.')
                return False

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
                if not os.path.exists(inferer_path):
                    print('[inference.yml] inferer_path {} not exist.'.format(inferer_path))
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

    def __check_server_conf(self, server_conf_path):
        """Check server conf."""
        with open(server_conf_path, 'r', encoding='utf-8') as reader:
            server_conf = yaml.load(reader, Loader=yaml.FullLoader)
            # print(server_conf)
            if not server_conf:
                print('[server.yml] server conf is empty.')
                return False

        # 1. Check gpu conf.
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

        # 2. Check log conf.
        log_conf = server_conf.get('log')
        if not log_conf:
            print('[server.yml] log conf not exists.')
            return False
        log_log_dir = log_conf.get('log_dir')
        if not log_log_dir:
            print('[server.yml] log log dir conf not exists.')
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

    def __cpp_build_project(self, project_path, conf_path, server_work_dir):
        """
        Cpp build project.

        Args:
            project_path: Project path.
            conf_path: Build conf path.
            server_work_dir: Server work dir.

        Returns:
            True if build success, else False.
        """
        # Merge user project and grps server.
        # 1. Merge conf.
        build_conf_path_src = conf_path
        build_conf_path_dst = os.path.join(server_work_dir, 'conf/inference.yml')
        if not os.path.exists(build_conf_path_src):
            print('inference conf {} not exist.'.format(build_conf_path_src))
            return False
        shutil.copyfile(build_conf_path_src, build_conf_path_dst)
        run_conf_path_src = os.path.join(project_path, 'conf/server.yml')
        run_conf_path_dst = os.path.join(server_work_dir, 'conf/server.yml')
        if not os.path.exists(run_conf_path_src):
            print('server conf {} not exist.'.format(run_conf_path_src))
            return False
        shutil.copyfile(run_conf_path_src, run_conf_path_dst)

        # 2. Merge third party lib.
        customized_src_path = os.path.join(project_path, 'third_party/lib')
        customized_dst_path = os.path.join(server_work_dir, 'third_party/grps-server-customized/lib')
        if not os.path.exists(customized_src_path):
            print('customized src {} not exist.'.format(customized_src_path))
            return False
        subprocess.call('cp -r {}/* {}/'.format(customized_src_path, customized_dst_path), shell=True)

        # 3. Merge data.
        data_src_path = os.path.join(project_path, 'data')
        data_dst_path = os.path.join(server_work_dir, 'data')
        if not os.path.exists(data_src_path):
            print('data src {} not exist.'.format(data_src_path))
            return False
        subprocess.call('cp -r {}/* {}/'.format(data_src_path, data_dst_path), shell=True)

        # 4. Merge so.
        so_src_path = os.path.join(project_path, 'build/RelWithDebInfo_install/lib')
        so_dst_path = os.path.join(server_work_dir,
                                   'third_party/grps-server-customized/lib')
        if not os.path.exists(so_src_path):
            print('{} not exist.'.format(so_src_path))
            return False
        subprocess.call('cp -r {}/* {}/'.format(so_src_path, so_dst_path), shell=True)

        # 5. Merge .config
        config_src_path = os.path.join(project_path, '.config')
        config_dst_path = os.path.join(server_work_dir, '.config')
        if not os.path.exists(config_src_path):
            print('{} not exist.'.format(config_src_path))
            return False
        if os.path.exists(config_dst_path):
            os.remove(config_dst_path)
        shutil.copyfile(config_src_path, config_dst_path)

        # 6. Build.
        build_cmd = 'cd {} && bash build.sh clean && bash build.sh'.format(server_work_dir)
        ret = os.system(build_cmd)
        return ret == 0

    def __py_build_project(self, project_path, conf_path, server_work_dir):
        """
        Py build project.

        Args:
            project_path: Project path.
            conf_path: Build conf path.
            server_work_dir: Server work dir.

        Returns:
            True if build success, else False.
        """
        # 1. Merge conf.
        build_conf_path_src = conf_path
        build_conf_path_dst = os.path.join(server_work_dir, 'conf/inference.yml')
        if not os.path.exists(build_conf_path_src):
            print('inference conf {} not exist.'.format(build_conf_path_src))
            return False
        shutil.copyfile(build_conf_path_src, build_conf_path_dst)
        run_conf_path_src = os.path.join(project_path, 'conf/server.yml')
        run_conf_path_dst = os.path.join(server_work_dir, 'conf/server.yml')
        if not os.path.exists(run_conf_path_src):
            print('server conf {} not exist.'.format(run_conf_path_src))
            return False
        shutil.copyfile(run_conf_path_src, run_conf_path_dst)

        # 2. Merge data.
        data_src_path = os.path.join(project_path, 'data')
        data_dst_path = os.path.join(server_work_dir, 'data')
        if not os.path.exists(data_src_path):
            print('data src {} not exist.'.format(data_src_path))
            return False
        subprocess.call('cp -r {}/* {}/'.format(data_src_path, data_dst_path), shell=True)

        # 3. Merge src.
        src_path_src = os.path.join(project_path, 'src')
        src_path_dst = os.path.join(server_work_dir, 'src')
        if not os.path.exists(src_path_src):
            print('src {} not exist.'.format(src_path_src))
            return False
        subprocess.call('cp -r {}/* {}/'.format(src_path_src, src_path_dst), shell=True)

        # 4. Override requirements.txt
        requirements_path_src = os.path.join(project_path, 'requirements.txt')
        requirements_path_dst = os.path.join(server_work_dir, 'requirements.txt')
        if not os.path.exists(requirements_path_src):
            print('requirements {} not exist.'.format(requirements_path_src))
            return False
        shutil.copyfile(requirements_path_src, requirements_path_dst)

        return True

    def __build_project(self, project_path, conf_path, server_work_dir, project_type):
        """
        Build project.

        Args:
            project_path: Project path.
            conf_path: Build conf path.
            server_work_dir: Server work dir.
            project_type: Project type.

        Returns:
            True if build success, else False.
        """
        if project_type in ['cpp', 'cpp_tf', 'cpp_torch', 'cpp_trt']:
            return self.__cpp_build_project(project_path, conf_path, server_work_dir)
        elif project_type == 'py':
            return self.__py_build_project(project_path, conf_path, server_work_dir)

    def __tar_project(self, server_work_dir, output_path, project_type):
        """
        Tar project.
        Args:
            server_work_dir: Server work dir.
            output_path: output path.
            project_type: Project type.
        """
        if project_type in ['cpp', 'cpp_tf', 'cpp_torch', 'cpp_trt']:
            build_path = os.path.join(server_work_dir, 'build/RelWithDebInfo_install')
            shutil.rmtree(os.path.join(build_path, 'include'))
        elif project_type == 'py':
            build_path = server_work_dir
        else:
            print('Project type({}) not support.'.format(project_type))
            return False
        os.system('tar zcvf {} -C {} .'.format(output_path, build_path))
        return True

    def archive(self, args):
        """Archive project."""
        if not os.path.exists(args.project_path):
            print('Project not exists.')
            return -1

        if os.path.exists(args.output_path):
            print('Output path({}) already exists.'.format(args.output_path))
            return -1

        print('>>>> Archiving project({}) to {}.'.format(args.project_path, args.output_path))

        file_lock = args.output_path + '.lock'
        file_lock_fd = utils.file_lock(file_lock)
        if not file_lock_fd:
            print('Output path({}) has been locked.'.format(args.output_path))
            return -1

        # check inference conf
        inference_conf_path = os.path.join(args.project_path, 'conf/inference.yml')
        if not os.path.exists(inference_conf_path):
            print('Inference conf not exists, inference_conf_path: {}.'.format(inference_conf_path))
            utils.file_unlock(file_lock_fd, file_lock)
            restore_permission(args.project_path)
            return -1
        print('>>>> Check inference conf({}).'.format(inference_conf_path))
        if self.__check_inference_conf(inference_conf_path):
            print('>>>> Check inference conf success.')
        else:
            print('Check inference conf failed.')
            utils.file_unlock(file_lock_fd, file_lock)
            restore_permission(args.project_path)
            return -1

        # check server conf
        server_conf_path = os.path.join(args.project_path, 'conf/server.yml')
        if not os.path.exists(server_conf_path):
            print('Server conf not exists, server_conf_path: {}.'.format(server_conf_path))
            utils.file_unlock(file_lock_fd, file_lock)
            restore_permission(args.project_path)
            return -1
        print('>>>> Check server conf({}).'.format(server_conf_path))
        if self.__check_server_conf(server_conf_path):
            print('>>>> Check server conf success.')
        else:
            print('Check server conf failed.')
            utils.file_unlock(file_lock_fd, file_lock)
            restore_permission(args.project_path)
            return -1

        # get project type
        if os.path.exists(os.path.join(args.project_path, 'src', 'main.cc')):
            if os.path.exists(os.path.join(args.project_path, '.config')):
                with open(os.path.join(args.project_path, '.config'), 'r', encoding='utf-8') as reader:
                    lines = reader.readlines()
                if 'tf_enable=1\n' in lines:
                    project_type = 'cpp_tf'
                elif 'torch_enable=1\n' in lines:
                    project_type = 'cpp_torch'
                elif 'trt_enable=1\n' in lines:
                    project_type = 'cpp_trt'
                else:
                    project_type = 'cpp'
            else:
                print('.config not exists in cpp project.')
                utils.file_unlock(file_lock_fd, file_lock)
                return -1
        elif os.path.exists(os.path.join(args.project_path, 'test.py')):
            project_type = 'py'
        else:
            print('Not found main Project type not support.')
            utils.file_unlock(file_lock_fd, file_lock)
            return -1

        if project_type == 'py':
            template_path = self.__py_template_path
            template_name = GRPS_PY_TEMPLATE_NAME
        elif project_type == 'cpp':
            template_path = self.__cpp_template_path
            template_name = GRPS_CPP_TEMPLATE_NAME
        elif project_type == 'cpp_torch':
            template_path = self.__cpp_template_with_torch_path
            template_name = GRPS_CPP_TEMPLATE_WITH_TORCH_NAME
        elif project_type == 'cpp_tf':
            template_path = self.__cpp_template_with_tf_path
            template_name = GRPS_CPP_TEMPLATE_WITH_TF_NAME
        elif project_type == 'cpp_trt':
            template_path = self.__cpp_template_with_trt_path
            template_name = GRPS_CPP_TEMPLATE_WITH_TRT_NAME
        else:
            print('Project type({}) not support.'.format(project_type))
            utils.file_unlock(file_lock_fd, file_lock)
            return -1
        if not os.path.exists(template_path):
            print('Project type({}) not support in current env because not installed.'.format(project_type))
            utils.file_unlock(file_lock_fd, file_lock)
            return -1

        tar = tarfile.open(template_path)
        tar.extractall()
        tar.close()

        if not args.skip_unittest:
            print('>>>> Build customized project and unit test.')
            if not self.__unit_test(args.project_path, project_type, template_name):
                print('Build customized project and unit test failed.')
                shutil.rmtree(template_name)
                utils.file_unlock(file_lock_fd, file_lock)
                restore_permission(args.project_path)
                return -1
            print('>>>> Build customized project and unit test success.')
        elif project_type in ['cpp', 'cpp_tf', 'cpp_torch', 'cpp_trt']:
            print('>>>> Build {} customized project.'.format(project_type))
            if not self.__cpp_build(args.project_path, template_name):
                print('Build customized project failed.')
                shutil.rmtree(template_name)
                utils.file_unlock(file_lock_fd, file_lock)
                restore_permission(args.project_path)
                return -1

        shutil.rmtree(template_name)

        print('>>>> Unzip server src.')
        server_work_dir = ''
        server_src_path = ''
        if project_type == 'py':
            server_work_dir = self.__py_server_work_dir
            server_src_path = self.__py_server_src_path
        elif project_type in ['cpp', 'cpp_tf', 'cpp_torch', 'cpp_trt']:
            server_work_dir = self.__cpp_server_work_dir
            server_src_path = self.__cpp_server_src_path

        if not os.path.exists(server_src_path):
            print('Project type({}) not support in current env because not installed.'.format(project_type))
            utils.file_unlock(file_lock_fd, file_lock)
            restore_permission(args.project_path)
            return -1
        if os.path.exists(server_work_dir):
            shutil.rmtree(server_work_dir, ignore_errors=True)
        tar = tarfile.open(server_src_path)
        tar.extractall()
        tar.close()
        print('>>>> Unzip finished.')

        print('>>>> Build project.')
        if not self.__build_project(args.project_path, inference_conf_path, server_work_dir, project_type):
            print('Build project failed.')
            utils.file_unlock(file_lock_fd, file_lock)
            restore_permission(args.project_path)
            return -1
        print('>>>> Build project success.')

        print('>>>> Archive project, output_path: {}'.format(args.output_path))
        if not self.__tar_project(server_work_dir, args.output_path, project_type):
            print('Archive project failed.')
            utils.file_unlock(file_lock_fd, file_lock)
            restore_permission(args.project_path)
            return -1
        shutil.rmtree(server_work_dir, ignore_errors=True)
        print('>>>> Archive project({}) finished.'.format(args.output_path))
        utils.file_unlock(file_lock_fd, file_lock)

        restore_permission(args.project_path)
        return 0
