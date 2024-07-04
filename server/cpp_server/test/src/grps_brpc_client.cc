#include <brpc/channel.h>
#include <butil/logging.h>
#include <gflags/gflags.h>
#include <google/protobuf/text_format.h>

#include <thread>

#include "grps.brpc.pb.h"

DEFINE_string(server, "0.0.0.0:8080", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_int32(timeout_ms, 400, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_string(test_mode, "single", "Test mode[single|loop]");
DEFINE_int32(concurrency, 1, "Number of concurrent requests");
DEFINE_string(str_data, "", "string data in request");
DEFINE_string(bin_data, "", "binary data in request");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  brpc::Channel channel;

  brpc::ChannelOptions options;
  options.protocol = brpc::PROTOCOL_BAIDU_STD;
  options.connection_type = FLAGS_connection_type;
  options.timeout_ms = FLAGS_timeout_ms /*milliseconds*/;
  options.max_retry = FLAGS_max_retry;

  if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0) {
    LOG(ERROR) << "Fail to initialize channel";
    return -1;
  }

  grps::protos::v1::GrpsBrpcService_Stub stub(&channel);

  grps::protos::v1::GrpsMessage request;
  grps::protos::v1::GrpsMessage response;

  brpc::Controller cntl;

  stub.Online(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send online request, " << cntl.ErrorText();
    return -1;
  }
  std::string res_str;
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Online response: " << res_str;

  cntl.Reset();
  stub.CheckLiveness(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send check liveness request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Check liveness response: " << res_str;

  cntl.Reset();
  stub.CheckReadiness(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send check readiness request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Check readiness response: " << res_str;

  std::vector<std::thread> threads;
  threads.reserve(FLAGS_concurrency);

  for (int i = 0; i < FLAGS_concurrency; i++) {
    threads.emplace_back([&]() {
      grps::protos::v1::GrpsMessage request;
      grps::protos::v1::GrpsMessage response;
      if (!FLAGS_str_data.empty()) {
        request.set_str_data(FLAGS_str_data);
      } else if (!FLAGS_bin_data.empty()) {
        request.set_bin_data(FLAGS_bin_data.c_str());
      }

      brpc::Controller cntl;
      while (true) {
        cntl.Reset();
        stub.Predict(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) {
          LOG(ERROR) << "Fail to send predict request, " << cntl.ErrorText();
          return -1;
        }
        std::string res_str;
        ::google::protobuf::TextFormat::PrintToString(response, &res_str);
        LOG(INFO) << "Predict response: " << res_str << ", latency: " << cntl.latency_us() << " us";

        if (FLAGS_test_mode != "loop") {
          break;
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      return 0;
    });
  }
  for (auto& t : threads) {
    t.join();
  }

  cntl.Reset();
  stub.ServerMetadata(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send server metadata request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Server metadata response: " << res_str;

  cntl.Reset();
  stub.ModelMetadata(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send model metadata request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Model metadata response: " << res_str;

  cntl.Reset();
  stub.Offline(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send offline request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Offline response: " << res_str;

  return 0;
}
