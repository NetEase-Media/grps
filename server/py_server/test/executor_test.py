import os
import sys
import unittest

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from grps_framework.executor.executor import Executor
from grps_framework.apis.grps_pb2 import GrpsMessage
from grps_framework.context.context import GrpsContext

import src.customized_converter
import src.customized_inferer


class MyTestCase(unittest.TestCase):
    def test_executor(self):
        executor = Executor()
        inp = GrpsMessage()
        inp.str_data = 'hello grps'
        print('input: ', inp)

        output = executor.infer(inp, GrpsContext())
        print('output: ', output)
        self.assertEqual(inp, output)


if __name__ == '__main__':
    unittest.main()
