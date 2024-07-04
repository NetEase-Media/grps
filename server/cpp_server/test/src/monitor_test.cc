/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/09/13
 * Brief  Monitor test.
 */

#include "monitor/monitor.h"

#include <gtest/gtest.h>

#include "logger/logger.h"

int parallel_num = 4;

TEST(monitor_test, test_inc) {
  std::vector<std::thread> threads;

  for (int i = 0; i < parallel_num; i++) {
    threads.emplace_back([]() {
      for (int i = 0; i < 1200; i++) {
        MONITOR_INC("test", 1.0);
        MONITOR_INC("test", 2.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

TEST(monitor_test, test_avg) {
  std::vector<std::thread> threads;

  for (int i = 0; i < parallel_num; i++) {
    threads.emplace_back([]() {
      for (int i = 0; i < 1200; i++) {
        MONITOR_AVG("test", 1.0);
        MONITOR_AVG("test", 2.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

TEST(monitor_test, test_max) {
  std::vector<std::thread> threads;

  for (int i = 0; i < parallel_num; i++) {
    threads.emplace_back([]() {
      for (int i = 0; i < 1200; i++) {
        MONITOR_MAX("test", 1.0);
        MONITOR_MAX("test", 2.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

TEST(monitor_test, test_min) {
  std::vector<std::thread> threads;

  for (int i = 0; i < parallel_num; i++) {
    threads.emplace_back([]() {
      for (int i = 0; i < 1200; i++) {
        MONITOR_MIN("test", 1.0);
        MONITOR_MIN("test", 2.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

int main(int argc, char** argv) {
  if (argc == 2) { // parse parallel_num
    parallel_num = std::stoi(argv[1]);
  }

  // Init logger.
  std::string sys_log_path = "./logs/grps_server.log";
  std::string usr_log_path = "./logs/grps_usr.log";
  netease::grps::DailyLogger::Instance().Init(sys_log_path, 7, usr_log_path, 7);

  auto& monitor_inst = netease::grps::Monitor::Instance();
  monitor_inst.Init();
  monitor_inst.Start();
  ::testing::InitGoogleTest(&argc, argv);

  auto ret = RUN_ALL_TESTS();

  monitor_inst.Stop();
  return ret;
}