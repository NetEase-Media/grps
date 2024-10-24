# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/4/10
# Brief  Project stop tools.
import os.path


# from common import utils


class GrpsProjectStopper(object):
    """Grps project stopper."""

    def __init__(self):
        """Init."""
        self.__grps_server_work_dir = ''  # Grps server work dir.

    def build_parser(self, subparsers):
        """
        Build parser for stop sub command.

        Args:
            subparsers: sub parser.
        """
        parser_stop = subparsers.add_parser('stop', help='stop project')
        parser_stop.add_argument('server_name', type=str, help='server name')
        parser_stop.set_defaults(func=self.stop)  # Set default function.

    def __stop_server(self):
        """
        Stop server.

        Returns
            True for success, False for failed.
        """
        if not os.path.exists(os.path.join(self.__grps_server_work_dir, 'stop_server.sh')):
            server_name = os.path.basename(self.__grps_server_work_dir)
            print('Server({}) not existed, please check current server by grpst ps.'.format(server_name))
            return False
        os.chdir(self.__grps_server_work_dir)
        cmd_str = 'bash stop_server.sh'
        ret = os.system(cmd_str)
        return ret == 0

    def stop(self, args):
        """Stop project."""
        grps_worker_root = os.path.join(os.environ['HOME'], '.grps')
        self.__grps_server_work_dir = os.path.join(grps_worker_root, args.server_name)
        if not os.path.exists(self.__grps_server_work_dir):
            print('Server({}) not existed, please check current server by grpst ps.'.format(args.server_name))
            return -1

        # Lock work dir.
        # lock_file = self.__grps_server_work_dir + '.lock'
        # lock_file_fd = utils.file_lock(lock_file)
        # if not lock_file_fd:
        #     print('Server({}) has been locked. You can start another grps server with your server name.'.format(
        #         args.server_name))
        #     return -1

        print('>>>> Stopping server({})...'.format(args.server_name))
        if not self.__stop_server():
            print('Stop server({}) failed.'.format(args.server_name))
            # utils.file_unlock(lock_file_fd, lock_file)
            return -1
        print('>>>> Stop server({}) success.'.format(args.server_name))

        # Unlock work dir.
        # utils.file_unlock(lock_file_fd, lock_file)
        return 0
