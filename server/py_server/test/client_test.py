import os
import sys
import unittest

import grpc
import requests

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from grps_framework.apis.grps_pb2 import GrpsMessage
from grps_framework.apis.grps_pb2_grpc import GrpsServiceStub


class MyTestCase(unittest.TestCase):
    def test_grpc_api(self):
        conn = grpc.insecure_channel('0.0.0.0:7081')
        client = GrpsServiceStub(channel=conn)

        request = GrpsMessage()
        response = client.CheckLiveness(request)
        self.assertEqual(response.status.status, 0)

        request = GrpsMessage()
        response = client.Online(request)
        self.assertEqual(response.status.status, 0)

        request = GrpsMessage()
        response = client.CheckReadiness(request)
        self.assertEqual(response.status.status, 0)

        request = GrpsMessage(str_data='hello grps.')
        response = client.Predict(request)
        self.assertEqual(response.str_data, 'hello grps.')
        self.assertEqual(response.status.status, 0)

        request = GrpsMessage(str_data='hello grps.')
        response = client.PredictStreaming(request)
        result = []
        for i in response:
            result.append(i.str_data)
        # print(result)
        for cnt in range(1, 3):
            self.assertEqual(result[cnt - 1], 'stream data ' + str(cnt))

        request = GrpsMessage()
        response = client.ServerMetadata(request)
        # print(response.str_data)
        self.assertEqual(response.status.status, 0)

        request = GrpsMessage(str_data='your_model')
        response = client.ModelMetadata(request)
        # print(response.str_data)
        self.assertEqual(response.status.status, 0)

        request = GrpsMessage()
        response = client.Offline(request)
        self.assertEqual(response.status.status, 0)

    def test_http_apis(self):
        url = 'http://0.0.0.0:7080'

        response = requests.get(url + '/grps/v1/health/live').json()
        self.assertEqual(response['status']['status'], 'SUCCESS')

        response = requests.get(url + '/grps/v1/health/online').json()
        self.assertEqual(response['status']['status'], 'SUCCESS')

        response = requests.get(url + '/grps/v1/health/ready').json()
        self.assertEqual(response['status']['status'], 'SUCCESS')

        response = requests.post(url + '/grps/v1/infer/predict', json={'str_data': 'hello grps.'}).json()
        self.assertEqual(response['str_data'], 'hello grps.')
        self.assertEqual(response['status']['status'], 'SUCCESS')

        response = requests.get(url + '/grps/v1/metadata/server').json()
        # print(response['str_data'])
        self.assertEqual(response['status']['status'], 'SUCCESS')

        response = requests.post(url + '/grps/v1/metadata/model', json={'str_data': 'your_model'}).json()
        # print(response['str_data'])
        self.assertEqual(response['status']['status'], 'SUCCESS')

        response = requests.get(url + '/grps/v1/health/offline').json()
        self.assertEqual(response['status']['status'], 'SUCCESS')


if __name__ == '__main__':
    unittest.main()
