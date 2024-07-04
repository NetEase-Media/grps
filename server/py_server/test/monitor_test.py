import os.path
import sys
import threading
import time
import unittest

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from grps_framework.monitor.monitor import app_monitor


class MyTestCase(unittest.TestCase):
    def add_monitor(self):
        for i in range(120):
            app_monitor.avg('avg_test', 1)
            app_monitor.min('min_test', 1)
            app_monitor.max('max_test', 1)
            app_monitor.inc('inc_test', 1)
            app_monitor.cdf('cdf_test', i)
            time.sleep(1)

    def test_something(self):
        app_monitor.start()
        parallel_num = 4

        thread_list = []
        for i in range(0, parallel_num):
            thread_list.append(threading.Thread(target=self.add_monitor))

        for thread in thread_list:
            thread.start()

        for thread in thread_list:
            thread.join()

        all = app_monitor.read_all()
        avg_test = all['avg_test']['data']
        min_test = all['min_test']['data']
        max_test = all['max_test']['data']
        inc_test = all['inc_test']['data']
        cdf_test = all['cdf_test']
        # 受稳定性影响，应该大概率是这个结果
        expect_avg_test = [0.0] * 112 + [1.0] * (173 - 111)
        expect_inc_test = [0] * 112 + [4.0] * 2 + [4] * (173 - 113)
        expect_min_test = [0] * 112 + [1.0] * 2 + [1] * (173 - 113)
        expect_max_test = [0] * 112 + [1.0] * 2 + [1] * (173 - 113)
        expect_cdf_test = {
            'label': 'cdf',
            'data': [[i * 10, 119] for i in range(1, 10)] + [[i, 119] for i in range(91, 102)]
        }
        self.assertTrue(avg_test == expect_avg_test)
        self.assertTrue(min_test == expect_min_test)
        self.assertTrue(max_test == expect_max_test)
        self.assertTrue(inc_test == expect_inc_test)
        self.assertTrue(cdf_test == expect_cdf_test)
        app_monitor.stop()


if __name__ == '__main__':
    unittest.main()
