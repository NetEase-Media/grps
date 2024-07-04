// Brpc client demo. Complete interface description can be learned from docs/2_Interface.md.

#include <brpc/channel.h>
#include <butil/logging.h>
#include <gflags/gflags.h>
#include <google/protobuf/text_format.h>
#include <grps_apis/grps.brpc.pb.h>

DEFINE_string(server, "0.0.0.0:8081", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_int32(timeout_ms, 400, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");

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

  // Online request.
  stub.Online(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send online request, " << cntl.ErrorText();
    return -1;
  }
  std::string res_str;
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Online response: " << res_str;

  // Check liveness request.
  cntl.Reset();
  stub.CheckLiveness(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send check liveness request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Check liveness response: " << res_str;

  // Check readiness request.
  cntl.Reset();
  stub.CheckReadiness(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send check readiness request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Check readiness response: " << res_str;

  // Predict request.
  cntl.Reset();
  request.set_str_data("hello grps.");
  stub.Predict(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send predict request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Predict response: " << res_str << ", latency: " << cntl.latency_us() << " us";

  // Server metadata request.
  cntl.Reset();
  stub.ServerMetadata(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send server metadata request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Server metadata response: " << res_str;

  // Model metadata request.
  cntl.Reset();
  request.set_str_data("your_model");
  stub.ModelMetadata(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "Fail to send model metadata request, " << cntl.ErrorText();
    return -1;
  }
  ::google::protobuf::TextFormat::PrintToString(response, &res_str);
  LOG(INFO) << "Model metadata response: " << res_str;

  // Offline request.
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
