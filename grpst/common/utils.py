# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/10/12
# Brief  Utils.
import os

import fcntl


def get_project_root():
    """Get project root path."""
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def file_lock(file_path, block=False):
    """File lock.

    Args:
        file_path: File path.
        block: Block or not.

    Returns:
        fd.
    """
    fd = open(file_path, 'w')
    try:
        if block:
            fcntl.flock(fd, fcntl.LOCK_EX)
        else:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except Exception as e:
        print('File lock failed: {}'.format(e))
        return None
    return fd


def file_unlock(fd, file_path):
    """File unlock.

    Args:
        fd: File descriptor.
        file_path: File path.
    """
    fcntl.flock(fd, fcntl.LOCK_UN)
    fd.close()
    os.remove(file_path)


def restore_permission(path):
    """Chown all generated file in docker to host user."""
    print('Restore permission for {}'.format(path))
    os.system('DIR_UID=$(stat -c "%u" .) && '
              'DIR_GID=$(stat -c "%g" .) && '
              'chown -R $DIR_UID:$DIR_GID {}'.format(path))
