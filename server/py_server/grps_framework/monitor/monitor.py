# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2022/9/22
# Brief  Monitor used to monitor the running metrics of the program. The monitor metrics will be show in
# http://ip:port/grps/v1/monitor/metrics
#
# Usage:
# from grps_framework.monitor.monitor import app_monitor
# app_monitor.max('metrics_name', value)  # Monitor metrics with max aggregation.
# app_monitor.min('metrics_name', value)  # Monitor metrics with min aggregation.
# app_monitor.avg('metrics_name', value)  # Monitor metrics with avg aggregation.
# app_monitor.inc('metrics_name', value)  # Monitor metrics with inc aggregation.
# app_monitor.cdf('metrics_name', value)  # Monitor metrics with cdf aggregation.
import os
import sys
import threading
import time
from queue import Empty, Full, Queue

from apscheduler.schedulers.background import BackgroundScheduler

from grps_framework.logger.logger import logger
from grps_framework.conf.conf import global_conf


class AggType(object):
    """Aggregation type."""
    AVG = 0
    MAX = 1
    MIN = 2
    INC = 3
    CDF = 4


class AggTimeUnit(object):
    """Aggregation time unit."""
    SECOND = 0
    MINUTE = 1
    HOUR = 2
    DAY = 3


class MetricsHistory(object):
    """
    AggType:
        For CDF, process only second_data and store detailed data.
        For AVG, store second-level data in the format of {sum, count}.
        For other types, store data in the format of value.
    When aggregating fine-grained data into coarse-grained data, use the average value. For example, when aggregating
    second-level data into minute-level data, use the average value of the second-level data.
    """

    def __init__(self, time_unit=AggTimeUnit.SECOND, agg_type=AggType.AVG):
        self.__time_unit = time_unit

        self.last_ns = 1
        self.num = 61 + self.last_ns  # store last n seconds data and current data.
        self.default_value = 0
        self.agg_type = agg_type

        if time_unit == AggTimeUnit.MINUTE:
            self.num = 60
        elif time_unit == AggTimeUnit.HOUR:
            self.num = 24
        elif time_unit == AggTimeUnit.DAY:
            self.num = 30
        if agg_type == AggType.CDF:
            self.num = self.last_ns + 1  # store last_ns data and current data.
            self.default_value = []
        elif agg_type == AggType.AVG and time_unit == AggTimeUnit.SECOND:
            self.default_value = [0, 0]
        elif agg_type == AggType.AVG:
            self.default_value = 0.0
        elif agg_type == AggType.MIN:
            self.default_value = sys.maxsize
        if isinstance(self.default_value, list):
            self.data = [self.default_value.copy() for i in range(0, self.num)]
        else:
            self.data = [self.default_value] * self.num

    def update(self):
        """
        Rolling the entire data, discard the data at position 0, and set the last data to the default value.
        """
        if isinstance(self.default_value, list):
            if self.agg_type == AggType.CDF:
                self.data = self.data[1:] + [[]]
            elif self.agg_type == AggType.AVG and self.__time_unit == AggTimeUnit.SECOND:
                self.data = self.data[1:] + [[0, 0]]
        else:
            self.data = self.data[1:] + [self.default_value]

    def put(self, data, agg_type):
        """
        Adding monitoring data, theoretically only operate on second-level data.
        """
        if agg_type == AggType.AVG:
            if isinstance(data, list):
                value = data[0]
                count = data[1]
            else:
                value = data
                count = 1
            self.data[-1][0] += value
            self.data[-1][1] += count
        elif agg_type == AggType.CDF:
            self.data[-1].append(data)
        elif agg_type == AggType.MAX:
            self.data[-1] = max(self.data[-1], data)
        elif agg_type == AggType.MIN:
            self.data[-1] = min(self.data[-1], data)
        elif agg_type == AggType.INC:
            self.data[-1] = self.data[-1] + data

    def merge(self, data):
        """
        Aggregating fine-grained data into coarse-grained data.
        """
        if data is None:
            return
        if len(data) == 0:
            return

        if isinstance(data[0], list):
            sum_result = 0
            for d in data:
                if d[1] != 0:
                    sum_result += d[0] / d[1]
            avg = sum_result / len(data)
        else:
            sum_result = 0
            for d in data:
                if d != self.default_value:
                    sum_result += d

            avg = sum_result / len(data)

        self.data[-1] = avg

    def read(self, agg_type):
        """
        Reading monitoring data.
        """
        last_ns = self.last_ns
        if self.__time_unit == AggTimeUnit.SECOND:
            result_data = self.data[:-1].copy()
        else:
            result_data = self.data.copy()
        if agg_type == AggType.AVG and self.__time_unit == AggTimeUnit.SECOND:
            avg_result = []
            for i in range(last_ns, len(result_data)):
                last_ns_data = result_data[i - last_ns:i]  # get last_ns data
                sum_result = 0
                sum_count = 0
                for d in last_ns_data:
                    sum_result += d[0]
                    sum_count += d[1]
                if sum_count == 0:
                    avg_result.append(0.0)
                else:
                    avg_result.append(sum_result / sum_count)
            return avg_result
        elif agg_type == AggType.CDF:
            return self.calc_cdf(self.data[:-1].copy())
        elif agg_type == AggType.MIN:
            if self.__time_unit == AggTimeUnit.SECOND:
                min_result = []
                for i in range(last_ns, len(result_data)):
                    min_result.append(min(result_data[i - last_ns:i]))  # get min data in last_ns
                return min_result
            else:
                return [0 if x == self.default_value else x for x in result_data]
        elif agg_type == AggType.MAX and self.__time_unit == AggTimeUnit.SECOND:
            max_result = []
            for i in range(last_ns, len(result_data)):
                max_result.append(max(result_data[i - last_ns:i]))
            return max_result
        elif agg_type == AggType.INC and self.__time_unit == AggTimeUnit.SECOND:
            inc_result = []
            for i in range(last_ns, len(result_data)):
                inc_result.append(sum(result_data[i - last_ns:i]) / last_ns)
            return inc_result
        else:
            return result_data

    def calc_cdf(self, data):
        """
        Calculating CDF (cumulative distribution function) data, called only during data reading.
        """
        percent_index = [10, 20, 30, 40, 50, 60, 70, 80, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101]
        last_ns_data = [elem for sublist in data for elem in sublist]
        if len(last_ns_data) == 0:
            return [[percent, 0] for percent in percent_index]
        sorted_data = sorted(last_ns_data)
        length = len(sorted_data)
        result = []
        for percent in percent_index:
            if percent == 100:
                index = int(length * 99.9 / 100) - 1
            elif percent == 101:
                index = int(length * 99.99 / 100) - 1
            else:
                index = int(length * percent / 100) - 1
            index = max(0, index)
            index = min(index, length - 1)
            result.append([percent, sorted_data[index]])
        return result


