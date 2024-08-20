# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/10/8
# Brief  Grps service handler.

import http
import json
import time
import traceback
from concurrent.futures import ThreadPoolExecutor
from enum import Enum

import grpc
import numpy as np
import yaml
from flask import request, Request, Response, stream_with_context
from google.protobuf.json_format import MessageToDict, MessageToJson, ParseDict

from grps_framework.apis import grps_pb2, grps_pb2_grpc
from grps_framework.apis.grps_pb2 import DataType, GenericTensor, GrpsMessage, Status
from grps_framework.conf.conf import global_conf
from grps_framework.constant import GPU_OOM_COUNT, INFERENCE_CONF_PATH, QPS, REQ_FAIL_RATE, REQ_LATENCY_AVG, \
    REQ_LATENCY_CDF, REQ_LATENCY_MAX, SERVER_CONF_PATH
from grps_framework.context.context import GrpsContext
from grps_framework.logger.logger import logger
from grps_framework.monitor.monitor import app_monitor


class GrpcHandler(grps_pb2_grpc.GrpsServiceServicer):
    def __init__(self, executor, tp: ThreadPoolExecutor = None):
        self.__executor = executor
        self.__health = False
        self.__tp = tp

    def Predict(self, request, context):
        """
        Infer predict grpc handler.
        Returns:
            GrpsMessage.
        """
        app_monitor.inc(QPS, 1)
        begin_time = time.time()

        try:
            grps_context = GrpsContext(grpc_context=context)
            grpc_response = self.__tp.submit(self.__executor.infer, request, grps_context, request.model).result()

            if not grps_context.has_err() and str(type(grpc_response)) != str(GrpsMessage):
                logger.error('Output type must be GrpsMessage.')
                grps_context.set_err_msg('Output type must be GrpsMessage.')

            if grps_context.has_err():
                app_monitor.avg(REQ_FAIL_RATE, 100)
                logger.error('Predict error: {}'.format(grps_context.get_err_msg()))
                grpc_response = GrpsMessage(
                    status=Status(code=http.HTTPStatus.INTERNAL_SERVER_ERROR.value, msg=str(grps_context.get_err_msg()),
                                  status=Status.FAILURE))
            else:
                grpc_response.status.code = http.HTTPStatus.OK.value
                grpc_response.status.msg = 'OK'
                grpc_response.status.status = grps_pb2.Status.SUCCESS
                app_monitor.avg(REQ_FAIL_RATE, 0)
        except Exception as e:
            trace_back = traceback.format_exc()
            logger.error(trace_back)

            # Gpu oom statistic.
            if trace_back.find('CUDA out of memory') != -1 or trace_back.find('OOM') != -1:
                app_monitor.inc(GPU_OOM_COUNT, 1)

            app_monitor.avg(REQ_FAIL_RATE, 100)
            grpc_response = grps_pb2.GrpsMessage()
            grpc_response.status.code = http.HTTPStatus.INTERNAL_SERVER_ERROR.value
            grpc_response.status.msg = str(trace_back)
            grpc_response.status.status = grps_pb2.Status.FAILURE

        total_latency_ms = (time.time() - begin_time) * 1e3
        app_monitor.avg(REQ_LATENCY_AVG, total_latency_ms)
        app_monitor.max(REQ_LATENCY_MAX, total_latency_ms)
        app_monitor.cdf(REQ_LATENCY_CDF, total_latency_ms)
        logger.info('[Predict] from client: {}, latency: {:.2f} ms'.format(context.peer(), total_latency_ms))

        return grpc_response

    def PredictStreaming(self, request, context):
        """
        Infer predict streaming grpc handler.
        Args:
            request:
            context:

        Returns:

        """
        app_monitor.inc(QPS, 1)
        begin_time = time.time()

        grps_context = GrpsContext(grpc_context=context)
        try:
            grps_context.start_grpc_streaming_generator()
            work = self.__tp.submit(self.__executor.infer, request, grps_context, request.model)

            while grps_context._grpc_streaming_run:
                grpc_response = grps_context._grpc_streaming_queue.get()
                if grpc_response:
                    yield grpc_response
            work.result()

            if grps_context.has_err():
                app_monitor.avg(REQ_FAIL_RATE, 100)
                logger.error('Predict error: {}'.format(grps_context.get_err_msg()))
                grpc_response = grps_pb2.GrpsMessage(
                    status=Status(code=http.HTTPStatus.INTERNAL_SERVER_ERROR.value, msg=str(grps_context.get_err_msg()),
                                  status=Status.FAILURE))
                yield grpc_response
            else:
                app_monitor.avg(REQ_FAIL_RATE, 0)
        except Exception as e:
            trace_back = traceback.format_exc()
            logger.error(trace_back)

            # Gpu oom statistic.
            if trace_back.find('CUDA out of memory') != -1 or trace_back.find('OOM') != -1:
                app_monitor.inc(GPU_OOM_COUNT, 1)

            app_monitor.avg(REQ_FAIL_RATE, 100)
            grpc_response = grps_pb2.GrpsMessage()
            grpc_response.status.code = http.HTTPStatus.INTERNAL_SERVER_ERROR.value
            grpc_response.status.msg = str(trace_back)
            grpc_response.status.status = grps_pb2.Status.FAILURE
            yield grpc_response

        total_latency_ms = (time.time() - begin_time) * 1e3
        app_monitor.avg(REQ_LATENCY_AVG, total_latency_ms)
        app_monitor.max(REQ_LATENCY_MAX, total_latency_ms)
        app_monitor.cdf(REQ_LATENCY_CDF, total_latency_ms)
        logger.info('[Predict] from client: {}, latency: {:.2f} ms'.format(context.peer(), total_latency_ms))

    def Online(self, request, context):
        """
        set online
        Args:
            request:
            context:

        Returns:
            GrpsMessage.
        """
        self.__health = True
        logger.info('[Online] from client: {}'.format(context.peer()))
        grpc_response = grps_pb2.GrpsMessage()
        grpc_response.status.code = http.HTTPStatus.OK.value
        grpc_response.status.msg = 'OK'
        grpc_response.status.status = grps_pb2.Status.SUCCESS
        return grpc_response

    def Offline(self, request, context):
        """
        set offline.
        Args:
            request:
            context:

        Returns:
            GrpsMessage.
        """
        self.__health = False
        logger.info('[Offline] from client: {}'.format(context.peer()))
        grpc_response = grps_pb2.GrpsMessage()
        grpc_response.status.code = http.HTTPStatus.OK.value
        grpc_response.status.msg = 'OK'
        grpc_response.status.status = grps_pb2.Status.SUCCESS
        return grpc_response

    def CheckLiveness(self, request, context):
        """
        check liveness
        Args:
            request:
            context:

        Returns:
            GrpsMessage.
        """
        logger.info('[CheckLiveness] from client: {}'.format(context.peer()))
        grpc_response = grps_pb2.GrpsMessage()
        grpc_response.status.code = http.HTTPStatus.OK.value
        grpc_response.status.msg = 'OK'
        grpc_response.status.status = grps_pb2.Status.SUCCESS
        return grpc_response

    def CheckReadiness(self, request, context):
        """
        check ready
        Args:
            request:
            context:

        Returns:
            GrpsMessage.
        """
        logger.info('[CheckReadiness] from client: {}'.format(context.peer()))
        grpc_response = grps_pb2.GrpsMessage()
        if self.__health:
            grpc_response.status.code = http.HTTPStatus.OK.value
            grpc_response.status.msg = 'OK'
            grpc_response.status.status = grps_pb2.Status.SUCCESS
        else:
            grpc_response.status.code = http.HTTPStatus.FORBIDDEN.value
            grpc_response.status.msg = 'Service Unavailable'
            grpc_response.status.status = grps_pb2.Status.FAILURE
        return grpc_response

    def ServerMetadata(self, request, context):
        """
        get server metadata
       Args:
            request:
            context:

        Returns:
            GrpsMessage.
        """
        logger.info('[ServerMetadata] from client: {}'.format(context.peer()))
        with open(INFERENCE_CONF_PATH, 'r') as f:
            metadata = f.read()
        with open(SERVER_CONF_PATH, 'r') as f:
            metadata += '\n' + f.read()
        grpc_response = grps_pb2.GrpsMessage()
        grpc_response.status.code = http.HTTPStatus.OK.value
        grpc_response.status.msg = 'OK'
        grpc_response.status.status = grps_pb2.Status.SUCCESS
        grpc_response.str_data = metadata
        return grpc_response

    def ModelMetadata(self, request, context):
        """
        get model metadata
        Args:
            request:
            context:

        Returns:
            GrpsMessage.
        """
        logger.info('[ModelMetadata] from client: {}'.format(context.peer()))
        grpc_response = grps_pb2.GrpsMessage()
        if not request.HasField("str_data"):
            grpc_response.status.code = http.HTTPStatus.BAD_REQUEST.value
            grpc_response.status.msg = 'No model name.'
            grpc_response.status.status = grps_pb2.Status.FAILURE
            return grpc_response
        model_name = request.str_data
        if not model_name:
            grpc_response.status.code = http.HTTPStatus.BAD_REQUEST.value
            grpc_response.status.msg = 'The model name is empty.'
            grpc_response.status.status = grps_pb2.Status.FAILURE
            return grpc_response
        for model_desc in global_conf.inference_conf['models']:
            if model_desc['name'] == model_name:
                grpc_response.status.code = http.HTTPStatus.OK.value
                grpc_response.status.msg = 'OK'
                grpc_response.status.status = grps_pb2.Status.SUCCESS
                grpc_response.str_data = yaml.dump(model_desc)
                return grpc_response
        grpc_response.status.code = http.HTTPStatus.NOT_FOUND.value
        grpc_response.status.msg = 'Model not found.'
        grpc_response.status.status = grps_pb2.Status.FAILURE
        return grpc_response


