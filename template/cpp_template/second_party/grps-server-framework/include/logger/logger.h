/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/06
 * Brief  Daily logger by log4cxx.
 *
 * Usage:
 * #include "logger/logger.h"
 * // server logger.
 * LOG4(INFO, "a = " << 1);
 * LOG4(WARN, "a = " << 1);
 * LOG4(ERROR, "a = " << 1);
 * LOG4(FATAL, "a = " << 1);
 * // customer logger.
 * CLOG4(INFO, "a = " << 1);
 * CLOG4(WARN, "a = " << 1);
 * CLOG4(ERROR, "a = " << 1);
 * CLOG4(FATAL, "a = " << 1);
 */

#pragma once

#include <log4cxx/dailyrollingfileappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>

#include <cstdarg>
#include <cstdio>

// System log4xx.
#define LOG4(level, ...)                                                \
  do {                                                                  \
    LOG4CXX_##level(DailyLogger::Instance().sys_logger(), __VA_ARGS__); \
  } while (0)

// Customer log4xx.
#define CLOG4(level, ...)                                               \
  do {                                                                  \
    LOG4CXX_##level(DailyLogger::Instance().usr_logger(), __VA_ARGS__); \
  } while (0)

namespace netease::grps {
class DailyLogger {
public:
  ~DailyLogger() = default;
  DailyLogger(const DailyLogger&) = delete;
  DailyLogger& operator=(const DailyLogger&) = delete;
  DailyLogger(DailyLogger&&) = delete;
  DailyLogger& operator=(DailyLogger&&) = delete;

  static DailyLogger& Instance() {
    static DailyLogger instance;
    return instance;
  }

  void Init(const std::string& sys_log_path,
            int sys_log_backup_count,
            const std::string& usr_log_path,
            int usr_log_backup_count);

  [[nodiscard]] log4cxx::LoggerPtr sys_logger() const { return sys_logger_; }

  [[nodiscard]] log4cxx::LoggerPtr usr_logger() const { return usr_logger_; }

private:
  DailyLogger() = default;

  void StartCleaner();

  std::string sys_log_path_;
  std::string usr_log_path_;
  int sys_log_backup_count_ = 0;
  int usr_log_backup_count_ = 0;

  log4cxx::LoggerPtr sys_logger_; // system logger.
  log4cxx::PatternLayoutPtr sys_layout_;
  log4cxx::DailyRollingFileAppenderPtr sys_appender_;
  log4cxx::helpers::Pool sys_pool_; // system pool.
  log4cxx::LoggerPtr usr_logger_;   // user logger.
  log4cxx::PatternLayoutPtr usr_layout_;
  log4cxx::DailyRollingFileAppenderPtr usr_appender_;
  log4cxx::helpers::Pool usr_pool_; // user pool.
};
} // namespace netease::grps
