/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/06
 * Brief  main
 */

#include <brpc/server.h>
#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h>

#include <fstream>
#include <regex>

#define BACKWARD_HAS_DW 1
#include "common/backward.hpp"
#include "common/global_gflags.h"
#include "config/global_config.h"
#include "constant.h"
#include "customized/grps_server_customized.h"
#include "executor/executor.h"
#include "logger/logger.h"
#include "mem_manager/gpu_mem_mgr.h"
#include "monitor/monitor.h"
#include "service/grps_service.h"
#include "service/js_service.h"
#include "service/monitor_service.h"
#include "system_monitor/system_monitor.h"

namespace backward {
backward::SignalHandling sh;
}

using namespace netease::grps;

int main(int argc, char** argv) {
  // Dump pid.
  std::string pid_path = "./PID";
  pid_t pid = getpid();
  std::fstream pid_file(pid_path, std::ios::out | std::ios::trunc);
  assert(pid_file.is_open());
  pid_file << pid;
  pid_file.flush();
  pid_file.close();

  // Dump version.
  std::string version_path = "./VERSION";
  std::fstream version_file(version_path, std::ios::out | std::ios::trunc);
  assert(version_file.is_open());
  version_file << GRPS_VERSION;
  version_file.close();

  GrpsServerCustomizedLibInit(); // Init customized lib.

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::SetCommandLineOption("bvar_dump", "true");
  google::SetCommandLineOption("bvar_dump_file", "./logs/monitor/bvar.<app>.data");
  google::SetCommandLineOption("flagfile", "./conf/gflags.conf");
  google::SetCommandLineOption("usercode_in_pthread", "true");

  // Load global config.
  if (!GlobalConfig::Instance().Load()) {
    return -1;
  }
  const auto& server_config = GlobalConfig::Instance().server_config();

  // Init logger.
  std::string sys_log_path = server_config.log.log_dir + "/grps_server.log";
  std::string usr_log_path = server_config.log.log_dir + "/grps_usr.log";
  DailyLogger::Instance().Init(sys_log_path, server_config.log.log_backup_count, usr_log_path,
                               server_config.log.log_backup_count);

  // Init and start monitor.
  Monitor::Instance().Init();
  Monitor::Instance().Start();
  LOG4(INFO, "Start monitor success.");

  // System monitor: cpu & gpu
  try {
    SystemMonitor::Instance().Init();
    SystemMonitor::Instance().Start();
  } catch (const std::exception& e) {
    LOG4(FATAL, "Init system monitor failed: " << e.what());
    abort();
  }

  // Init executor.
  try {
    Executor::Instance().Init();
  } catch (const std::exception& e) {
    LOG4(FATAL, "Init executor failed: " << e.what());
    abort();
  }

  // Check interface framework.
  if (server_config.interface.framework != "http" && server_config.interface.framework != "http+brpc" &&
      server_config.interface.framework != "http+grpc") {
    LOG4(FATAL,
         "Invalid framework: " << server_config.interface.framework << ", must be http or http+brpc or http+grpc.");
    abort();
  }

  // Check interface host.
  auto& host = server_config.interface.host;
  std::regex ip_regex(R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$)");
  if (!std::regex_match(host, ip_regex)) {
    LOG4(FATAL, "Invalid host: " << host << ", must be ip.");
    abort();
  }

  // Remove space for server_config.interface.port and split by ,
  std::string port_str = server_config.interface.port;
  port_str.erase(std::remove_if(port_str.begin(), port_str.end(), isspace), port_str.end());
  std::vector<std::string> ports;
  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while ((pos = port_str.find(',', prev)) != std::string::npos) {
    ports.emplace_back(port_str.substr(prev, pos - prev));
    prev = pos + 1;
  }
  ports.emplace_back(port_str.substr(prev));

  // Check port.
  int http_port = -1;
  int rpc_port = -1;
  if (server_config.interface.framework == "http" && ports.size() == 1) {
    if (!std::all_of(ports[0].begin(), ports[0].end(), ::isdigit)) {
      LOG4(FATAL, "Invalid port: " << ports[0] << ", must be digit.");
      abort();
    }
    http_port = std::stoi(ports[0]);
  } else if ((server_config.interface.framework == "http+brpc" || server_config.interface.framework == "http+grpc") &&
             ports.size() == 2) {
    if (!std::all_of(ports[0].begin(), ports[0].end(), ::isdigit)) {
      LOG4(FATAL, "Invalid port: " << ports[0] << ", must be digit.");
      abort();
    }
    if (!std::all_of(ports[1].begin(), ports[1].end(), ::isdigit)) {
      LOG4(FATAL, "Invalid port: " << ports[1] << ", must be digit.");
      abort();
    }
    http_port = std::stoi(ports[0]);
    rpc_port = std::stoi(ports[1]);
  } else {
    LOG4(FATAL, "Invalid port, framework: " << server_config.interface.framework
                                            << ", port: " << server_config.interface.port);
    abort();
  }

  // Init system metrics monitor.
  MONITOR_INC(QPS, 0);
  MONITOR_AVG(REQ_FAIL_RATE, 0);
  MONITOR_AVG(REQ_LATENCY_AVG, 0);
  MONITOR_MAX(REQ_LATENCY_MAX, 0);
  MONITOR_CDF(REQ_LATENCY_CDF, 0);
  MONITOR_AVG(CPU_USAGE_AVG, 0);
  MONITOR_AVG(MEM_USAGE_AVG, 0);

  // Threadpool used for predict.
  g_predict_threadpool = std::make_unique<::boost::asio::thread_pool>(server_config.max_concurrency);

  if (server_config.interface.framework == "http+brpc") {
    std::thread([&]() {
      // Add brpc server.
      brpc::Server server;
      server.set_version(GRPS_VERSION);
      brpc::ServerOptions options;
      options.idle_timeout_sec = FLAGS_idle_timeout_sec;
      options.max_concurrency = server_config.max_connections;
      options.num_threads = server_config.max_concurrency + 1;
      options.pid_file = FLAGS_pid_file;
      options.has_builtin_services = false;
      GrpsBrpcServiceImpl grps_service(GrpsBrpcServiceImpl::HandlerType::kRPC);
      if (server.AddService(&grps_service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG4(FATAL, "Fail to add brpc service.");
        abort();
      }
      LOG4(INFO, "Add grps brpc service success, version: " << GRPS_VERSION << ", port: " << rpc_port
                                                            << ", max_connections: " << server_config.max_connections
                                                            << ", max_concurrency: " << server_config.max_concurrency);

      // Start brpc server.
      std::string server_address = host + ":" + std::to_string(rpc_port);
      if (server.Start(server_address.c_str(), &options) != 0) {
        LOG4(FATAL, "Fail to start brpc server.");
        abort();
      }

      server.RunUntilAskedToQuit();
    }).detach();
  } else if (server_config.interface.framework == "http+grpc") {
    std::thread([&]() {
      // Add grpc server.
      grpc::ServerBuilder builder;
      builder.SetMaxReceiveMessageSize(1024 * 1024 * 1024); // 1G
      builder.SetMaxSendMessageSize(1024 * 1024 * 1024);    // 1G
      // Set threadpool worker count.
      builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::NUM_CQS, 1);
      builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MIN_POLLERS,
                                  std::min(8, server_config.max_connections));
      builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, server_config.max_concurrency);
      builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::CQ_TIMEOUT_MSEC, 1000);
      // Set max connection count.
      grpc::ResourceQuota quota;
      quota.SetMaxThreads(server_config.max_connections + 1);
      builder.SetResourceQuota(quota);

      std::string server_address = host + ":" + std::to_string(rpc_port);
      // Listen on the given address without any authentication mechanism.
      builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
      GrpsGrpcServiceImpl grps_service;
      // Register "service" as the instance through which we'll communicate with
      // clients. In this case it corresponds to an *synchronous* service.
      builder.RegisterService(&grps_service);
      // Finally assemble the server.
      std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
      LOG4(INFO, "Add grps grpc service success, version: " << GRPS_VERSION << ", port: " << rpc_port
                                                            << ", max_connections: " << server_config.max_connections
                                                            << ", max_concurrency: " << server_config.max_concurrency);

      // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
      server->Wait();
    }).detach();
  }

  // Add http server.
  brpc::Server server;
  server.set_version(GRPS_VERSION);
  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_sec;
  options.max_concurrency = server_config.max_connections;
  options.num_threads = server_config.max_concurrency + 1;
  options.pid_file = FLAGS_pid_file;
  options.has_builtin_services = false;
  GrpsBrpcServiceImpl grps_service(GrpsBrpcServiceImpl::HandlerType::kHTTP);
  std::string url_path_map =
    "/grps/v1/infer/predict => PredictByHttp,"
    "/grps/v1/health/online => Online,"
    "/grps/v1/health/offline => Offline,"
    "/grps/v1/health/live => CheckLiveness,"
    "/grps/v1/health/ready => CheckReadiness,"
    "/grps/v1/metadata/server => ServerMetadata,"
    "/grps/v1/metadata/model => ModelMetadata";
  if (GlobalConfig::Instance().server_config()._is_set.customized_predict_http) {
    auto& customized_path = GlobalConfig::Instance().server_config().interface.customized_predict_http.path;
    // Check customized path, cannot use internal path.
    if (customized_path == "/grps/v1/infer/predict" || customized_path == "/grps/v1/health/online" ||
        customized_path == "/grps/v1/health/offline" || customized_path == "/grps/v1/health/live" ||
        customized_path == "/grps/v1/health/ready" || customized_path == "/grps/v1/metadata/server" ||
        customized_path == "/grps/v1/metadata/model" || customized_path == "/grps/v1/js/jquery_min" ||
        customized_path == "/grps/v1/js/flot_min" || customized_path == "/grps/v1/monitor/series" ||
        customized_path == "/grps/v1/monitor/metrics" || customized_path == "/") {
      LOG4(FATAL, "Invalid customized path: " << customized_path << ", cannot use internal path.");
      abort();
    }
    url_path_map +=
      "," + GlobalConfig::Instance().server_config().interface.customized_predict_http.path + " => " + "PredictByHttp";
  }
  if (server.AddService(&grps_service, brpc::SERVER_DOESNT_OWN_SERVICE, url_path_map) != 0) {
    LOG4(FATAL, "Fail to add grps http service.");
    abort();
  }
  LOG4(INFO, "Add grps http service success, version: " << GRPS_VERSION << ", port: " << http_port
                                                        << ", max_connections: " << server_config.max_connections
                                                        << ", max_concurrency: " << server_config.max_concurrency);

  JsServiceImpl js_service;
  if (server.AddService(&js_service, brpc::SERVER_DOESNT_OWN_SERVICE,
                        "/grps/v1/js/jquery_min => JqueryMinJs,"
                        "/grps/v1/js/flot_min => FloatMinJs") != 0) {
    LOG4(FATAL, "Fail to add js http service.");
    abort();
  }
  LOG4(INFO, "Add js http service success, port: " << http_port);

  MonitorServiceImpl monitor_service;
  if (server.AddService(&monitor_service, brpc::SERVER_DOESNT_OWN_SERVICE,
                        "/grps/v1/monitor/series => SeriesData,"
                        "/ => Metrics,"
                        "/grps/v1/monitor/metrics => Metrics") != 0) {
    LOG4(FATAL, "Fail to add monitor http service.");
    abort();
  }
  LOG4(INFO, "Add monitor http service success, port: " << http_port);

  // Start http server.
  std::string server_address = host + ":" + std::to_string(http_port);
  if (server.Start(server_address.c_str(), &options) != 0) {
    LOG4(FATAL, "Fail to start http server.");
    abort();
  }

  // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
  server.RunUntilAskedToQuit();
  Executor::Instance().Terminate();
  return 0;
}