class HttpHandler(object):
    class HttpStreamingCtrlMode(Enum):
        QUERY_PARAM = 0
        HEADER_PARAM = 1
        BODY_PARAM = 2

    def __init__(self, executor, tp: ThreadPoolExecutor = None):
        self.__executor = executor
        self.__health = False
        self.__tp = tp

        self.__http_streaming_ctrl_mode = HttpHandler.HttpStreamingCtrlMode.QUERY_PARAM
        self.__http_streaming_ctrl_key = 'streaming'
        self.__http_streaming_res_content_type = 'application/octet-stream'
        customized_predict_http = global_conf.server_conf.get('interface').get('customized_predict_http')
        if customized_predict_http:
            streaming_ctrl = customized_predict_http.get('streaming_ctrl')
            if streaming_ctrl:
                ctrl_mode = streaming_ctrl.get('ctrl_mode')
                if ctrl_mode:
                    if type(ctrl_mode) is not str:
                        raise ValueError('server.yml: streaming control mode must be str.')
                    if ctrl_mode == "" or ctrl_mode == 'query_param':
                        self.__http_streaming_ctrl_mode = HttpHandler.HttpStreamingCtrlMode.QUERY_PARAM
                    elif ctrl_mode == 'header_param':
                        self.__http_streaming_ctrl_mode = HttpHandler.HttpStreamingCtrlMode.HEADER_PARAM
                    elif ctrl_mode == 'body_param':
                        self.__http_streaming_ctrl_mode = HttpHandler.HttpStreamingCtrlMode.BODY_PARAM
                    else:
                        raise ValueError('server.yml: streaming control mode {} not supported.'.format(ctrl_mode))

                ctrl_key = streaming_ctrl.get('ctrl_key')
                if ctrl_key:
                    if type(ctrl_key) is not str:
                        raise ValueError('server.yml: streaming control key must be str.')
                    if ctrl_key != "":
                        self.__http_streaming_ctrl_key = ctrl_key

                res_content_type = streaming_ctrl.get('res_content_type')
                if res_content_type:
                    if type(res_content_type) is not str:
                        raise ValueError('server.yml: streaming response content type must be str.')
                    if res_content_type != "":
                        self.__http_streaming_res_content_type = res_content_type

    @staticmethod
    def __jsonify(res, code):
        return (MessageToJson(res, including_default_value_fields=False, preserving_proto_field_name=True, indent=1),
                code, [('Content-Type', 'application/json;charset=utf-8')])

    def online(self):
        ip = request.remote_addr
        self.__health = True
        logger.info('[Online] from client: {}'.format(ip))
        res = GrpsMessage(status=Status(code=http.HTTPStatus.OK.value, msg='OK', status=Status.SUCCESS))
        return self.__jsonify(res, http.HTTPStatus.OK.value)

    def offline(self):
        ip = request.remote_addr
        self.__health = False
        logger.info('[Offline] from client: {}'.format(ip))
        res = GrpsMessage(status=Status(code=http.HTTPStatus.OK.value, msg='OK', status=Status.SUCCESS))
        return self.__jsonify(res, http.HTTPStatus.OK.value)

    def ready(self):
        logger.info('[CheckReadiness] from client: {}'.format(request.remote_addr))
        if self.__health:
            res = GrpsMessage(status=Status(code=http.HTTPStatus.OK.value, msg='OK', status=Status.SUCCESS))
            return self.__jsonify(res, http.HTTPStatus.OK.value)
        else:
            res = GrpsMessage(status=Status(code=http.HTTPStatus.SERVICE_UNAVAILABLE.value, msg='Service Unavailable',
                                            status=Status.FAILURE))
            return self.__jsonify(res, http.HTTPStatus.SERVICE_UNAVAILABLE.value)

    def live(self):
        logger.info('[CheckLiveness] from client: {}'.format(request.remote_addr))
        res = GrpsMessage(status=Status(code=http.HTTPStatus.OK.value, msg='OK', status=Status.SUCCESS))
        return self.__jsonify(res, http.HTTPStatus.OK.value)

    def server_metadata(self):
        logger.info('[ServerMetadata] from client: {}'.format(request.remote_addr))
        with open(INFERENCE_CONF_PATH, 'r') as f:
            metadata = f.read()
        with open(SERVER_CONF_PATH, 'r') as f:
            metadata += '\n' + f.read()
        res = GrpsMessage(status=Status(code=http.HTTPStatus.OK.value, msg='OK', status=Status.SUCCESS),
                          str_data=metadata)
        return self.__jsonify(res, http.HTTPStatus.OK.value)

    def model_metadata(self):
        logger.info('[ModelMetadata] from client: {}'.format(request.remote_addr))
        body = request.get_json()
        if body.get('str_data') is None:
            res = GrpsMessage(status=Status(code=http.HTTPStatus.BAD_REQUEST.value, msg='No model name.',
                                            status=Status.FAILURE))
            return self.__jsonify(res, http.HTTPStatus.BAD_REQUEST.value)
        model_name = body['str_data']
        for model_desc in global_conf.inference_conf['models']:
            if model_desc['name'] == model_name:
                res = GrpsMessage(status=Status(code=http.HTTPStatus.OK.value, msg='OK', status=Status.SUCCESS),
                                  str_data=yaml.dump(model_desc))
                return self.__jsonify(res, http.HTTPStatus.OK.value)
        res = GrpsMessage(status=Status(code=http.HTTPStatus.NOT_FOUND.value, msg='Model not found.',
                                        status=Status.FAILURE))
        return self.__jsonify(res, http.HTTPStatus.NOT_FOUND.value)

    def if_streaming(self, request):
        """
        Check if request is streaming.
        Args:
            request: flask request.

        Returns:
            bool.
        """
        if self.__http_streaming_ctrl_mode == HttpHandler.HttpStreamingCtrlMode.QUERY_PARAM:
            streaming_query_arg = request.args.get(self.__http_streaming_ctrl_key)
            if streaming_query_arg is not None and streaming_query_arg.lower() == 'true':
                return True
            return False
        elif self.__http_streaming_ctrl_mode == HttpHandler.HttpStreamingCtrlMode.HEADER_PARAM:
            streaming_header_arg = request.headers.get(self.__http_streaming_ctrl_key)
            if streaming_header_arg is not None and streaming_header_arg.lower() == 'true':
                return True
            return False
        elif self.__http_streaming_ctrl_mode == HttpHandler.HttpStreamingCtrlMode.BODY_PARAM:
            body = request.get_data()
            if body is None:
                return False
            try:
                js_dict = json.loads(body)
                if (js_dict.get(self.__http_streaming_ctrl_key) is not None and
                        type(js_dict[self.__http_streaming_ctrl_key]) is bool and
                        js_dict[self.__http_streaming_ctrl_key]):
                    return True
                return False
            except Exception as e:
                return False

    def predict(self):
        """
        Infer predict http url handler.
        Returns:
            flask response.
        """
        try:
            content_type = request.headers.get('content-type')
            if content_type is None:
                app_monitor.avg(REQ_FAIL_RATE, 100)
                res = GrpsMessage(
                    status=Status(code=http.HTTPStatus.BAD_REQUEST.value, msg='No content type in headers.',
                                  status=Status.FAILURE))
                return self.__jsonify(res, http.HTTPStatus.BAD_REQUEST.value)
            if content_type in ['application/json', 'application/octet-stream']:
                body = request.get_data()
            else:
                app_monitor.avg(REQ_FAIL_RATE, 100)
                res = GrpsMessage(status=Status(code=http.HTTPStatus.BAD_REQUEST.value, msg='Unsupported content type.',
                                                status=Status.FAILURE))
                return self.__jsonify(res, http.HTTPStatus.BAD_REQUEST.value)

            # get streaming arg.
            is_streaming = self.if_streaming(request)

            # get return-ndarray arg.
            ret_ndarray = False
            ret_ndarray_query_arg = request.args.get('return-ndarray')
            if ret_ndarray_query_arg is not None and ret_ndarray_query_arg.lower() == 'true':
                ret_ndarray = True

            if is_streaming and ret_ndarray:
                app_monitor.avg(REQ_FAIL_RATE, 100)
                res = GrpsMessage(status=Status(
                    code=http.HTTPStatus.BAD_REQUEST.value,
                    msg='Bad Request, err: Streaming and ret ndarray are not supported at the same time.',
                    status=Status.FAILURE))
                return self.__jsonify(res, http.HTTPStatus.BAD_REQUEST.value)

            grps_context = GrpsContext()
            if is_streaming:
                grps_context.start_http_streaming_generator()

            task = self.__tp.submit(self.__predict_task, content_type, body, request._get_current_object(),
                                    is_streaming, ret_ndarray, grps_context)
            if is_streaming:
                return Response(stream_with_context(grps_context.http_streaming_generator()),
                                mimetype=self.__http_streaming_res_content_type)
            else:
                res = task.result()
                return res
        except Exception as e:
            trace_back = traceback.format_exc()
            logger.error(trace_back)
            app_monitor.avg(REQ_FAIL_RATE, 100)
            res = GrpsMessage(status=Status(code=http.HTTPStatus.INTERNAL_SERVER_ERROR.value, msg=str(trace_back),
                                            status=Status.FAILURE))
            return self.__jsonify(res, http.HTTPStatus.INTERNAL_SERVER_ERROR.value)

    def __predict_task(self, content_type, body, request: Request, is_streaming, ret_ndarray, grps_context):
        try:
            begin = time.time()
            res = self.__predict_impl(content_type, body, request, is_streaming, ret_ndarray, grps_context)
            total_latency_ms = (time.time() - begin) * 1e3
            app_monitor.avg(REQ_LATENCY_AVG, total_latency_ms)
            app_monitor.max(REQ_LATENCY_MAX, total_latency_ms)
            app_monitor.cdf(REQ_LATENCY_CDF, total_latency_ms)
            logger.info('[Predict] from client: {}, latency: {:.2f} ms'.format(request.remote_addr, total_latency_ms))
        except Exception as e:
            trace_back = traceback.format_exc()
            logger.error(trace_back)
            app_monitor.avg(REQ_FAIL_RATE, 100)
            res = 'Internal Server Error: {}'.format(trace_back), http.HTTPStatus.INTERNAL_SERVER_ERROR.value, \
                [('Content-Type', 'text/plain;charset=utf-8')]
        finally:
            if is_streaming:
                grps_context.stop_http_streaming_generator()
                return None
        return res

    def __predict_impl(self, content_type, body, request: Request, is_streaming, ret_ndarray, grps_context):
        """
        Infer predict implementation.

        Args:
            content_type: request content type.
            body: body.
            request: flask request.
            is_streaming: is streaming request.
            ret_ndarray: if return ndarray.
            grps_context: grps context.

        Returns:
            flask response.
        """
        if not body:
            app_monitor.avg(REQ_FAIL_RATE, 100)
            res = GrpsMessage(status=Status(code=http.HTTPStatus.BAD_REQUEST.value, msg='The http body is empty.',
                                            status=Status.FAILURE))
            if is_streaming:
                grps_context.stream_respond(res)
                return None
            return self.__jsonify(res, http.HTTPStatus.BAD_REQUEST.value)

        app_monitor.inc(QPS, 1)

        try:
            # Get model name from args.
            model_name = request.args.get('model', None)

            # 1. Parse different content type of request.
            if content_type == 'application/json':
                js_dict = json.loads(body)
                if js_dict.get('str_data') or js_dict.get('gtensors') or js_dict.get('gmap'):
                    infer_in = ParseDict(js_dict, GrpsMessage(), ignore_unknown_fields=True)
                elif js_dict.get('ndarray'):
                    ndarray = np.array(js_dict['ndarray'])
                    infer_in = GrpsMessage()
                    gtensor = GenericTensor(dtype=DataType.DT_FLOAT32, shape=ndarray.shape,
                                            flat_float32=ndarray.flatten().tolist())
                    infer_in.gtensors.tensors.append(gtensor)
                elif js_dict.get('bin_data'):
                    raise Exception('bin_data should use application/octet-stream format.')
                else:
                    raise Exception('No legal field in json.')
                # Override model name if model name is in json.
                if js_dict.get('model') and type(js_dict['model']) is str:
                    model_name = js_dict['model']
            elif content_type.startswith('application/octet-stream'):
                infer_in = GrpsMessage(bin_data=body)
            else:
                raise Exception('Unsupported content type{}.'.format(content_type))

            # 2. Execute predict.
            if str(type(infer_in)) != str(GrpsMessage):
                logger.error('Input type must be GrpsMessage.')
                grps_context.set_err_msg('Input type must be GrpsMessage.')

            infer_out = self.__executor.infer(infer_in, grps_context, model_name)

            if not grps_context.has_err() and str(type(infer_out)) != str(GrpsMessage) and not is_streaming:
                logger.error('Output type must be GrpsMessage.')
                grps_context.set_err_msg('Output type must be GrpsMessage.')

            if grps_context.has_err():
                app_monitor.avg(REQ_FAIL_RATE, 100)
                logger.error('Predict error: {}'.format(grps_context.get_err_msg()))
                res = GrpsMessage(
                    status=Status(code=http.HTTPStatus.INTERNAL_SERVER_ERROR.value, msg=str(grps_context.get_err_msg()),
                                  status=Status.FAILURE))
                if is_streaming:
                    grps_context.stream_respond(res)
                    return None
                return self.__jsonify(res, http.HTTPStatus.INTERNAL_SERVER_ERROR.value)

            if is_streaming:
                app_monitor.avg(REQ_FAIL_RATE, 0)
                return None

            # 3. Return response.
            if infer_out.bin_data:
                app_monitor.avg(REQ_FAIL_RATE, 0)
                res = infer_out.bin_data
                return res, http.HTTPStatus.OK.value, [('Content-Type', 'application/octet-stream')]
            else:
                infer_out.status.status = Status.SUCCESS
                infer_out.status.code = http.HTTPStatus.OK.value
                infer_out.status.msg = 'OK'
                if ret_ndarray:  # gtensor to ndarray.
                    js_dict = MessageToDict(infer_out, including_default_value_fields=True)
                    if len(infer_out.gtensors.tensors) == 0 or \
                            infer_out.gtensors.tensors[0].dtype != DataType.DT_FLOAT32:
                        res = GrpsMessage(
                            status=Status(code=http.HTTPStatus.INTERNAL_SERVER_ERROR.value,
                                          msg='No float32 tensors in output. Cannot convert to ndarray.',
                                          status=Status.FAILURE))
                        if is_streaming:
                            grps_context.stream_respond(res)
                            return None
                        app_monitor.avg(REQ_FAIL_RATE, 100)
                        return self.__jsonify(res, http.HTTPStatus.INTERNAL_SERVER_ERROR.value)
                    ndarray = np.array(infer_out.gtensors.tensors[0].flat_float32, dtype=np.float32).reshape(
                        infer_out.gtensors.tensors[0].shape)
                    js_dict['ndarray'] = ndarray.tolist()
                    del js_dict['gtensors']
                    app_monitor.avg(REQ_FAIL_RATE, 0)
                    return (json.dumps(js_dict, indent=1), http.HTTPStatus.OK.value,
                            [('Content-Type', 'application/json;charset=utf-8')])
                else:
                    app_monitor.avg(REQ_FAIL_RATE, 0)
                    return self.__jsonify(infer_out, http.HTTPStatus.OK.value)
        except Exception as e:
            trace_back = traceback.format_exc()
            logger.error(trace_back)

            # Gpu oom statistic.
            if trace_back.find('CUDA out of memory') != -1 or trace_back.find('OOM') != -1:
                app_monitor.inc(GPU_OOM_COUNT, 1)

            app_monitor.avg(REQ_FAIL_RATE, 100)
            res = GrpsMessage(status=Status(code=http.HTTPStatus.INTERNAL_SERVER_ERROR.value, msg=str(trace_back),
                                            status=Status.FAILURE))
            if is_streaming:
                grps_context.stream_respond(res)
                return None
            return self.__jsonify(res, http.HTTPStatus.INTERNAL_SERVER_ERROR.value)

    def predict_custom_http(self):
        """
        Infer predict http with custom url handler.
        Returns:
            flask response.
        """
        try:
            # get streaming arg.
            is_streaming = self.if_streaming(request)

            grps_context = GrpsContext(http_request=request._get_current_object())
            if is_streaming:
                grps_context.start_http_streaming_generator()

            task = self.__tp.submit(self.__predict_impl_with_custom_body_task, request._get_current_object(),
                                    is_streaming, grps_context)
            if is_streaming:
                return Response(stream_with_context(grps_context.http_streaming_generator()),
                                mimetype=self.__http_streaming_res_content_type)
            else:
                res = task.result()
                return res
        except Exception as e:
            trace_back = traceback.format_exc()
            logger.error(trace_back)
            app_monitor.avg(REQ_FAIL_RATE, 100)
            return self.__jsonify(str(trace_back), http.HTTPStatus.INTERNAL_SERVER_ERROR.value)

    def __predict_impl_with_custom_body_task(self, request: Request, is_streaming, grps_context):
        try:
            begin = time.time()
            res = self.__predict_impl_with_custom_body(request, is_streaming, grps_context)
            total_latency_ms = (time.time() - begin) * 1e3
            app_monitor.avg(REQ_LATENCY_AVG, total_latency_ms)
            app_monitor.max(REQ_LATENCY_MAX, total_latency_ms)
            app_monitor.cdf(REQ_LATENCY_CDF, total_latency_ms)
            logger.info('[Predict] from client: {}, latency: {:.2f} ms'.format(request.remote_addr, total_latency_ms))
        except Exception as e:
            trace_back = traceback.format_exc()
            logger.error(trace_back)
            app_monitor.avg(REQ_FAIL_RATE, 100)
            res = 'Internal Server Error: {}'.format(trace_back), http.HTTPStatus.INTERNAL_SERVER_ERROR.value, \
                [('Content-Type', 'text/plain;charset=utf-8')]
        finally:
            if is_streaming:
                grps_context.stop_http_streaming_generator()
                return None
        return res

    def __predict_impl_with_custom_body(self, request: Request, is_streaming, grps_context):
        """
        Infer predict implementation.

        Args:
            request: flask request.
            is_streaming: is streaming request.
            grps_context: grps context.

        Returns:
            flask response.
        """
        app_monitor.inc(QPS, 1)

        try:
            # Get model name from args.
            model_name = request.args.get('model', None)
            infer_in = GrpsMessage()
            self.__executor.infer(infer_in, grps_context, model_name)

            if grps_context.has_err():
                app_monitor.avg(REQ_FAIL_RATE, 100)
                logger.error('Predict error: {}'.format(grps_context.get_err_msg()))

            app_monitor.avg(REQ_FAIL_RATE, 0)

            if is_streaming:
                return None

            if grps_context.get_http_response() is not None:
                return grps_context.get_http_response()
            else:
                return '', http.HTTPStatus.OK.value, {'Content-Type': 'text/plain'}
        except Exception as e:
            trace_back = traceback.format_exc()
            logger.error(trace_back)

            # Gpu oom statistic.
            if trace_back.find('CUDA out of memory') != -1 or trace_back.find('OOM') != -1:
                app_monitor.inc(GPU_OOM_COUNT, 1)

            app_monitor.avg(REQ_FAIL_RATE, 100)
            if is_streaming:
                grps_context.customized_http_stream_respond(str(trace_back))
                return None
            return str(trace_back), http.HTTPStatus.INTERNAL_SERVER_ERROR.value, {'Content-Type': 'text/plain'}

    def monitor_data(self):
        name = request.args.get('name')
        monitor_data = app_monitor.read(name)
        if isinstance(monitor_data, str):
            return monitor_data
        else:
            label = monitor_data['label']
            if label == 'cdf':
                for i in range(0, len(monitor_data['data'])):
                    monitor_data['data'][i][1] = round(monitor_data['data'][i][1], 2)
                return json.dumps(monitor_data), http.HTTPStatus.OK.value, [
                    ('Content-Type', 'application/json;charset=utf-8')]

            # trend
            data = []
            monitor_data = monitor_data['data']
            for i in range(0, len(monitor_data)):
                if monitor_data[i] is None:
                    data.append([i, 0])
                else:
                    data.append([i, round(monitor_data[i], 2)])
            result = {
                'label': label,
                'data': data
            }
            return json.dumps(result), http.HTTPStatus.OK.value, [('Content-Type', 'application/json;charset=utf-8')]
