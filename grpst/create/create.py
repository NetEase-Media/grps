# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/4/10
# Brief  Project create tools.
import os.path
import tarfile

from common import utils
from common.utils import restore_permission
from const import GRPS_CPP_TEMPLATE_NAME, GRPS_CPP_TEMPLATE_WITH_TF_NAME, GRPS_CPP_TEMPLATE_WITH_TORCH_NAME, \
    GRPS_CPP_TEMPLATE_WITH_TRT_NAME, GRPS_PY_TEMPLATE_NAME


class GrpsProjectCreator(object):
    """Grps project creator."""

    def __init__(self):
        """Init."""
        self.__py_template_path = os.path.join(utils.get_project_root(), 'releases',
                                               GRPS_PY_TEMPLATE_NAME + '.tar')
        self.__cpp_template_path = os.path.join(utils.get_project_root(), 'releases',
                                                GRPS_CPP_TEMPLATE_NAME + '.tar')
        self.__cpp_template_with_torch_path = os.path.join(utils.get_project_root(), 'releases',
                                                           GRPS_CPP_TEMPLATE_WITH_TORCH_NAME + '.tar')
        self.__cpp_template_with_tf_path = os.path.join(utils.get_project_root(), 'releases',
                                                        GRPS_CPP_TEMPLATE_WITH_TF_NAME + '.tar')
        self.__cpp_template_with_trt_path = os.path.join(utils.get_project_root(), 'releases',
                                                         GRPS_CPP_TEMPLATE_WITH_TRT_NAME + '.tar')

    def build_parser(self, subparsers):
        """
        Build parser for create sub command.

        Args:
            subparsers: sub parser.
        """
        parser_create = subparsers.add_parser('create', help='create project')
        parser_create.add_argument('--project_type', type=str,
                                   help='\"py\": python project with pytorch, tf and trt support. '
                                        '\"cpp\": c++ project without nn lib support. '
                                        '\"cpp_torch\": c++ project with libtorch support. '
                                        '\"cpp_tf\": c++ project with libtensorflow support. '
                                        '\"cpp_trt\": c++ project with libtensorrt support.')
        parser_create.add_argument('project_path', type=str, help='project path will be created')
        parser_create.set_defaults(func=self.create)  # Set default function.

    def create(self, args):
        """Create project."""
        if args.project_type is None or args.project_type == '':
            args.project_type = input(
                'Select project type.\n'
                '[1] \"py\": python project with pytorch, tf and trt support.\n'
                '[2] \"cpp\": c++ project without nn lib support.\n'
                '[3] \"cpp_torch\": c++ project with libtorch support.\n'
                '[4] \"cpp_tf\": c++ project with libtensorflow support.\n'
                '[5] \"cpp_trt\": c++ project with libtensorrt support.\n'
                'Please input number(1-5), default is \"1\":\n')
            if args.project_type == '' or args.project_type == '1':
                args.project_type = 'py'
            elif args.project_type == '2':
                args.project_type = 'cpp'
            elif args.project_type == '3':
                args.project_type = 'cpp_torch'
            elif args.project_type == '4':
                args.project_type = 'cpp_tf'
            elif args.project_type == '5':
                args.project_type = 'cpp_trt'
            else:
                print('Project type({}) not support.'.format(args.project_type))
                return -1

        if os.path.exists(args.project_path):
            print('Project path({}) already exists.'.format(args.project_path))
            return -1

        file_lock = args.project_path + '.lock'
        file_lock_fd = utils.file_lock(file_lock)
        if not file_lock_fd:
            print('Project path({}) has been locked.'.format(args.project_path))
            return -1

        print('>>>> Creating project({}) with type({}).'.format(args.project_path, args.project_type))
        template_path = ''
        template_name = ''
        if args.project_type == 'py':
            template_path = self.__py_template_path
            template_name = GRPS_PY_TEMPLATE_NAME
        elif args.project_type == 'cpp':
            template_path = self.__cpp_template_path
            template_name = GRPS_CPP_TEMPLATE_NAME
        elif args.project_type == 'cpp_torch':
            template_path = self.__cpp_template_with_torch_path
            template_name = GRPS_CPP_TEMPLATE_WITH_TORCH_NAME
        elif args.project_type == 'cpp_tf':
            template_path = self.__cpp_template_with_tf_path
            template_name = GRPS_CPP_TEMPLATE_WITH_TF_NAME
        elif args.project_type == 'cpp_trt':
            template_path = self.__cpp_template_with_trt_path
            template_name = GRPS_CPP_TEMPLATE_WITH_TRT_NAME
        else:
            print('Project type({}) not support.'.format(args.project_type))
            utils.file_unlock(file_lock_fd, file_lock)
            return -1

        if not os.path.exists(template_path):
            print('Project type({}) not support in current env because not installed.'.format(args.project_type))
            utils.file_unlock(file_lock_fd, file_lock)
            return -1

        tar = tarfile.open(template_path)
        tar.extractall()
        tar.close()
        os.rename(template_name, args.project_path)

        utils.file_unlock(file_lock_fd, file_lock)
        print('>>>> Create project({}) with {} project type finished.'.format(args.project_path, args.project_type))

        restore_permission(args.project_path)
        return 0
