# Copyright 2023 netease. All rights reserved.
# Author huangchengbo@corp.netease.com
# Date   2023/10/7
# Brief  Grps context. Use grps context, you can save and get user data, send streaming response, set error message,
#        or implement customized http.
import http
import queue
import concurrent.futures
import platform

import flask
from flask import Request
from google.protobuf.json_format import MessageToJson

from grps_framework.apis.grps_pb2 import GrpsMessage, Status


class GrpsContext(object):
    def __init__(self, grpc_context=None, http_request: Request = None):
        self._data = {}
        self._grpc_context = grpc_context
        self._http_request = http_request
        self._has_err = False
        self._err_msg = ''
        self._http_response = None
        self._http_streaming_queue = queue.Queue()
        self._http_streaming_run = False
        self._grpc_streaming_queue = queue.Queue()
        self._grpc_streaming_run = False
        self._converter = None
        self._inferer = None
        self._batcher_future = None
        self._if_disconnected = False

    # ---------------------------- User data function. ----------------------------

    def put_user_data(self, key, value):
        """
        Put user data into context.

        Args:
            key: key of user data.
            value: value of user data.
        """
        self._data[key] = value

    def get_user_data(self, key):
        """
        Get user data from context.
        Args:
            key: key of user data.

        Returns:
            None if key not in context, else return value.
        """
        if key not in self._data.keys():
            return None
        else:
            return self._data[key]

    # ---------------------------- Streaming function. ----------------------------

    def if_streaming(self):
        """If using streaming predict request"""
        if self._grpc_context is not None and self._grpc_streaming_run:
            return True
        elif self._http_streaming_run:
            return True
        else:
            return False

    def stream_respond(self, msg: GrpsMessage, final=False):
        """
        Streaming respond when using streaming predict request.

        Args:
            msg: GrpsMessage to be sent to client.
            final: If final is true, that means the msg is the last message of the streaming response. Your should never
            respond any message after that. In batching request mode, you should never use request message and output
            message anymore, those will be invalid.
        """
        if str(type(msg)) != str(GrpsMessage):
            raise ValueError('Streaming response should be GrpsMessage')

        if msg.status is None or msg.status.status != Status.FAILURE:
            msg.status.status = Status.SUCCESS
            msg.status.code = http.HTTPStatus.OK.value
            msg.status.msg = 'OK'

        if self._grpc_context is not None and self._grpc_streaming_run:
            self._grpc_streaming_queue.put(msg)
            if final:
                self.stop_grpc_streaming_generator()
        elif self._http_streaming_run:
            self._http_streaming_queue.put(msg)
            if final:
                self.stop_http_streaming_generator()

        if final and self._batcher_future:
            self.batcher_future_notify()

    def stream_respond_with_postprocess(self, inp, final=False):
        """
        Streaming respond with postprocess when using streaming predict request.
        Args:
            inp: Inferer output.
            final: If final is true, that means the msg is the last message of the streaming response. Your should never
            respond any message after that. In batching request mode, you should never use request message and output
            message anymore, those will be invalid.
        """
        if self._converter is None:
            raise ValueError('stream_respond_with_postprocess should only be used with converter')
        self.stream_respond(self._converter.postprocess(inp, self), final)

    def http_streaming_generator(self):
        """
        [Only call by grps framework.]
        Http streaming response generator when using http streaming predict request.
        """
        while self._http_streaming_run:
            msg = self._http_streaming_queue.get()
            if str(type(msg)) == str(GrpsMessage):
                try:
                    if msg.bin_data:
                        yield str(msg.bin_data)
                    else:
                        yield MessageToJson(msg, including_default_value_fields=False, preserving_proto_field_name=True)
                except GeneratorExit as e:
                    self._if_disconnected = True
            elif isinstance(msg, str) or isinstance(msg, bytes):
                try:
                    yield msg
                except GeneratorExit as e:
                    self._if_disconnected = True

    def start_http_streaming_generator(self):
        """
        [Only call by grps framework.]
        Start http streaming response generator when using http streaming predict request.
        """
        self._http_streaming_run = True

    def stop_http_streaming_generator(self):
        """
        [Only call by grps framework.]
        Stop http streaming response generator when using http streaming predict request.
        """
        self._http_streaming_run = False
        self._http_streaming_queue.put(None)  # put None to stop generator

    def start_grpc_streaming_generator(self):
        """
        [Only call by grps framework.]
        Start grpc streaming response generator when using grpc streaming predict request.
        """
        self._grpc_streaming_run = True

    def stop_grpc_streaming_generator(self):
        """
        [Only call by grps framework.]
        Stop grpc streaming response generator when using grpc streaming predict request.
        """
        self._grpc_streaming_run = False
        self._grpc_streaming_queue.put(None)

    # ---------------------------- Error function. ----------------------------

    def set_has_err(self, has_err: bool):
        """Set has err. If has err, predict process will be terminated and will return error message to client."""
        self._has_err = has_err

    def has_err(self):
        """Get has err"""
        return self._has_err

    def set_err_msg(self, err_msg: str):
        """Set err_msg that will be returned to client"""
        self._has_err = True
        self._err_msg = err_msg

    def get_err_msg(self):
        """Get err_msg"""
        return self._err_msg

    # ---------------------------- Customized http function. ----------------------------

    def get_http_request(self) -> Request:
        """If using customized http, return flask request, else will return None."""
        return self._http_request

    def set_http_response(self, http_response):
        """
        Set response to client while you are using customized http request.
        Note that it is useless if using streaming request, you should use customized_http_stream_respond instead.

        Http_response should be one of the following:
        1. You can use str directly, Content-Type will be set to text/plain and status will be set to 200:
            set_http_response('Hello World')
        2. You can use bytes directly, Content-Type will be set to application/octet-stream and status will be set to 200:
            set_http_response(b'Hello World')
        3. You can use tuple with 3 elements:
            set_http_response(('Hello World', 200, {'Content-Type': 'text/plain'}))
            set_http_response((b'Hello World', 200, {'Content-Type': 'application/octet-stream'}))
        4. You can use flask response:
            set_http_response(flask.Response(response='Hello World', status=200, content_type="text/plain"))
        """
        if http_response is None:
            raise ValueError('http_response should not be None')
        elif isinstance(http_response, str):
            http_response = flask.Response(http_response, 200, {'Content-Type': 'text/plain'})
        elif isinstance(http_response, bytes):
            http_response = flask.Response(http_response, 200, {'Content-Type': 'application/octet-stream'})
        elif isinstance(http_response, tuple) and len(http_response) == 3:
            if not (isinstance(http_response[0], str) or isinstance(http_response[0], bytes)):
                raise ValueError('http_response[0] should be str or bytes')
            if not isinstance(http_response[1], int):
                raise ValueError('http_response[1] should be int')
            if not isinstance(http_response[2], dict):
                raise ValueError('http_response[2] should be dict')
            http_response = flask.Response(http_response[0], http_response[1], http_response[2])
        elif not isinstance(http_response, flask.Response):
            raise ValueError('http_response should be str, bytes, tuple(str/bytes, status, headers) or flask.Response')

        self._http_response = http_response

    def customized_http_stream_respond(self, msg, final=False):
        """
        Streaming respond when using customized http streaming predict request.

        Args:
            msg: string or bytes message to be sent to client.
            final: If final is true, that means the msg is the last message of the streaming response. Your should never
            respond any message after that. In batching request mode, you should never use request message and output
            message anymore, those will be invalid.
        """
        if not isinstance(msg, str) and not isinstance(msg, bytes):
            raise ValueError('Streaming response should be str or bytes')
        if self._http_request is None:
            raise ValueError('customized_http_stream_respond should only be used with customized http')
        if self._http_streaming_run:
            self._http_streaming_queue.put(msg)

        if final:
            self.stop_http_streaming_generator()
            if self._batcher_future:
                self.batcher_future_notify()

    def get_http_response(self):
        """
        [Only call by grps framework.]
        """
        return self._http_response

    # ---------------------------- Batching function. ----------------------------
    def set_batcher_future(self, future):
        """
        [Only call by grps framework.]
        """
        self._batcher_future = future

    def batcher_future_notify(self):
        """
        [Only call by grps framework.]
        """
        if platform.python_version_tuple() >= ('3', '8'):
            try:
                self._batcher_future.set_result(True)
            except concurrent.futures.InvalidStateError as e:
                pass
            except Exception as e:
                self.set_err_msg(str(e))
                self.set_has_err(True)
        else:
            try:
                self._batcher_future.set_result(True)
            except Exception as e:
                self.set_err_msg(str(e))
                self.set_has_err(True)

    # ---------------------------- Other function. ----------------------------
    def set_converter(self, converter):
        """
        [Only call by grps framework.]
        """
        self._converter = converter

    @property
    def converter(self):
        return self._converter

    def set_inferer(self, inferer):
        """
        [Only call by grps framework.]
        """
        self._inferer = inferer

    @property
    def inferer(self):
        return self._inferer

    def if_disconnected(self):
        """If connection with client is disconnected. [NOTE: Do not support http non-streaming request]"""
        if self._grpc_context is not None:
            self._if_disconnected = not self._grpc_context.is_active()
        return self._if_disconnected
