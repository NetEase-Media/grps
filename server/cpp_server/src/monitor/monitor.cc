/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/20
 * Brief  Monitor used to monitor the running metrics of the program.
 */

#include "monitor.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cmath>
#include <fstream>
#include <functional>
#include <numeric>

#include "config/global_config.h"
#include "logger/logger.h"

#define MONITOR_DEBUG 0

namespace netease::grps {
boost::asio::thread_pool MetricsAgg::pool(1);
MetricsAgg::MetricsAgg(const std::string& agg_name, AggType agg_type)
    : agg_name_(agg_name)
    , agg_type_(agg_type)
    , agg_buffer_()
    , trend_agg_datas_(174, 0.0)
    , cdf_agg_datas_(20, 0)
    , running_(false) {
  agg_buffer_mutex_ = std::make_unique<std::mutex>();
  agg_datas_mutex_ = std::make_unique<std::mutex>();
}

void MetricsAgg::Put(int64_t now, float data) {
  std::lock_guard<std::mutex> lock(*agg_buffer_mutex_);
  agg_buffer_.emplace_back(now, data);
}

void MetricsAgg::Start() {
  running_ = true;
  timer_.async_wait([this](auto&& param) { AggThreadFunc(std::forward<decltype(param)>(param)); });
}

void MetricsAgg::Stop() {
  running_ = false;
}

std::string MetricsAgg::agg_datas_json() {
  std::vector<float> agg_datas;
  {
    std::lock_guard<std::mutex> lock(*agg_datas_mutex_);
    if (agg_type_ == AggType::kCdf) {
      agg_datas = cdf_agg_datas_;
    } else if (agg_type_ == AggType::kAvg || agg_type_ == AggType::kMax || agg_type_ == AggType::kMin ||
               agg_type_ == AggType::kInc) {
      agg_datas = trend_agg_datas_;
    } else {
      LOG4(ERROR, "AggType not support yet, agg_type: " << int(agg_type_));
      return "";
    }
  }

  // Build json.
  if (agg_type_ == AggType::kCdf) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    doc.AddMember("label", "cdf", allocator);
    rapidjson::Value data(rapidjson::kArrayType);
    for (int i = 0; i < agg_datas.size(); ++i) {
      rapidjson::Value data_item(rapidjson::kArrayType);
      if (i < 9)
        data_item.PushBack((i + 1) * 10, allocator);
      else
        data_item.PushBack(90 + (i - 8), allocator);
      data_item.PushBack(agg_datas[i], allocator);
      data.PushBack(data_item, allocator);
    }
    doc.AddMember("data", data, allocator);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.SetMaxDecimalPlaces(2);
    doc.Accept(writer);
    return buffer.GetString();
  } else if (agg_type_ == AggType::kAvg || agg_type_ == AggType::kMax || agg_type_ == AggType::kMin ||
             agg_type_ == AggType::kInc) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    doc.AddMember("label", "trend", allocator);
    rapidjson::Value data(rapidjson::kArrayType);
    for (int i = 0; i < agg_datas.size(); ++i) {
      rapidjson::Value data_item(rapidjson::kArrayType);
      data_item.PushBack(i, allocator);
      data_item.PushBack(agg_datas[i], allocator);
      data.PushBack(data_item, allocator);
    }
    doc.AddMember("data", data, allocator);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.SetMaxDecimalPlaces(2);
    doc.Accept(writer);
    return buffer.GetString();
  } else {
    LOG4(ERROR, "AggType not support yet, agg_type: " << int(agg_type_));
    return "";
  }
}

auto MetricsAgg::AvgFunc(const std::vector<float>::iterator& begin, const std::vector<float>::iterator& end) {
  if (end - begin == 0) {
    return float(0.0);
  }
  auto sum = std::accumulate(begin, end, 0.0);
  return float(sum) / float(end - begin);
}

