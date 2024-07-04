# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/9/4
# Brief  Tensorflow converter unittest.

import os
import sys
import unittest

import tensorflow as tf

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from grps_framework.converter.tf_tensor_converter import TfTensorConverter
from grps_framework.apis.grps_pb2 import GrpsMessage, GenericTensor, DataType
from grps_framework.context.context import GrpsContext


class MyTestCase(unittest.TestCase):
    converter = TfTensorConverter()

    def test_uint8(self):
        grps_msg = GrpsMessage()
        context = GrpsContext()
        gtensor = GenericTensor(dtype=DataType.DT_UINT8, shape=[2, 3], flat_uint8=[1, 2, 3, 4, 5, 6])
        grps_msg.gtensors.names.append('inp')
        grps_msg.gtensors.tensors.append(gtensor)
        tf_tensor_map = self.converter.preprocess(grps_msg, context)
        # print(tf_tensor_map)
        self.assertTrue('inp' in tf_tensor_map)
        self.assertTrue(tf_tensor_map['inp'].shape == (2, 3))
        self.assertTrue(tf_tensor_map['inp'].dtype == tf.uint8)
        self.assertTrue(tf_tensor_map['inp'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])

        grps_msg = self.converter.postprocess(tf_tensor_map, context)
        # print(grps_msg)
        self.assertTrue(len(grps_msg.gtensors.names) == 1)
        self.assertTrue(len(grps_msg.gtensors.tensors) == 1)
        self.assertTrue(grps_msg.gtensors.names[0] == 'inp')
        self.assertTrue(grps_msg.gtensors.tensors[0].dtype == DataType.DT_UINT8)
        self.assertTrue(grps_msg.gtensors.tensors[0].shape == [2, 3])
        self.assertTrue(grps_msg.gtensors.tensors[0].flat_uint8 == [1, 2, 3, 4, 5, 6])

    def test_int8(self):
        grps_msg = GrpsMessage()
        context = GrpsContext()
        gtensor = GenericTensor(dtype=DataType.DT_INT8, shape=[2, 3], flat_int8=[1, 2, 3, 4, 5, 6])
        grps_msg.gtensors.names.append('inp')
        grps_msg.gtensors.tensors.append(gtensor)
        tf_tensor_map = self.converter.preprocess(grps_msg, context)
        # print(tf_tensor_map)
        self.assertTrue('inp' in tf_tensor_map)
        self.assertTrue(tf_tensor_map['inp'].shape == (2, 3))
        self.assertTrue(tf_tensor_map['inp'].dtype == tf.int8)
        self.assertTrue(tf_tensor_map['inp'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])

        grps_msg = self.converter.postprocess(tf_tensor_map, context)
        # print(grps_msg)
        self.assertTrue(len(grps_msg.gtensors.names) == 1)
        self.assertTrue(len(grps_msg.gtensors.tensors) == 1)
        self.assertTrue(grps_msg.gtensors.names[0] == 'inp')
        self.assertTrue(grps_msg.gtensors.tensors[0].dtype == DataType.DT_INT8)
        self.assertTrue(grps_msg.gtensors.tensors[0].shape == [2, 3])
        self.assertTrue(grps_msg.gtensors.tensors[0].flat_int8 == [1, 2, 3, 4, 5, 6])

    def test_int16(self):
        grps_msg = GrpsMessage()
        context = GrpsContext()
        gtensor = GenericTensor(dtype=DataType.DT_INT16, shape=[2, 3], flat_int16=[1, 2, 3, 4, 5, 6])
        grps_msg.gtensors.names.append('inp')
        grps_msg.gtensors.tensors.append(gtensor)
        tf_tensor_map = self.converter.preprocess(grps_msg, context)
        # print(tf_tensor_map)
        self.assertTrue('inp' in tf_tensor_map)
        self.assertTrue(tf_tensor_map['inp'].shape == (2, 3))
        self.assertTrue(tf_tensor_map['inp'].dtype == tf.int16)
        self.assertTrue(tf_tensor_map['inp'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])

        grps_msg = self.converter.postprocess(tf_tensor_map, context)
        # print(grps_msg)
        self.assertTrue(len(grps_msg.gtensors.names) == 1)
        self.assertTrue(len(grps_msg.gtensors.tensors) == 1)
        self.assertTrue(grps_msg.gtensors.names[0] == 'inp')
        self.assertTrue(grps_msg.gtensors.tensors[0].dtype == DataType.DT_INT16)
        self.assertTrue(grps_msg.gtensors.tensors[0].shape == [2, 3])
        self.assertTrue(grps_msg.gtensors.tensors[0].flat_int16 == [1, 2, 3, 4, 5, 6])

    def test_int32(self):
        grps_msg = GrpsMessage()
        context = GrpsContext()
        gtensor = GenericTensor(dtype=DataType.DT_INT32, shape=[2, 3], flat_int32=[1, 2, 3, 4, 5, 6])
        grps_msg.gtensors.names.append('inp')
        grps_msg.gtensors.tensors.append(gtensor)
        tf_tensor_map = self.converter.preprocess(grps_msg, context)
        # print(tf_tensor_map)
        self.assertTrue('inp' in tf_tensor_map)
        self.assertTrue(tf_tensor_map['inp'].shape == (2, 3))
        self.assertTrue(tf_tensor_map['inp'].dtype == tf.int32)
        self.assertTrue(tf_tensor_map['inp'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])

        grps_msg = self.converter.postprocess(tf_tensor_map, context)
        # print(grps_msg)
        self.assertTrue(len(grps_msg.gtensors.names) == 1)
        self.assertTrue(len(grps_msg.gtensors.tensors) == 1)
        self.assertTrue(grps_msg.gtensors.names[0] == 'inp')
        self.assertTrue(grps_msg.gtensors.tensors[0].dtype == DataType.DT_INT32)
        self.assertTrue(grps_msg.gtensors.tensors[0].shape == [2, 3])
        self.assertTrue(grps_msg.gtensors.tensors[0].flat_int32 == [1, 2, 3, 4, 5, 6])

    def test_int64(self):
        grps_msg = GrpsMessage()
        context = GrpsContext()
        gtensor = GenericTensor(dtype=DataType.DT_INT64, shape=[2, 3], flat_int64=[1, 2, 3, 4, 5, 6])
        grps_msg.gtensors.names.append('inp')
        grps_msg.gtensors.tensors.append(gtensor)
        tf_tensor_map = self.converter.preprocess(grps_msg, context)
        # print(tf_tensor_map)
        self.assertTrue('inp' in tf_tensor_map)
        self.assertTrue(tf_tensor_map['inp'].shape == (2, 3))
        self.assertTrue(tf_tensor_map['inp'].dtype == tf.int64)
        self.assertTrue(tf_tensor_map['inp'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])

        grps_msg = self.converter.postprocess(tf_tensor_map, context)
        # print(grps_msg)
        self.assertTrue(len(grps_msg.gtensors.names) == 1)
        self.assertTrue(len(grps_msg.gtensors.tensors) == 1)
        self.assertTrue(grps_msg.gtensors.names[0] == 'inp')
        self.assertTrue(grps_msg.gtensors.tensors[0].dtype == DataType.DT_INT64)
        self.assertTrue(grps_msg.gtensors.tensors[0].shape == [2, 3])
        self.assertTrue(grps_msg.gtensors.tensors[0].flat_int64 == [1, 2, 3, 4, 5, 6])

    def test_float16(self):
        grps_msg = GrpsMessage()
        context = GrpsContext()
        gtensor = GenericTensor(dtype=DataType.DT_FLOAT16, shape=[2, 3], flat_float16=[1, 2, 3, 4, 5, 6])
        grps_msg.gtensors.names.append('inp')
        grps_msg.gtensors.tensors.append(gtensor)
        tf_tensor_map = self.converter.preprocess(grps_msg, context)
        # print(tf_tensor_map)
        self.assertTrue('inp' in tf_tensor_map)
        self.assertTrue(tf_tensor_map['inp'].shape == (2, 3))
        self.assertTrue(tf_tensor_map['inp'].dtype == tf.float16)
        self.assertTrue(tf_tensor_map['inp'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])

        grps_msg = self.converter.postprocess(tf_tensor_map, context)
        # print(grps_msg)
        self.assertTrue(len(grps_msg.gtensors.names) == 1)
        self.assertTrue(len(grps_msg.gtensors.tensors) == 1)
        self.assertTrue(grps_msg.gtensors.names[0] == 'inp')
        self.assertTrue(grps_msg.gtensors.tensors[0].dtype == DataType.DT_FLOAT16)
        self.assertTrue(grps_msg.gtensors.tensors[0].shape == [2, 3])
        self.assertTrue(grps_msg.gtensors.tensors[0].flat_float16 == [1, 2, 3, 4, 5, 6])

    def test_float32(self):
        grps_msg = GrpsMessage()
        context = GrpsContext()
        gtensor = GenericTensor(dtype=DataType.DT_FLOAT32, shape=[2, 3], flat_float32=[1, 2, 3, 4, 5, 6])
        grps_msg.gtensors.names.append('inp')
        grps_msg.gtensors.tensors.append(gtensor)
        tf_tensor_map = self.converter.preprocess(grps_msg, context)
        # print(tf_tensor_map)
        self.assertTrue('inp' in tf_tensor_map)
        self.assertTrue(tf_tensor_map['inp'].shape == (2, 3))
        self.assertTrue(tf_tensor_map['inp'].dtype == tf.float32)
        self.assertTrue(tf_tensor_map['inp'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])

        grps_msg = self.converter.postprocess(tf_tensor_map, context)
        # print(grps_msg)
        self.assertTrue(len(grps_msg.gtensors.names) == 1)
        self.assertTrue(len(grps_msg.gtensors.tensors) == 1)
        self.assertTrue(grps_msg.gtensors.names[0] == 'inp')
        self.assertTrue(grps_msg.gtensors.tensors[0].dtype == DataType.DT_FLOAT32)
        self.assertTrue(grps_msg.gtensors.tensors[0].shape == [2, 3])
        self.assertTrue(grps_msg.gtensors.tensors[0].flat_float32 == [1, 2, 3, 4, 5, 6])

    def test_float64(self):
        grps_msg = GrpsMessage()
        context = GrpsContext()
        gtensor = GenericTensor(dtype=DataType.DT_FLOAT64, shape=[2, 3], flat_float64=[1, 2, 3, 4, 5, 6])
        grps_msg.gtensors.names.append('inp')
        grps_msg.gtensors.tensors.append(gtensor)
        tf_tensor_map = self.converter.preprocess(grps_msg, context)
        # print(tf_tensor_map)
        self.assertTrue('inp' in tf_tensor_map)
        self.assertTrue(tf_tensor_map['inp'].shape == (2, 3))
        self.assertTrue(tf_tensor_map['inp'].dtype == tf.float64)
        self.assertTrue(tf_tensor_map['inp'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])

        grps_msg = self.converter.postprocess(tf_tensor_map, context)
        # print(grps_msg)
        self.assertTrue(len(grps_msg.gtensors.names) == 1)
        self.assertTrue(len(grps_msg.gtensors.tensors) == 1)
        self.assertTrue(grps_msg.gtensors.names[0] == 'inp')
        self.assertTrue(grps_msg.gtensors.tensors[0].dtype == DataType.DT_FLOAT64)
        self.assertTrue(grps_msg.gtensors.tensors[0].shape == [2, 3])
        self.assertTrue(grps_msg.gtensors.tensors[0].flat_float64 == [1, 2, 3, 4, 5, 6])

    def test_multi_tensor(self):
        grps_msg = GrpsMessage()
        context = GrpsContext()
        gtensor1 = GenericTensor(dtype=DataType.DT_FLOAT64, shape=[2, 3], flat_float64=[1, 2, 3, 4, 5, 6])
        gtensor2 = GenericTensor(dtype=DataType.DT_UINT8, shape=[3, 2], flat_uint8=[1, 2, 3, 4, 5, 6])
        grps_msg.gtensors.names.append('inp1')
        grps_msg.gtensors.tensors.append(gtensor1)
        grps_msg.gtensors.names.append('inp2')
        grps_msg.gtensors.tensors.append(gtensor2)

        tf_tensor_map = self.converter.preprocess(grps_msg, context)
        # print(tf_tensor_map)
        self.assertTrue('inp1' in tf_tensor_map)
        self.assertTrue('inp2' in tf_tensor_map)
        self.assertTrue(tf_tensor_map['inp1'].shape == (2, 3))
        self.assertTrue(tf_tensor_map['inp1'].dtype == tf.float64)
        self.assertTrue(tf_tensor_map['inp1'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])
        self.assertTrue(tf_tensor_map['inp2'].shape == (3, 2))
        self.assertTrue(tf_tensor_map['inp2'].dtype == tf.uint8)
        self.assertTrue(tf_tensor_map['inp2'].numpy().flatten().tolist() == [1, 2, 3, 4, 5, 6])

        grps_msg = self.converter.postprocess(tf_tensor_map, context)
        # print(grps_msg)
        self.assertTrue(len(grps_msg.gtensors.names) == 2)
        self.assertTrue(len(grps_msg.gtensors.tensors) == 2)
        self.assertTrue(grps_msg.gtensors.names[0] == 'inp1')
        self.assertTrue(grps_msg.gtensors.tensors[0].dtype == DataType.DT_FLOAT64)
        self.assertTrue(grps_msg.gtensors.tensors[0].shape == [2, 3])
        self.assertTrue(grps_msg.gtensors.tensors[0].flat_float64 == [1, 2, 3, 4, 5, 6])
        self.assertTrue(grps_msg.gtensors.names[1] == 'inp2')
        self.assertTrue(grps_msg.gtensors.tensors[1].dtype == DataType.DT_UINT8)
        self.assertTrue(grps_msg.gtensors.tensors[1].shape == [3, 2])
        self.assertTrue(grps_msg.gtensors.tensors[1].flat_uint8 == [1, 2, 3, 4, 5, 6])


if __name__ == '__main__':
    unittest.main()
