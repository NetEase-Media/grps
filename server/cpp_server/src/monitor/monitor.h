/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/20
 * Brief  Monitor used to monitor the running metrics of the program. The monitor metrics will be show in
 * http://ip:port/grps/v1/monitor/metrics
 *
 * Usage:
 * // Monitor metrics with increase aggregation.
 * MONITOR_INC(name, value);
 * // Monitor metrics with max aggregation.
 * MONITOR_MAX(name, value);
 * // Monitor metrics with min aggregation.
 * MONITOR_MIN(name, value);
 * // Monitor metrics with avg aggregation.
 * MONITOR_AVG(name, value);
 * // Monitor metrics with cdf(continuous distribution function) aggregation.
 * MONITOR_CDF(name, value);
 */

#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Monitor metrics with increase aggregation.
#define MONITOR_INC(name, value) netease::grps::Monitor::Instance().Inc(name, value)
// Monitor metrics with max aggregation.
#define MONITOR_MAX(name, value) netease::grps::Monitor::Instance().Max(name, value)
// Monitor metrics with min aggregation.
#define MONITOR_MIN(name, value) netease::grps::Monitor::Instance().Min(name, value)
// Monitor metrics with avg aggregation.
#define MONITOR_AVG(name, value) netease::grps::Monitor::Instance().Avg(name, value)
// Monitor metrics with cdf(continuous distribution function) aggregation.
#define MONITOR_CDF(name, value) netease::grps::Monitor::Instance().Cdf(name, value)

// Time constants.
#define ONE_MIN (60)
#define ONE_HOUR (60 * ONE_MIN)
#define ONE_DAY (24 * ONE_HOUR)

namespace netease::grps {
const static int kMin = 60;
const static int kHour = 60 * kMin;
const static int kDay = 24 * kHour;

class MonitorException : public std::exception {
public:
  explicit MonitorException(std::string message) : message_(std::move(message)) {}
  ~MonitorException() override = default;
  [[nodiscard]] const char* what() const noexcept override {
    static std::string err_message;
    err_message = "[MonitorException] " + message_;
    return err_message.c_str();
  }

private:
  std::string message_;
};

// Aggregation type.
enum class AggType { kAvg = 0, kMax = 1, kMin = 2, kInc = 3, kCdf = 4 };

// Metrics aggregation.
class MetricsAgg {
public:
  friend class Monitor;
  MetricsAgg() = default;

  /**
   * Metrics aggregation constructor.
   * @param agg_name: The name of the aggregation.
   * @param agg_type: Aggregation type.
   */
  MetricsAgg(const std::string& agg_name, AggType agg_type);

  /**
   * Put data into aggregation.
   * @param now: The timestamp of the data.
   * @param data: The data.
   */
  void Put(int64_t now, float data);

  /**
   * Start aggregation thread.
   */
  void Start();

  /**
   * Stop aggregation thread.
   */
  void Stop();

  /**
   * aggregation datas with json format.
   * trend data format:
   * { "label": "trend", "data": [[0, 0], [1, 0], ... [173, 0]] }
   * cdf data format:
   * { "label": "cdf", "data": [[10, 0], [20, 0], ... , [90,0], [91,0], ... [100, 0], [101, 0]] }
   */
  std::string agg_datas_json();

  /* aggregation type */
  [[nodiscard]] AggType agg_type() const { return agg_type_; }

private:
  std::string agg_name_;
  AggType agg_type_ = AggType::kAvg;

  // TODO(zhaochaochao): Use lock free queue.
  std::list<std::pair<int64_t, float>> agg_buffer_; // [0]: timestamp, [1]: data.
  std::unique_ptr<std::mutex> agg_buffer_mutex_;

  // trend aggregation data.
  // [0:29]: represent last 30 days agg data.
  // [30:53]: represent last 24 hours agg data.
  // [54:113]: represent last 60 minutes agg data.
  // [114:173]: represent last 60 seconds agg data.
  std::vector<float> trend_agg_datas_;

  // cdf aggregation data.
  // [0:19]: represent 10%, 20%, ..., 90%, 91%, ... 100%(99.9%), 101%(99.99%).
  std::vector<float> cdf_agg_datas_;

  std::unique_ptr<std::mutex> agg_datas_mutex_;

