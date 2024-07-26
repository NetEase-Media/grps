/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/06
 * Brief  Daily logger by log4cxx.
 */

#include "logger.h"

#include <thread>

#include "config/global_config.h"

namespace netease::grps {
void DailyLogger::Init(const std::string& sys_log_path,
                       int sys_log_backup_count,
                       const std::string& usr_log_path,
                       int usr_log_backup_count) {
  sys_log_path_ = GlobalConfig::Instance().mpi().world_rank > 0
                    ? sys_log_path + "-rank" + std::to_string(GlobalConfig::Instance().mpi().world_rank)
                    : sys_log_path;
  usr_log_path_ = GlobalConfig::Instance().mpi().world_rank > 0
                    ? usr_log_path + "-rank" + std::to_string(GlobalConfig::Instance().mpi().world_rank)
                    : usr_log_path;
  sys_log_backup_count_ = sys_log_backup_count;
  usr_log_backup_count_ = usr_log_backup_count;

  sys_layout_ =
    log4cxx::PatternLayoutPtr(new log4cxx::PatternLayout("%d{yyyyMMdd HH:mm:ss.SSS} %t %-.30F:%L %p] %m%n"));
  sys_appender_ = log4cxx::DailyRollingFileAppenderPtr(
    new log4cxx::DailyRollingFileAppender(sys_layout_, sys_log_path_, ".yyyy-MM-dd"));
  sys_appender_->setName("sys");
  sys_appender_->setAppend(true);
  sys_appender_->setImmediateFlush(true);
  sys_appender_->setThreshold(log4cxx::Level::getInfo());
  sys_appender_->activateOptions(sys_pool_);
  sys_logger_ = log4cxx::Logger::getLogger("sys");
  sys_logger_->addAppender(sys_appender_);
  sys_logger_->setLevel(log4cxx::Level::getInfo());
  sys_logger_->setAdditivity(false);

  usr_layout_ =
    log4cxx::PatternLayoutPtr(new log4cxx::PatternLayout("%d{yyyyMMdd HH:mm:ss.SSS} %t %-.30F:%L %p] %m%n"));
  usr_appender_ = log4cxx::DailyRollingFileAppenderPtr(
    new log4cxx::DailyRollingFileAppender(usr_layout_, usr_log_path_, ".yyyy-MM-dd"));
  usr_appender_->setName("usr");
  usr_appender_->setAppend(true);
  usr_appender_->setImmediateFlush(true);
  usr_appender_->setThreshold(log4cxx::Level::getInfo());
  usr_appender_->activateOptions(usr_pool_);
  usr_logger_ = log4cxx::Logger::getLogger("usr");
  usr_logger_->addAppender(usr_appender_);
  usr_logger_->setLevel(log4cxx::Level::getInfo());
  usr_logger_->setAdditivity(false);

  StartCleaner(); // Start cleaner thread to clean periodically.

  LOG4(INFO, "Daily logger initialized, sys_log_path: " << sys_log_path_ << ", usr_log_path: " << usr_log_path_
                                                        << ", sys_log_backup_count: " << sys_log_backup_count
                                                        << ", usr_log_backup_count: " << usr_log_backup_count);
}

void DailyLogger::StartCleaner() {
  std::thread cleaner([this]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::hours(24));
      auto now = std::chrono::system_clock::now();
      auto now_time_t = std::chrono::system_clock::to_time_t(now);
      auto now_tm_sys = *std::localtime(&now_time_t);
      auto now_tm_usr = *std::localtime(&now_time_t);
      now_tm_sys.tm_mday -= sys_log_backup_count_;
      auto sys_clean_time = std::chrono::system_clock::from_time_t(std::mktime(&now_tm_sys));
      now_tm_usr.tm_mday -= usr_log_backup_count_;
      auto usr_clean_time = std::chrono::system_clock::from_time_t(std::mktime(&now_tm_usr));
      auto sys_clean_time_t = std::chrono::system_clock::to_time_t(sys_clean_time);
      auto usr_clean_time_t = std::chrono::system_clock::to_time_t(usr_clean_time);
      auto sys_clean_tm = *std::localtime(&sys_clean_time_t);
      auto usr_clean_tm = *std::localtime(&usr_clean_time_t);
      char sys_clean_path[256 + sys_log_path_.length()];
      char usr_clean_path[256 + usr_log_path_.length()];
      std::string sys_log_path_fmt = sys_log_path_;
      sys_log_path_fmt.append(".%Y-%m-%d");
      std::string usr_log_path_fmt = usr_log_path_;
      usr_log_path_fmt.append(".%Y-%m-%d");
      std::strftime(sys_clean_path, sizeof(sys_clean_path), sys_log_path_fmt.c_str(), &sys_clean_tm);
      std::strftime(usr_clean_path, sizeof(usr_clean_path), usr_log_path_fmt.c_str(), &usr_clean_tm);
      LOG4(INFO, "Clean backup sys log file: " << sys_clean_path);
      LOG4(INFO, "Clean backup usr log file: " << usr_clean_path);
      std::remove(sys_clean_path);
      std::remove(usr_clean_path);
    }
  });
  cleaner.detach();
}
} // namespace netease::grps