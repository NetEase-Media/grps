// Grpc client demo. Complete interface description can be learned from docs/2_Interface.md.

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>
#include <grps_apis/grps.grpc.pb.h>

#include <string>

DEFINE_string(server, "0.0.0.0:8080", "IP Address of server");

#define GET_US() \
  std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(FLAGS_server, grpc::InsecureChannelCredentials());

  std::unique_ptr<::grps::protos::v1::GrpsService::Stub> stub = ::grps::protos::v1::GrpsService::NewStub(channel);

  ::grps::protos::v1::GrpsMessage request;
  ::grps::protos::v1::GrpsMessage response;

  // Online request.
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

  // Check liveness request.
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

  // Check readiness request.
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

  // Predict request.
  {
    grpc::ClientContext context;
    request.set_str_data("hello grps.");
    auto begin_us = GET_US();
    grpc::Status status = stub->Predict(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send predict request, " << status.error_message();
      return -1;
    }
    auto end_us = GET_US();
    std::string res_str;
    ::google::protobuf::TextFormat::PrintToString(response, &res_str);
    LOG(INFO) << "Predict response: " << res_str << ", latency: " << end_us - begin_us << " us";
  }

  // Predict with gtensors request.
  {
    grpc::ClientContext context;
    // Add generic tensor([[1.0, 2.0, 3.0]]) like:
    auto* g_tensor = request.mutable_gtensors()->add_tensors();
    // tensor name
    g_tensor->set_name("tensor_name");
    // shape [1, 3]
    g_tensor->add_shape(1);
    g_tensor->add_shape(3);
    // data type float32
    g_tensor->set_dtype(::grps::protos::v1::DT_FLOAT32);
    // data [1, 2, 3]
    g_tensor->add_flat_float32(1);
    g_tensor->add_flat_float32(2);
    g_tensor->add_flat_float32(3);
    auto begin_us = GET_US();
    grpc::Status status = stub->Predict(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send predict request, " << status.error_message();
      return -1;
    }
    auto end_us = GET_US();
    std::string res_str;
    ::google::protobuf::TextFormat::PrintToString(response, &res_str);
    LOG(INFO) << "Predict response: " << res_str << ", latency: " << end_us - begin_us << " us";
  }

  // Predict streaming request.
  {
    grpc::ClientContext context;
    request.set_str_data("hello grps.");
    auto begin_us = GET_US();
    auto stream = stub->PredictStreaming(&context, request);
    while (stream->Read(&response)) {
      std::string res_str;
      ::google::protobuf::TextFormat::PrintToString(response, &res_str);
      LOG(INFO) << "Predict streaming response: " << res_str;
    }
    grpc::Status status = stream->Finish();
    auto end_us = GET_US();
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send predict streaming request, " << status.error_message();
      return -1;
    }
    LOG(INFO) << "Predict streaming latency: " << end_us - begin_us << " us";
  }

  // Server metadata request.
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

  // Model metadata request.
  {
    grpc::ClientContext context;
    request.set_str_data("your_model");
    grpc::Status status = stub->ModelMetadata(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Fail to send model metadata request, " << status.error_message();
      return -1;
    }
    std::string res_str;
    ::google::protobuf::TextFormat::PrintToString(response, &res_str);
    LOG(INFO) << "Model metadata response: " << res_str;
  }

  // Offline request.
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