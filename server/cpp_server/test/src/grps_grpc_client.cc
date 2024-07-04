#include <gflags/gflags.h>
#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>

#include <string>
#include <thread>

#include "grps.grpc.pb.h"

DEFINE_string(server, "0.0.0.0:8080", "IP Address of server");
DEFINE_string(test_mode, "single", "Test mode[single|loop]");
DEFINE_int32(concurrency, 1, "Number of concurrent requests");
DEFINE_string(str_data, "", "string data in request");
DEFINE_string(bin_data, "", "binary data in request");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(FLAGS_server, grpc::InsecureChannelCredentials());

  std::unique_ptr<::grps::protos::v1::GrpsService::Stub> stub = ::grps::protos::v1::GrpsService::NewStub(channel);

  ::grps::protos::v1::GrpsMessage request;
  ::grps::protos::v1::GrpsMessage response;

  {
    grpc::ClientContext context;
    grpc::Status status = stub->Online(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send online request, " << status.error_message();
      return -1;
    }
    std::string res_str;
    ::google::protobuf::TextFormat::PrintToString(response, &res_str);
    LOG(INFO) << "Online response: " << res_str;
  }

  {
    grpc::ClientContext context;
    grpc::Status status = stub->CheckLiveness(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send check liveness request, " << status.error_message();
      return -1;
    }
    std::string res_str;
    ::google::protobuf::TextFormat::PrintToString(response, &res_str);
    LOG(INFO) << "Check liveness response: " << res_str;
  }

  {
    grpc::ClientContext context;
    grpc::Status status = stub->CheckReadiness(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send check readiness request, " << status.error_message();
      return -1;
    }
    std::string res_str;
    ::google::protobuf::TextFormat::PrintToString(response, &res_str);
    LOG(INFO) << "Check readiness response: " << res_str;
  }

  std::vector<std::thread> threads;
  threads.reserve(FLAGS_concurrency);
  for (int i = 0; i < FLAGS_concurrency; ++i) {
    threads.emplace_back([&]() {
      ::grps::protos::v1::GrpsMessage request;
      ::grps::protos::v1::GrpsMessage response;
      if (!FLAGS_str_data.empty()) {
        request.set_str_data(FLAGS_str_data);
      } else if (!FLAGS_bin_data.empty()) {
        request.set_bin_data(FLAGS_bin_data.c_str());
      }
      while (true) {
        grpc::ClientContext context;
        auto begin_us =
          std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
        grpc::Status status = stub->Predict(&context, request, &response);
        if (!status.ok()) {
          LOG(ERROR) << "Fail to send predict request, " << status.error_message();
          return -1;
        }
        auto end_us =
          std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
        std::string res_str;
        ::google::protobuf::TextFormat::PrintToString(response, &res_str);
        LOG(INFO) << "Predict response: " << res_str << ", latency: " << end_us - begin_us << " us";
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

  {
    grpc::ClientContext context;
    auto stream = stub->PredictStreaming(&context, request);
    while (stream->Read(&response)) {
      std::string res_str;
      ::google::protobuf::TextFormat::PrintToString(response, &res_str);
      LOG(INFO) << "Predict streaming response: " << res_str;
    }
    grpc::Status status = stream->Finish();
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send predict streaming request, " << status.error_message();
      return -1;
    }
  }

  {
    grpc::ClientContext context;
    grpc::Status status = stub->ServerMetadata(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send server metadata request, " << status.error_message();
      return -1;
    }
    std::string res_str;
    ::google::protobuf::TextFormat::PrintToString(response, &res_str);
    LOG(INFO) << "Server metadata response: " << res_str;
  }

  {
    grpc::ClientContext context;
    grpc::Status status = stub->ModelMetadata(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send model metadata request, " << status.error_message();
      return -1;
    }
    std::string res_str;
    ::google::protobuf::TextFormat::PrintToString(response, &res_str);
    LOG(INFO) << "Model metadata response: " << res_str;
  }

  {
    grpc::ClientContext context;
    grpc::Status status = stub->Offline(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send offline request, " << status.error_message();
      return -1;
    }
    std::string res_str;
    ::google::protobuf::TextFormat::PrintToString(response, &res_str);
    LOG(INFO) << "Offline response: " << res_str;
  }
  return 0;
}
