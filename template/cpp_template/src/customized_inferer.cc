// Customized deep learning model inferer. Including model load and model infer.

#include "customized_inferer.h"

#include "logger/logger.h"

namespace netease::grps {
YourInferer::YourInferer() = default;
YourInferer::~YourInferer() = default;

void YourInferer::Init(const std::string& path, const std::string& device, const YAML::Node& args) {
  ModelInferer::Init(path, device, args);
  // Add your codes here.
  CLOG4(INFO, "your inferer init success. path: " << path << ", device: " << device << ", args: " << args);
}

void YourInferer::Load() {
  // Add your codes here.
  CLOG4(INFO, "your inferer load success.");
}

void YourInferer::Infer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                        std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                        GrpsContext& ctx) {
  // Add your codes here.

  // If using streaming, you can respond multi msg to client as follows:
  if (ctx.IfStreaming()) {
    ::grps::protos::v1::GrpsMessage msg;
    msg.set_str_data("hello client.");
    ctx.StreamingRespond(msg);
    usleep(1e4); // Simulate the process of model infer.
    ::grps::protos::v1::GrpsMessage msg2;
    msg2.set_str_data("hello client2.");
    ctx.StreamingRespond(msg2);
    usleep(1e4); // Simulate the process of model infer.
  }
}

void YourInferer::BatchInfer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                             std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                             std::vector<GrpsContext*>& ctxs) {
  // Infer in batch. Ctxs is the context of each request in batch.
  // Add your codes here.

  // If using streaming, you can respond multi msg to client as follows:
  for (auto ctx : ctxs) {
    if (ctx->IfStreaming()) {
      ::grps::protos::v1::GrpsMessage msg;
      msg.set_str_data("hello client.");
      ctx->StreamingRespond(msg);
      usleep(1e4); // Simulate the process of model infer.
      ::grps::protos::v1::GrpsMessage msg2;
      msg2.set_str_data("hello client2.");
      ctx->StreamingRespond(msg2);
      usleep(1e4); // Simulate the process of model infer.
    }
  }
}

// --------------------------------------- No converter mode [BEGIN] ---------------------------------------

void YourInferer::Infer(const ::grps::protos::v1::GrpsMessage& input,
                        ::grps::protos::v1::GrpsMessage& output,
                        netease::grps::GrpsContext& ctx) {
  // Add your codes here.

  // If using streaming, you can respond multi msg to client as follows:
  if (ctx.IfStreaming()) {
    ::grps::protos::v1::GrpsMessage msg;
    msg.set_str_data("hello client.");
    ctx.StreamingRespond(msg);
    usleep(1e4); // Simulate the process of model infer.
    ::grps::protos::v1::GrpsMessage msg2;
    msg2.set_str_data("hello client2.");
    ctx.StreamingRespond(msg2);
    usleep(1e4); // Simulate the process of model infer.
  }
}

void YourInferer::BatchInfer(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                             std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                             std::vector<GrpsContext*>& ctxs) {
  // Infer in batch. Inputs, outputs, ctxs are the request, response and context of each request in batch.
  // Add your codes here.

  // If using streaming, you can respond multi msg to client as follows:
  for (auto ctx : ctxs) {
    if (ctx->IfStreaming()) {
      ::grps::protos::v1::GrpsMessage msg;
      msg.set_str_data("hello client.");
      ctx->StreamingRespond(msg);
      usleep(1e4); // Simulate the process of model infer.
      ::grps::protos::v1::GrpsMessage msg2;
      msg2.set_str_data("hello client2.");
      ctx->StreamingRespond(msg2);
      usleep(1e4); // Simulate the process of model infer.
    }
  }
}

// --------------------------------------- No converter mode [END] ---------------------------------------
} // namespace netease::grps