# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/28
# Brief
from concurrent import futures

import grpc

from grps_framework.apis import grps_pb2_grpc
from grps_framework.constant import GRPC_MAX_MESSAGE_LENGTH
from grps_framework.constant import GRPS_VERSION
from grps_framework.executor.executor import Executor
from grps_framework.logger.logger import logger
from grps_framework.handler.grps_handler import GrpcHandler


class GrpsGrpcServer(object):
    """GRPS Grpc app."""

    def __init__(self, app_name, executor: Executor, host, port, tp: futures.ThreadPoolExecutor, max_connections):
        self.__app_name = app_name
        self.__executor = executor
        self.__host = host
        self.__port = port
        self.__tp = tp
        self.__max_connections = max_connections

    def __init_grpc_server(self):
        """Init grpc server"""

        self.__grps_grpc_service = grpc.server(
            futures.ThreadPoolExecutor(max_workers=self.__max_connections + 1), options=[
                ('grpc.max_send_message_length', GRPC_MAX_MESSAGE_LENGTH),
                ('grpc.max_receive_message_length', GRPC_MAX_MESSAGE_LENGTH),
            ], maximum_concurrent_rpcs=self.__max_connections)
        grps_pb2_grpc.add_GrpsServiceServicer_to_server(GrpcHandler(executor=self.__executor, tp=self.__tp),
                                                        self.__grps_grpc_service)
        self.__grps_grpc_service.add_insecure_port('[::]:' + str(self.__port))
        self.__grps_grpc_service.start()
        self.__grps_grpc_service.wait_for_termination()

    def start(self):
        """Start grps grpc server."""
        logger.info('Start grps grpc service, version: {}, port: {}, max_connections: {}, max_concurrency: {}.'
                    .format(GRPS_VERSION, self.__port, self.__max_connections, self.__tp._max_workers))
        self.__init_grpc_server()
        logger.info('Grps grpc server terminated.')
