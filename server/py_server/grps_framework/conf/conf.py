# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  Global conf.
import json
import os

import yaml

from grps_framework.constant import INFERENCE_CONF_PATH, SERVER_CONF_PATH


class Config(object):
    """
    Global conf.

    Attributes:
        server_conf: server configuration.
        inference_conf: inference configuration.
    """

    def __init__(self):
        if not os.path.exists(SERVER_CONF_PATH) or not os.path.exists(INFERENCE_CONF_PATH):
            self.server_conf = None
            self.inference_conf = None
            self.loaded = False
            return
        with open(SERVER_CONF_PATH, 'r') as f:
            self.server_conf = yaml.load(f, Loader=yaml.FullLoader)

        log_dir = self.server_conf['log'].get('log_dir')  # Log dir.
        if not isinstance(log_dir, str):
            print('log_dir must be str, log_dir: {}.'.format(log_dir))
            exit(-1)
        if os.path.exists(log_dir) and os.path.isfile(log_dir):
            print('log_dir must be a dir, log_dir: {}.'.format(log_dir))
            exit(-1)
        if not os.path.exists(log_dir):
            os.makedirs(log_dir)

        with open(INFERENCE_CONF_PATH, 'r') as f:
            self.inference_conf = yaml.load(f, Loader=yaml.FullLoader)

        self.loaded = True
        # print('conf loaded\nserver conf: \n{}\ninference conf: \n{}'
        #       .format(json.dumps(self.server_conf, ensure_ascii=False, indent='  '),
        #               json.dumps(self.inference_conf, ensure_ascii=False, indent='  ')))


global_conf = Config()
