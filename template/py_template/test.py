# Local unittest.

import unittest

from grps_framework.apis.grps_pb2 import GrpsMessage
from grps_framework.context.context import GrpsContext
from grps_framework.test import GrpsTest
# import to register customized converter and inferer.
from src.customized_converter import converter_register
from src.customized_inferer import inferer_register


class MyTestCase(GrpsTest):
    def test_infer(self):
        self.assertGreater(len(converter_register.converter_dict), 0)
        self.assertGreater(len(inferer_register.model_inferer_dict), 0)

        self.test_init()

        grps_in = GrpsMessage()
        # Add your codes to set input as follows:
        # grps_in.str_data = 'hello grps'
        # grps_in.bin_data = b'hello grps'
        # gtensor = GenericTensor(name='inp', dtype=DataType.DT_FLOAT32, shape=[1, 2], flat_float32=[1, 2])
        # grps_in.gtensors.tensors.append(gtensor)

        # Infer.
        context = GrpsContext()
        grps_out = self.executor.infer(grps_in, context)

        self.assertEqual(context.has_err(), False)

        # Check your result as follows:
        # self.assertEqual(grps_out.str_data, 'hello grps')
        # self.assertEqual(grps_out.bin_data, b'hello grps')
        # gtensor = GenericTensor(name='inp', dtype=DataType.DT_FLOAT32, shape=[1, 2], flat_float32=[1, 2])
        # self.assertEqual(grps_out.gtensors.tensors[0], gtensor)


if __name__ == '__main__':
    suite = unittest.TestSuite()
    suite.addTest(MyTestCase('test_infer'))
    runner = unittest.TextTestRunner()
    runner.run(suite)
