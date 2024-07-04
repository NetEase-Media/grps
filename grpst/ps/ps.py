# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/4/10
# Brief  Project status tool.
import os

import yaml


class GrpsPrintStatus(object):
    """Grps print status tool."""

    def __init__(self):
        """Init."""
        pass

    def build_parser(self, subparsers):
        """
        Build parser for status sub command.

        Args:
            subparsers: sub parser.
        """
        parser_status = subparsers.add_parser('ps', help='print status')
        parser_status.add_argument('-n', '--name', type=str, help='server name', default='')
        parser_status.set_defaults(func=self.status)  # Set default function.

    @staticmethod
    def __print_status(name, server_project_path):
        """Print status."""
        if not os.path.exists(server_project_path):
            print('{} not exists.'.format(name))
            return

        server_pid_path = os.path.join(server_project_path, 'PID')
        if not os.path.exists(server_pid_path):
            return
        else:
            server_conf_path = os.path.join(server_project_path, 'conf', 'server.yml')
            with open(server_conf_path, 'r', encoding='utf-8') as reader:
                conf = yaml.load(reader, Loader=yaml.FullLoader)
            port = str(conf['interface']['port'])
            with open(server_pid_path, 'r', encoding='utf-8') as reader:
                pid_str = reader.readline().strip()
            # check pid alive.
            pids = pid_str.split(' ')
            for pid in pids:
                if not os.path.exists('/proc/{}'.format(pid)):
                    return
            print('{:<20}{:<20}{:<20}{:<20}'.format(port[:20], name[:20], pid_str[:20], server_project_path))

    def status(self, args):
        """Project status."""
        grps_worker_root = os.path.join(os.environ['HOME'], '.grps')
        if not os.path.exists(grps_worker_root):
            print('No any server.')
            return -1

        print('{:<20}{:<20}{:<20}{:<20}'.format('PORT(HTTP,RPC)', 'NAME', 'PID', 'DEPLOY_PATH'))

        # Specify server name.
        if args.name != '':
            self.__print_status(args.name, os.path.join(grps_worker_root, args.name))
            return 0

        # Print all server.
        for server_name in os.listdir(grps_worker_root):
            self.__print_status(server_name, os.path.join(grps_worker_root, server_name))
        return 0