auto MetricsAgg::AggFunc(const std::vector<float>::iterator& begin, const std::vector<float>::iterator& end) {
  if (agg_type_ == AggType::kAvg) {
    return MetricsAgg::AvgFunc(begin, end);
  } else if (agg_type_ == AggType::kMax) {
    if (end - begin == 0) {
      return float(0.0);
    }
    return *std::max_element(begin, end);
  } else if (agg_type_ == AggType::kMin) {
    if (end - begin == 0) {
      return float(0.0);
    }
    return *std::min_element(begin, end);
  } else if (agg_type_ == AggType::kInc) {
    if (end - begin == 0) {
      return float(0.0);
    }
    auto sum = std::accumulate(begin, end, 0.0);
    return float(sum);
  }
  return float(0.0);
}

void MetricsAgg::AggThreadFunc(const boost::system::error_code& ec) {
  if (!running_) return;
  iter_++;
  int last_ns = 1;
  auto cur_time = std::chrono::system_clock::now();
  auto cur_time_s = std::chrono::duration_cast<std::chrono::seconds>(cur_time.time_since_epoch()).count();

  // Get last ns data from agg_buffer_.
  std::vector<float> last_ns_data;
  {
    std::lock_guard<std::mutex> lock(*agg_buffer_mutex_);
    while (!agg_buffer_.empty()) {
      auto data = agg_buffer_.front();
      if (data.first < cur_time_s - last_ns) {
        agg_buffer_.pop_front();
        continue;
      }
      break;
    }
    for (auto& iterator : agg_buffer_) {
      if (iterator.first >= cur_time_s) {
        break;
      }
      last_ns_data.push_back(iterator.second);
    }
  }
  if (agg_type_ == AggType::kAvg || agg_type_ == AggType::kInc || agg_type_ == AggType::kMax ||
      agg_type_ == AggType::kMin) { // trend aggregate.
    // Trend aggregate data into trend_agg_datas according to the following rules:
    // 1. Aggregate second data once per second, and shift the second array in trend_agg_datas to the left once.
    // 2. Aggregate minute data once per minute, and shift the minute array in trend_agg_da to the left once.
    // 3. Aggregate hourly data once per hour, and shift the hour array in trend_agg_datas to the left once.
    // 4. Aggregate daily data once per day, and shift the day array in trend_agg_datas to the left once.

    // Aggregate last ns data, shift the second array in trend_agg_datas to the left once.
    std::lock_guard<std::mutex> lock(*agg_datas_mutex_);
    float last_ns_data_agg = AggFunc(last_ns_data.begin(), last_ns_data.end());
    if (agg_type_ == AggType::kInc) last_ns_data_agg /= float(last_ns);
    // LOG4(INFO, "curr_s: " << cur_time_s << "agg_tye: " << int(agg_type_) << " data: " << last_ns_data_agg);
    for (int i = 114; i < 173; ++i) {
      trend_agg_datas_[i] = trend_agg_datas_[i + 1];
    }
    trend_agg_datas_[173] = last_ns_data_agg;

    // If reach 1m, aggregate 1m data, shift the minute array in trend_agg_datas to the left once.
    if (iter_ % ONE_MIN == 0) {
      float last_1m_data_agg = AvgFunc(trend_agg_datas_.begin() + 114, trend_agg_datas_.begin() + 174);
      for (int i = 54; i < 113; ++i) {
        trend_agg_datas_[i] = trend_agg_datas_[i + 1];
      }
      trend_agg_datas_[113] = last_1m_data_agg;
    }

    // If reach 1h, aggregate 1h data, shift the hour array in trend_agg_datas to the left once.
    if (iter_ % ONE_HOUR == 0) {
      float last_1h_data_agg = AvgFunc(trend_agg_datas_.begin() + 54, trend_agg_datas_.begin() + 114);
      for (int i = 30; i < 53; ++i) {
        trend_agg_datas_[i] = trend_agg_datas_[i + 1];
      }
      trend_agg_datas_[53] = last_1h_data_agg;
    }

    // If reach 1d, aggregate 1d data, shift the day array in trend_agg_datas to the left once.
    if (iter_ % ONE_DAY == 0) {
      float last_1d_data_agg = AvgFunc(trend_agg_datas_.begin() + 30, trend_agg_datas_.begin() + 54);
      for (int i = 0; i < 29; ++i) {
        trend_agg_datas_[i] = trend_agg_datas_[i + 1];
      }
      trend_agg_datas_[29] = last_1d_data_agg;
      iter_ = 0; // reset iter.
    }

#if MONITOR_DEBUG
    std::stringstream ss;
    ss << "AggThreadFunc: agg_name: " << agg_name_ << ", agg_type: " << int(agg_type_) << ", cur_time_s " << cur_time_s
       << ", iter_: " << iter_ << ", trend_agg_datas: [";
    for (float trend_agg_data : trend_agg_datas_) {
      ss << trend_agg_data << ", ";
    }
    ss << "]";
    LOG4(INFO, ss.str());
#endif
  } else if (agg_type_ == AggType::kCdf) { // cdf aggregate.
    if (!last_ns_data.empty()) {
      auto begin = last_ns_data.begin();
      auto end = last_ns_data.end();
      std::sort(begin, end);

      // compute percentile: 10, 20,, ..., 90, 91, 92, ..., 100, 101
      std::vector<float> percentiles;
      for (int i = 1; i <= 9; ++i) {
        percentiles.push_back(float(i * 10));
      }
      for (int i = 1; i <= 9; ++i) {
        percentiles.push_back(float(90 + i));
      }
      percentiles.push_back(float(99.9));
      percentiles.push_back(float(99.99));

      std::lock_guard<std::mutex> lock(*agg_datas_mutex_);
      for (int i = 0; i < percentiles.size(); ++i) {
        int index = int(ceil(percentiles[i] / 100.0 * float(last_ns_data.size()))) - 1;
        cdf_agg_datas_[i] = (*(begin + index));
      }
    } else {
      // reset to 0
      std::lock_guard<std::mutex> lock(*agg_datas_mutex_);
      for (float& cdf_agg_data : cdf_agg_datas_) {
        cdf_agg_data = 0;
      }
    }

#if MONITOR_DEBUG
    std::stringstream ss;
    ss << "AggThreadFunc: agg_name: " << agg_name_ << ", agg_type: " << int(agg_type_) << ", cur_time_s " << cur_time_s
       << ", iter_: " << iter_ << ", cdf_agg_datas: [";
    for (float cdf_agg_data : cdf_agg_datas_) {
      ss << cdf_agg_data << ", ";
    }
    ss << "]";
    LOG4(INFO, ss.str());
#endif
  } else {
    LOG4(ERROR, "AggType not support yet, agg_type: " << int(agg_type_));
    return;
  }
  timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(1));
  timer_.async_wait([this](auto&& param) { AggThreadFunc(std::forward<decltype(param)>(param)); });
}