class MetricsCompose(object):
    """
    Storing historical monitoring data:
        second_data: used to store second-level data, updated every second
        minute_data: used to store minute-level data, updated every minute
        hour_data: used to store hour-level data, updated every hour
        day_data: used to store day-level data, updated every day
    """

    def __init__(self, agg_type, scheduler):
        self.day_data = MetricsHistory(AggTimeUnit.DAY, agg_type)
        self.hour_data = MetricsHistory(AggTimeUnit.HOUR, agg_type)
        self.minute_data = MetricsHistory(AggTimeUnit.MINUTE, agg_type)
        self.second_data = MetricsHistory(AggTimeUnit.SECOND, agg_type)

        self.day_update_cnt = 24 * 60 * 60
        self.hour_update_cnt = 60 * 60
        self.minute_update_cnt = 60
        self.update_cnt = 0
        self.agg_type = agg_type
        self.scheduler = scheduler
        self.scheduler.add_job(self.update, 'interval', seconds=1)
        if not self.scheduler.running:
            self.scheduler.start()
        self._lock = threading.RLock()

    # Calling every second.
    def update(self):
        self.update_cnt = self.update_cnt + 1
        # CDF only operate on second-level data.
        if self.agg_type == AggType.CDF:
            with self._lock:
                self.second_data.update()
            return

        # Data rolling, aggregating fine-grained data to coarse-grained data.
        # Only second-level data is concurrently operated, so only lock the second-level data.
        with self._lock:
            self.second_data.update()

        if self.update_cnt % self.minute_update_cnt == 0:
            self.minute_data.update()
            self.minute_data.merge(self.second_data.data[self.second_data.last_ns:-1])

        if self.update_cnt % self.hour_update_cnt == 0:
            self.hour_data.update()
            self.hour_data.merge(self.minute_data.data)

        if self.update_cnt == self.day_update_cnt:
            self.day_data.update()
            self.day_data.merge(self.hour_data.data)
            self.update_cnt = 0

    # The 'put' function is called by a single thread, so the lock won't have a significant impact on performance.
    # The main purpose is to prevent concurrency issues with the 'update' function.
    def put(self, data):
        with self._lock:
            self.second_data.put(data, self.agg_type)

    def read(self):
        if self.agg_type == AggType.CDF:
            return {
                'label': 'cdf',
                'data': self.second_data.read(self.agg_type)
            }
        res = []
        res.extend(self.day_data.read(self.agg_type))
        res.extend(self.hour_data.read(self.agg_type))
        res.extend(self.minute_data.read(self.agg_type))
        res.extend(self.second_data.read(self.agg_type))
        return {
            'label': 'trend',
            'data': res
        }

    def last_second_read(self):
        if self.agg_type == AggType.CDF:
            return self.second_data.read(self.agg_type)
        else:
            return self.second_data.read(self.agg_type)[-1]


class MetricsPiece(object):
    """One piece of metrics."""

    def __init__(self, name, now, value, agg_type):
        self.name = name
        self.now = now
        self.value = value
        self.agg_type = agg_type


