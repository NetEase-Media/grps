// Customized converter of model, including pre-process and post-process.

#include "customized_converter.h"

#include "logger/logger.h"

namespace netease::grps {

// Should be copiable and movable.
struct YourCtxVal {
  // Add your codes here.
  std::string val;
};

YourConverter::YourConverter() = default;
YourConverter::~YourConverter() = default;

void YourConverter::Init(const std::string& path, const YAML::Node& args) {
  Converter::Init(path, args);
  // Add your codes here.
  CLOG4(INFO, "your converter init, path: " << path << ", args: " << args);
}

void YourConverter::PreProcess(const ::grps::protos::v1::GrpsMessage& input,
                               std::vector<std::pair<std::string, TensorWrapper>>& output,
                               GrpsContext& ctx) {
  // Preprocess request and convert to tensors.
  // Add your codes here.

  // Your can set context user data of current request as follows:
  YourCtxVal your_ctx_val{"your_value"};
  ctx.SetUserData(std::move(your_ctx_val)); // Use move to avoid copy.

  // Set tf tensor like:
  // std::shared_ptr<tensorflow::Tensor> tensor = std::make_shared<tensorflow::Tensor>();
  // output.emplace_back("your_tensor_name", tensor);

  // Set torch tensor like:
  // std::shared_ptr<at::Tensor> tensor = std::make_shared<at::Tensor>();
  // output.emplace_back("your_tensor_name", tensor);

  // Set eigen tensor like:
  // std::shared_ptr<Eigen::Tensor<float, 2>> tensor = std::make_shared<Eigen::Tensor<float, 2>>(1, 2);
  // (*tensor)(0, 0) = 1.0;
  // (*tensor)(0, 1) = 2.0;
  // output.emplace_back("your_tensor_name", tensor);
}

void YourConverter::PostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                                ::grps::protos::v1::GrpsMessage& output,
                                GrpsContext& ctx) {
  // Postprocess tensors and convert to response.
  // Add your codes here like:

  // Your can get context user data of current request as follows:
  auto& your_value = ctx.GetUserData<YourCtxVal>(); // Use reference to avoid copy.
  CLOG4(INFO, "your context user data value: " << your_value.val);

  output.set_str_data("hello grps."); // Add string data.

  // Add generic tensor([[1.0, 2.0, 3.0]]) like:
  /*
  auto* g_tensor = output.mutable_gtensors()->add_tensors();
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
  */
}

void YourConverter::BatchPreProcess(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                    std::vector<std::pair<std::string, TensorWrapper>>& output,
                                    std::vector<GrpsContext*>& ctxs) {
  // You can preprocess every request and convert to tensor. Merge tensors of each request to batch tensor.
  // Add your codes here.
}

void YourConverter::BatchPostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                                     std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                     std::vector<GrpsContext*>& ctxs) {
  // You can postprocess batch tensor and convert to response. Split batch tensor to tensors of each request.
  // Add your codes here.
}
} // namespace netease::grps
