# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/9/11
# Brief  Project logs tool.
import os

import yaml


class GrpsLogs(object):
    """Grps logs tool."""

    def __init__(self):
        """Init."""
        pass

    def build_parser(self, subparsers):
        """
        Build parser for logs sub command.

        Args:
            subparsers: sub parser.
        """
        parser_logs = subparsers.add_parser('logs', help='show server logs')
        parser_logs.add_argument('server_name', type=str, help='server name')
        parser_logs.set_defaults(func=self.logs)  # Set default function.

    @staticmethod
    def __show_logs(server_path):
        """Show logs."""
        server_conf_path = os.path.join(server_path, 'conf', 'server.yml')
        with open(server_conf_path, 'r', encoding='utf-8') as reader:
            conf = yaml.load(reader, Loader=yaml.FullLoader)
        log_dir = conf['log']['log_dir']
        if not os.path.isabs(log_dir):  # if is relative path, convert to absolute path.
            log_dir = os.path.join(server_path, log_dir)
        server_log_path = os.path.join(log_dir, 'grps_server.log')
        user_log_path = os.path.join(log_dir, 'grps_usr.log')
        if not os.path.exists(server_log_path) or not os.path.exists(user_log_path):
            print('Server logs not exists.')
            return

        cmd_str = 'tail -f {} {} {}'.format(server_log_path, user_log_path, os.path.join(server_path, 'nohup.out'))
        os.system(cmd_str)

    def logs(self, args):
        """Project logs."""
        server_path = os.path.join(os.environ['HOME'], '.grps', args.server_name)
        if not os.path.exists(server_path):
            print('Server({}) not existed, please check current server by grpst ps.'.format(args.server_name))
            return -1

        # Specify server name.
        self.__show_logs(server_path)
        return 0