  std::unique_ptr<std::thread> agg_thread_;
  static boost::asio::thread_pool pool;
  boost::asio::deadline_timer timer_ = boost::asio::deadline_timer(pool.get_executor(), boost::posix_time::seconds(1));
  int64_t iter_ = 0; // current iter count.
  bool running_ = false;

  // Aggregation thread function.
  void AggThreadFunc(const boost::system::error_code& ec);
  // avg func
  static auto AvgFunc(const std::vector<float>::iterator& begin, const std::vector<float>::iterator& end);
  // agg func
  auto AggFunc(const std::vector<float>::iterator& begin, const std::vector<float>::iterator& end);
};

// One piece of metrics.
struct MetricsPiece {
public:
  std::string name;
  int64_t now;
  float value;
  AggType agg_type;

  MetricsPiece(const std::string& name, int64_t now, float value, AggType agg_type)
      : name(name), now(now), value(value), agg_type(agg_type) {}

  [[nodiscard]] std::string ToString() const {
    std::stringstream ss;
    ss << "{name: " << name << ", now: " << now << ", value: " << value << ", agg_type: " << int(agg_type) << "}";
    return ss.str();
  }
};

class Monitor {
public:
  ~Monitor() { Stop(); }
  Monitor(const Monitor&) = delete;
  Monitor& operator=(const Monitor&) = delete;
  Monitor(Monitor&&) = delete;
  Monitor& operator=(Monitor&&) = delete;

  static Monitor& Instance() {
    static Monitor monitor;
    return monitor;
  }

  /* Init monitor. */
  void Init();

  /* Start monitor. */
  void Start();

  /* Stop monitor. */
  void Stop();

  /**
   * Add avg metrics piece. Monitor will save the data and aggregate it by avg function in agg_time_unit(1s).
   * @param key: The name of the metrics.
   * @param value: The value of the metrics.
   */
  void Avg(const std::string& key, float value) {
    PutMetricPiece(MetricsPiece(key, time(nullptr), value, AggType::kAvg));
  }

  /**
   * Add max metrics piece. Monitor will save the data and aggregate it by max function in agg_time_unit(1s).
   * @param key: The name of the metrics.
   * @param value: The value of the metrics.
   */
  void Max(const std::string& key, float value) {
    PutMetricPiece(MetricsPiece(key, time(nullptr), value, AggType::kMax));
  }

  /**
   * Add min metrics piece. Monitor will save the data and aggregate it by min function in agg_time_unit(1s).
   * @param key: The name of the metrics.
   * @param value: The value of the metrics.
   */
  void Min(const std::string& key, float value) {
    PutMetricPiece(MetricsPiece(key, time(nullptr), value, AggType::kMin));
  }

  /**
   * Add inc(increase) metrics piece. Monitor will save the data and aggregate it by increase function in
   * agg_time_unit(1s).
   * @param key: The name of the metrics.
   * @param value: The value of the metrics.
   */
  void Inc(const std::string& key, float value) {
    PutMetricPiece(MetricsPiece(key, time(nullptr), value, AggType::kInc));
  }

  /**
   * Add cdf(continuous distribution function) metrics piece. Monitor will save the data and aggregate it by cdf
   * function in agg_time_unit(1s).
   * @param key: The name of the metrics.
   * @param value: The value of the metrics.
   */
  void Cdf(const std::string& key, float value) {
    PutMetricPiece(MetricsPiece(key, time(nullptr), value, AggType::kCdf));
  }

  /**
   * Aggregation datas with json format.
   * trend data format:
   * { "label": "trend", "data": [[0, 0], [1, 0], ... [173, 0]] }
   * cdf data format:
   * { "label": "cdf", "data": [[10, 0], [20, 0], ... [90,0], [91,0], ... [100, 0], [101, 0]] }
   */
  std::string GetMetricsAggDataJson(const char* name);

  /**
   * Get metrics names.
   */
  std::vector<std::string> GetMetricsNames();

private:
  std::unordered_map<std::string, MetricsAgg> metrics_agg_; // TODO(zhaochaochao): Use lock free map.
  boost::shared_mutex metrics_agg_mutex_;
  std::atomic<bool> running_;
  std::thread dump_metrics_agg_thread_;
  std::string monitor_log_path_;

  Monitor() : metrics_agg_(), running_(false) {}

  // Put metrics piece into metrics queue.
  void PutMetricPiece(const MetricsPiece& metrics_piece);
  // Dump metrics aggregation data into file.
  void DumpMetricsAgg();
};
} // namespace netease::grps