void Monitor::Init() {
  monitor_log_path_ = GlobalConfig::Instance().server_config().log.log_dir + "/grps_monitor.log";
  if (GlobalConfig::Instance().mpi().world_rank > 0) {
    monitor_log_path_ += "-rank" + std::to_string(GlobalConfig::Instance().mpi().world_rank);
  }
}

void Monitor::PutMetricPiece(const netease::grps::MetricsPiece& metrics_piece) {
  if (!running_) {
    LOG4(WARN, "Monitor is not running.");
    return;
  }

#if MONITOR_DEBUG
  LOG4(INFO, "PutMetricPiece, new metrics piece: " << metrics_piece.ToString());
#endif

  {
    boost::shared_lock<boost::shared_mutex> agg_rlock(metrics_agg_mutex_); // use read lock check firstly.
    if (metrics_agg_.find(metrics_piece.name) == metrics_agg_.end()) {
      // use write lock to update.
      agg_rlock.unlock();
      boost::lock_guard<boost::shared_mutex> agg_wlock(metrics_agg_mutex_);
      if (metrics_agg_.find(metrics_piece.name) == metrics_agg_.end()) {
        metrics_agg_.emplace(metrics_piece.name, MetricsAgg(metrics_piece.name, metrics_piece.agg_type));
        metrics_agg_[metrics_piece.name].Start();
      }
    }
  }

  if (metrics_agg_[metrics_piece.name].agg_type() != metrics_piece.agg_type) {
    LOG4(ERROR, "PutMetricPiece, agg_type not match, new agg_type: "
                  << int(metrics_piece.agg_type)
                  << ", old agg_type: " << int(metrics_agg_[metrics_piece.name].agg_type()));
    return;
  }

  metrics_agg_[metrics_piece.name].Put(metrics_piece.now, metrics_piece.value);
}

