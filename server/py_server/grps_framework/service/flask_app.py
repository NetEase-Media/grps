# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/13
# Brief  grps flask app.

from flask import Flask
from gunicorn.app.base import BaseApplication

from grps_framework.conf.conf import global_conf
from grps_framework.constant import GRPS_VERSION
from grps_framework.constant import URL_ROOT_PATH
from grps_framework.executor.executor import Executor
from grps_framework.logger.logger import logger
from grps_framework.handler.grps_handler import HttpHandler
from grps_framework.handler.js_handler import JSHandler
from grps_framework.handler.metrics_handler import MetricsHandler


class GrpsFlaskApp(object):
    """
    GRPS Flask Http app.

    Attributes:
        app: flask app.
    """

    def __init__(self, app_name, executor: Executor, host, port, tp, max_connections):
        self.__tp = tp
        self.__max_connections = max_connections
        self.__health_handler = None
        self.__infer_handler = None
        self.__metadata_handler = None
        self.__js_handler = None
        self.__metrics_handler = None
        self.__app = None
        self.__executor = executor
        self.__app = Flask(app_name)
        self.__init_router()
        self.__host = host
        self.__port = port

    def __init_router(self):
        """Init router."""
        self.__http_handler = HttpHandler(executor=self.__executor, tp=self.__tp)
        self.__js_handler = JSHandler()
        self.__metrics_handler = MetricsHandler()
        # Init health router.
        self.__app.add_url_rule(URL_ROOT_PATH + '/health/online', view_func=self.__http_handler.online,
                                methods=['GET', 'POST'])
        self.__app.add_url_rule(URL_ROOT_PATH + '/health/offline', view_func=self.__http_handler.offline,
                                methods=['GET', 'POST'])
        self.__app.add_url_rule(URL_ROOT_PATH + '/health/live', view_func=self.__http_handler.live,
                                methods=['GET', 'POST'])
        self.__app.add_url_rule(URL_ROOT_PATH + '/health/ready', view_func=self.__http_handler.ready,
                                methods=['GET', 'POST'])

        # Init infer router.
        customized_predict_http = global_conf.server_conf.get('interface').get('customized_predict_http')
        if customized_predict_http:
            path = customized_predict_http.get('path')
            if not path:
                logger.error("server.yml: customized_predict_http.path is empty but customized_predict_http is true")
                raise ValueError(
                    "server.yml: customized_predict_http.path is empty but customized_predict_http is true")
            if path in [URL_ROOT_PATH + '/health/online', URL_ROOT_PATH + '/health/offline',
                        URL_ROOT_PATH + '/health/live', URL_ROOT_PATH + '/health/ready',
                        URL_ROOT_PATH + '/infer/predict', URL_ROOT_PATH + '/metadata/server',
                        URL_ROOT_PATH + '/metadata/model', URL_ROOT_PATH + '/monitor/series',
                        URL_ROOT_PATH + '/monitor/metrics', URL_ROOT_PATH + '/']:
                logger.error("server.yml: Invalid customized path: " + path + ", cannot use internal path.")
                raise ValueError("server.yml: Invalid customized path: " + path + ", cannot use internal path.")
            if customized_predict_http.get('customized_body'):
                self.__app.add_url_rule(path, view_func=self.__http_handler.predict_custom_http,
                                        methods=['POST', 'GET'])
            else:
                self.__app.add_url_rule(path, view_func=self.__http_handler.predict,
                                        methods=['POST', 'GET'])

            logger.info('register customized predict http path: ' + path)

        self.__app.add_url_rule(URL_ROOT_PATH + '/infer/predict', view_func=self.__http_handler.predict,
                                methods=['POST'])

        # Init metadata router.
        self.__app.add_url_rule(URL_ROOT_PATH + '/metadata/server', view_func=self.__http_handler.server_metadata,
                                methods=['GET', 'POST'])
        self.__app.add_url_rule(URL_ROOT_PATH + '/metadata/model', view_func=self.__http_handler.model_metadata,
                                methods=['POST'])

        # Init monitor router
        self.__app.add_url_rule(URL_ROOT_PATH + '/monitor/series', view_func=self.__http_handler.monitor_data,
                                methods=['GET'])
        self.__app.add_url_rule(URL_ROOT_PATH + '/monitor/metrics', view_func=self.__metrics_handler.metrics,
                                methods=['GET'])
        self.__app.add_url_rule('/', view_func=self.__metrics_handler.metrics,
                                methods=['GET'])

        # js router
        self.__app.add_url_rule(URL_ROOT_PATH + '/js/jquery_min', view_func=self.__js_handler.jquery_min_js,
                                methods=['GET'])
        self.__app.add_url_rule(URL_ROOT_PATH + '/js/flot_min', view_func=self.__js_handler.flot_min_js,
                                methods=['GET'])

    def start(self):
        """Start grps flask app."""
        try:
            logger.info('Start grps http service, version: {}, port: {}, max_connections(not supported now)'
                        ', max_concurrency: {}.'.format(GRPS_VERSION, self.__port, self.__tp._max_workers))
            self.__app.run(host=self.__host, port=self.__port, threaded=True)
        except Exception as e:
            logger.critical('Start grps http service failed, error: {}'.format(e))

    @property
    def app(self):
        return self.__app
