# Grpc client demo. Complete interface description can be learned from docs/2_Interface.md

import sys

import grpc
from grps_apis.grps_pb2 import GrpsMessage, GenericTensor, DataType
from grps_apis.grps_pb2_grpc import GrpsServiceStub


def grpc_request(server):
    conn = grpc.insecure_channel(server)
    client = GrpsServiceStub(channel=conn)

    # check liveness.
    request = GrpsMessage()
    response = client.CheckLiveness(request)
    print(response)

    # online server.
    request = GrpsMessage()
    response = client.Online(request)
    print(response)

    # check readiness.
    request = GrpsMessage()
    response = client.CheckReadiness(request)
    print(response)

    # predict with str_data.
    request = GrpsMessage(str_data='hello grps.')
    response = client.Predict(request)
    print(response)

    # predict with gtensors.
    request = GrpsMessage()
    gtensor = GenericTensor(name='inp', dtype=DataType.DT_FLOAT32, shape=[2, 3], flat_float32=[1, 2, 3, 4, 5, 6])
    request.gtensors.tensors.append(gtensor)
    response = client.Predict(request)
    print(response)

    # predict with bin_data.
    request = GrpsMessage(bin_data=b'hello grps.')
    response = client.Predict(request)
    print(response)

    # predict with gmap.
    request = GrpsMessage()
    request.gmap.s_s['key'] = 'hello grps.'
    request.gmap.s_b['key'] = b'hello grps.'
    request.gmap.s_i32['key'] = 1
    request.gmap.s_i64['key'] = 1
    request.gmap.s_f['key'] = 1.0
    request.gmap.s_d['key'] = 1.0
    response = client.Predict(request)
    print(response)

    # predict streaming.
    request = GrpsMessage(str_data='hello grps.')
    response = client.PredictStreaming(request)
    for resp in response:
        print(resp)

    request = GrpsMessage()
    response = client.ServerMetadata(request)
    print(response)

    request = GrpsMessage(str_data='your_model')
    response = client.ModelMetadata(request)
    print(response)

    request = GrpsMessage()
    response = client.Offline(request)
    print(response)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: python3 grpc_client.py <server>')
        sys.exit(1)

    grpc_request(sys.argv[1])