void Monitor::Start() {
  running_ = true;
  dump_metrics_agg_thread_ = std::thread(&Monitor::DumpMetricsAgg, this);
}

void Monitor::Stop() {
  running_ = false;
  if (dump_metrics_agg_thread_.joinable()) {
    dump_metrics_agg_thread_.join();
  }
  for (auto& [name, metrics_agg] : metrics_agg_) {
    metrics_agg.Stop();
  }
  MetricsAgg::pool.join();
}

std::string Monitor::GetMetricsAggDataJson(const char* name) {
  boost::shared_lock_guard<boost::shared_mutex> lock(metrics_agg_mutex_);
  if (metrics_agg_.find(name) == metrics_agg_.end()) {
    // LOG4(WARN, "GetMetricsAggData, metrics name not found, name: " << name);
    return "";
  }
  return metrics_agg_[name].agg_datas_json();
}

std::vector<std::string> Monitor::GetMetricsNames() {
  boost::shared_lock_guard<boost::shared_mutex> lock(metrics_agg_mutex_);
  std::vector<std::string> names;
  names.reserve(metrics_agg_.size());
  for (auto& [name, metrics_agg] : metrics_agg_) {
    names.emplace_back(name);
  }
  return names;
}

void Monitor::DumpMetricsAgg() {
  auto fd = open(monitor_log_path_.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd < 0) {
    LOG4(ERROR, "open file failed");
  }
  while (running_) {
    auto trunc_res = ftruncate(fd, 0);
    if (trunc_res < 0) {
      LOG4(ERROR, "truncate file failed");
    }
    lseek(fd, 0, SEEK_SET);
    std::stringstream ss;
    ss.str("");
    for (auto& item : metrics_agg_) {
      if (item.second.agg_type_ == AggType::kCdf) {
        // Dump 80, 90, 99, 99.9, 99.99 percentiles. Index in cdf_agg_datas_: 7,8,17,18,19
        ss << item.first << "_80 : " << std::fixed << std::setprecision(2) << item.second.cdf_agg_datas_[7] << "\n"
           << item.first << "_90 : " << std::fixed << std::setprecision(2) << item.second.cdf_agg_datas_[8] << "\n"
           << item.first << "_99 : " << std::fixed << std::setprecision(2) << item.second.cdf_agg_datas_[17] << "\n"
           << item.first << "_999 : " << std::fixed << std::setprecision(2) << item.second.cdf_agg_datas_[18] << "\n"
           << item.first << "_9999 : " << std::fixed << std::setprecision(2) << item.second.cdf_agg_datas_[19] << "\n";
      } else {
        ss << item.first << " : " << std::fixed << std::setprecision(2) << item.second.trend_agg_datas_[173] << "\n";
      }
    }
    auto content_size = write(fd, ss.str().c_str(), ss.str().size());
    if (content_size < 0) {
      LOG4(ERROR, "write file failed");
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  close(fd);
}
} // namespace netease::grps
