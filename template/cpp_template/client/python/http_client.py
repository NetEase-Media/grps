# Http client demo. Complete interface description can be learned from docs/2_Interface.md.

import base64
import sys

import requests


def http_request(server):
    url = f'http://{server}'

    # check liveness.
    response = requests.get(url + '/grps/v1/health/live').json()
    print(response)

    # online server.
    response = requests.get(url + '/grps/v1/health/online').json()
    print(response)

    # check readiness.
    response = requests.get(url + '/grps/v1/health/ready').json()
    print(response)

    # predict with str_data.
    response = requests.post(url + '/grps/v1/infer/predict', json={'str_data': 'hello grps.'}).json()
    print(response)

    # streaming predict.
    response = requests.post(url + '/grps/v1/infer/predict?streaming=true', json={'str_data': 'hello grps.'},
                             stream=True)
    for trunk in response.iter_content(chunk_size=None):
        print(trunk)

    # predict with gtensors.
    g_tensors = {
        'tensors': [{
            'name': 'inp',
            'dtype': 'DT_FLOAT32',
            'shape': [2, 3],
            'flat_float32': [1, 2, 3, 4, 5, 6]
        }]
    }
    response = requests.post(url + '/grps/v1/infer/predict', json={'gtensors': g_tensors}).json()
    print(response)

    # predict with gmap.
    gmap = {
        's_s': {'key': 'value'},
        's_b': {'key': base64.b64encode(b'hello grps.').decode('utf-8')},
        's_i32': {'key': 1},
        's_i64': {'key': 1},
        's_f': {'key': 1.0},
        's_d': {'key': 1.0},
        's_t': {'key': {
            'dtype': 'DT_FLOAT32',
            'shape': [2, 3],
            'flat_float32': [1, 2, 3, 4, 5, 6]
        }},
    }
    response = requests.post(url + '/grps/v1/infer/predict', json={'gmap': gmap}).json()
    print(response)

    # predict with ndarray and return ndarray.
    response = requests.post(url + '/grps/v1/infer/predict?return-ndarray=true',
                             json={'ndarray': [[1, 2, 3], [4, 5, 6]]}).json()
    print(response)

    # predict with bin_data.
    response = requests.post(url + '/grps/v1/infer/predict', data=r'hello grps.',
                             headers={'content-type': 'application/octet-stream'}).content
    print(response)

    # get server metadata.
    response = requests.get(url + '/grps/v1/metadata/server').json()
    print(response)

    # get model metadata.
    response = requests.post(url + '/grps/v1/metadata/model', json={'str_data': 'your_model'}).json()
    print(response)

    # offline server.
    response = requests.get(url + '/grps/v1/health/offline').json()
    print(response)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: python3 http_client.py <server>')
        sys.exit(1)

    http_request(sys.argv[1])