class Monitor(object):

    def __init__(self, scheduler):
        """
        Init monitor.
        """
        self.__metrics_composes = {}

        self.__metrics_pieces_queue = Queue(maxsize=1000)
        self.__agg_thread = threading.Thread(target=self.__agg_thread_func)
        self.__dump_thread = threading.Thread(target=self.__dump_metrics)
        self.__scheduler = scheduler
        self.__running = False

    def __agg_thread_func(self):
        """Aggregation thread function."""
        while self.__running:
            try:
                metrics_piece = self.__metrics_pieces_queue.get(block=True, timeout=1)
                if metrics_piece.name not in self.__metrics_composes:
                    self.__metrics_composes[metrics_piece.name] = MetricsCompose(metrics_piece.agg_type,
                                                                                 self.__scheduler)

                if self.__metrics_composes[metrics_piece.name].agg_type != metrics_piece.agg_type:
                    logger.error('agg_type not match, new agg_type: {}, old agg_type: {}.'.format(
                        metrics_piece.agg_type, self.__metrics_composes[metrics_piece.name].agg_type))
                    continue
                self.__metrics_composes[metrics_piece.name].put(metrics_piece.value)
            except Empty:
                continue

    def start(self):
        """Start monitor."""
        self.__running = True
        self.__agg_thread.start()
        self.__dump_thread.start()

    def stop(self):
        """Stop monitor."""
        self.__running = False

        self.__agg_thread.join()
        self.__dump_thread.join()

        self.__scheduler.shutdown()

    def __put_metric_piece(self, metrics_piece):
        """Put metrics piece into metrics queue."""
        if not self.__running:
            return
        try:
            if metrics_piece.name is None or metrics_piece.value is None:
                logger.error('metrics name or value is None.')
                raise ValueError('metrics name or value is None.')
            if not isinstance(metrics_piece.value, (int, float)):
                logger.error('metrics value is not int or float.')
                raise ValueError('metrics value is not int or float.')
            self.__metrics_pieces_queue.put(metrics_piece, block=False)
        except Full:
            logger.error('Metrics queue is full.')

    def avg(self, key, value):
        """
        Add avg metrics piece. Will not block user thread.
        Args:
            key: Metrics key.
            value: Metrics value.
        """
        self.__put_metric_piece(MetricsPiece(key, time.time(), value, AggType.AVG))

    def max(self, key, value):
        """
        Add max metrics piece. Will not block user thread.
        Args:
            key: Metrics key.
            value: Metrics value.
        """
        self.__put_metric_piece(MetricsPiece(key, time.time(), value, AggType.MAX))

    def min(self, key, value):
        """
        Add min metrics piece. Will not block user thread.
        Args:
            key: Metrics key.
            value: Metrics value.
        """
        self.__put_metric_piece(MetricsPiece(key, time.time(), value, AggType.MIN))

    def inc(self, key, value):
        """
        Add inc metrics piece. Will not block user thread.
        Args:
            key: Metrics key.
            value: Metrics value.
        """
        self.__put_metric_piece(MetricsPiece(key, time.time(), value, AggType.INC))

    def cdf(self, key, value):
        """
        Add inc metrics piece. Will not block user thread.
        Args:
            key: Metrics key.
            value: Metrics value.
        """
        self.__put_metric_piece(MetricsPiece(key, time.time(), value, AggType.CDF))

    def read(self, key):
        if key in self.__metrics_composes:
            return self.__metrics_composes[key].read()
        return 'key not found'

    def read_all(self):
        res = {}
        for key in self.__metrics_composes:
            res[key] = self.__metrics_composes[key].read()
        return res

    def __dump_metrics(self):
        """Dump metrics to file and update every second."""
        monitor_log_path = os.path.join(global_conf.server_conf['log']['log_dir'], 'grps_monitor.log')
        with open(monitor_log_path, 'w+') as f:
            while self.__running:
                # clear file content before write
                f.seek(0)
                f.truncate()
                for key in self.__metrics_composes:
                    if self.__metrics_composes[key].agg_type == AggType.CDF:
                        # Dump percentile 80,90,99,99.9,99.99 data. The index of the percentile data is 7,8,17,18,19.
                        f.write('{}_80 : '.format(key))
                        f.write('{:.2f}\n'.format(self.__metrics_composes[key].last_second_read()[7][1]))
                        f.write('{}_90 : '.format(key))
                        f.write('{:.2f}\n'.format(self.__metrics_composes[key].last_second_read()[8][1]))
                        f.write('{}_99 : '.format(key))
                        f.write('{:.2f}\n'.format(self.__metrics_composes[key].last_second_read()[17][1]))
                        f.write('{}_999 : '.format(key))
                        f.write('{:.2f}\n'.format(self.__metrics_composes[key].last_second_read()[18][1]))
                        f.write('{}_9999 : '.format(key))
                        f.write('{:.2f}\n'.format(self.__metrics_composes[key].last_second_read()[19][1]))
                    else:
                        f.write('{} : '.format(key))
                        f.write('{:.2f}\n'.format(self.__metrics_composes[key].last_second_read()))
                f.flush()
                time.sleep(1)
        f.close()


executors = {
    'default': {'type': 'threadpool', 'max_workers': 1},
    'processpool': {'type': 'processpool', 'max_workers': 1}
}
scheduler = BackgroundScheduler()
scheduler.configure(executors=executors)
app_monitor = Monitor(scheduler)
