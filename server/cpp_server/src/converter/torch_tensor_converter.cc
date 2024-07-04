/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/04/17
 * Brief  Torch tensor converters. Converter grps msg to torch tensor when preprocess, and convert torch tensor to grps
 * msg when postprocess.
 */

#include "torch_tensor_converter.h"

#include <torch/script.h>

#include "logger/logger.h"

#define TORCH_TENSOR_CONVERTER_DEBUG 0

#if TORCH_TENSOR_CONVERTER_DEBUG
#include "google/protobuf/text_format.h"
#endif

#define LIST_TO_TORCH_TENSOR(list, list_dtype, tensor, tensor_dtype, offset) \
  do {                                                                       \
    if (tensor_size == list.size()) {                                        \
      for (int i = 0; i < list.size(); ++i) {                                \
        tensor.data_ptr<list_dtype>()[i + offset] = list_dtype(list[i]);     \
      }                                                                      \
    } else {                                                                 \
      err_list_size = list.size();                                           \
      goto SIZE_ERROR;                                                       \
    }                                                                        \
  } while (0)

namespace netease::grps {
TorchTensorConverter::TorchTensorConverter() = default;

TorchTensorConverter::~TorchTensorConverter() = default;

void TorchTensorConverter::Init(const std::string& path, const YAML::Node& args) {
  Converter::Init(path, args);
  LOG4(INFO, "torch tensor converter init, path: " << path << ", args: " << args);
}

c10::ScalarType TorchTensorConverter::GTensorType2TorchType(::grps::protos::v1::DataType dtype) {
  switch (dtype) {
    case ::grps::protos::v1::DataType::DT_UINT8: {
      return at::kByte;
    }
    case ::grps::protos::v1::DataType::DT_INT8: {
      return at::kChar;
    }
    case ::grps::protos::v1::DataType::DT_INT16: {
      return at::kShort;
    }
    case ::grps::protos::v1::DataType::DT_INT32: {
      return at::kInt;
    }
    case ::grps::protos::v1::DataType::DT_INT64: {
      return at::kLong;
    }
    case ::grps::protos::v1::DataType::DT_FLOAT16: {
      return at::kHalf;
      break;
    }
    case ::grps::protos::v1::DataType::DT_FLOAT32: {
      return at::kFloat;
    }
    case ::grps::protos::v1::DataType::DT_FLOAT64: {
      return at::kDouble;
    }
    default:
      LOG4(ERROR, "torch tensor converter dtype error, unsupported dtype: " << dtype);
      throw ConverterException("torch tensor converter dtype error, unsupported dtype.");
  }
}

void TorchTensorConverter::GTensor2TorchTensor(const ::grps::protos::v1::GenericTensor& g_tensor,
                                               const std::string& tensor_name,
                                               at::Tensor& tensor,
                                               size_t tensor_size,
                                               size_t offset) {
  long long err_list_size = 0;
  switch (g_tensor.dtype()) {
    case ::grps::protos::v1::DataType::DT_UINT8: {
      LIST_TO_TORCH_TENSOR(g_tensor.flat_uint8(), uint8_t, tensor, at::kByte, offset);
      break;
    }
    case ::grps::protos::v1::DataType::DT_INT8: {
      LIST_TO_TORCH_TENSOR(g_tensor.flat_int8(), int8_t, tensor, at::kChar, offset);
      break;
    }
    case ::grps::protos::v1::DataType::DT_INT16: {
      LIST_TO_TORCH_TENSOR(g_tensor.flat_int16(), int16_t, tensor, at::kShort, offset);
      break;
    }
    case ::grps::protos::v1::DataType::DT_INT32: {
      LIST_TO_TORCH_TENSOR(g_tensor.flat_int32(), int32_t, tensor, at::kInt, offset);
      break;
    }
    case ::grps::protos::v1::DataType::DT_INT64: {
      LIST_TO_TORCH_TENSOR(g_tensor.flat_int64(), int64_t, tensor, at::kLong, offset);
      break;
    }
    case ::grps::protos::v1::DataType::DT_FLOAT16: {
      LIST_TO_TORCH_TENSOR(g_tensor.flat_float16(), at::Half, tensor, at::kHalf, offset);
      break;
    }
    case ::grps::protos::v1::DataType::DT_FLOAT32: {
      LIST_TO_TORCH_TENSOR(g_tensor.flat_float32(), float, tensor, at::kFloat, offset);
      break;
    }
    case ::grps::protos::v1::DataType::DT_FLOAT64: {
      LIST_TO_TORCH_TENSOR(g_tensor.flat_float64(), double, tensor, at::kDouble, offset);
      break;
    }
    default:
      std::string error_msg = "generic tensor to torch tensor error, tensor: ";
      error_msg += tensor_name;
      error_msg += " unsupported dtype: ";
      error_msg += std::to_string(g_tensor.dtype());
      LOG4(ERROR, error_msg);
      throw ConverterException(error_msg);
  }
  return;

SIZE_ERROR:
  std::string shape_str("[");
  for (const auto& dim : g_tensor.shape()) {
    shape_str += std::to_string(dim) + ",";
  }
  shape_str += "]";
  std::string error_msg = "generic tensor to torch tensor error, tensor: ";
  error_msg += tensor_name;
  error_msg += " size not match, shape: ";
  error_msg += shape_str;
  error_msg += ", expected size: ";
  error_msg += std::to_string(tensor_size);
  error_msg += ", actual size: ";
  error_msg += std::to_string(err_list_size);
  LOG4(ERROR, error_msg);
  throw ConverterException(error_msg);
}

void TorchTensorConverter::TorchTensor2GTensor(const at::Tensor& tensor,
                                               const std::string& tensor_name,
                                               ::grps::protos::v1::GenericTensor& g_tensor,
                                               size_t tensor_size,
                                               size_t offset) {
  // Set data.
  auto flatten_tensor = tensor.view({tensor.numel()});
  if (tensor.dtype() == at::kByte) {
    g_tensor.set_dtype(::grps::protos::v1::DataType::DT_UINT8);
    g_tensor.mutable_flat_uint8()->Resize(int(tensor_size), 0);
    for (int i = 0; i < tensor_size; i++) {
      g_tensor.set_flat_uint8(i, flatten_tensor[long(offset + i)].item<uint8_t>());
    }
  } else if (tensor.dtype() == at::kChar) {
    g_tensor.set_dtype(::grps::protos::v1::DataType::DT_INT8);
    g_tensor.mutable_flat_int8()->Resize(int(tensor_size), 0);
    for (int i = 0; i < tensor_size; i++) {
      g_tensor.set_flat_int8(i, flatten_tensor[long(offset + i)].item<int8_t>());
    }
  } else if (tensor.dtype() == at::kShort) {
    g_tensor.set_dtype(::grps::protos::v1::DataType::DT_INT16);
    g_tensor.mutable_flat_int16()->Resize(int(tensor_size), 0);
    for (int i = 0; i < tensor_size; i++) {
      g_tensor.set_flat_int16(i, flatten_tensor[long(offset + i)].item<int16_t>());
    }
  } else if (tensor.dtype() == at::kInt) {
    g_tensor.set_dtype(::grps::protos::v1::DataType::DT_INT32);
    g_tensor.mutable_flat_int32()->Resize(int(tensor_size), 0);
    for (int i = 0; i < tensor_size; i++) {
      g_tensor.set_flat_int32(i, flatten_tensor[long(offset + i)].item<int32_t>());
    }
  } else if (tensor.dtype() == at::kLong) {
    g_tensor.set_dtype(::grps::protos::v1::DataType::DT_INT64);
    g_tensor.mutable_flat_int64()->Resize(int(tensor_size), 0);
    for (int i = 0; i < tensor_size; i++) {
      g_tensor.set_flat_int64(i, flatten_tensor[long(offset + i)].item<int64_t>());
    }
  } else if (tensor.dtype() == at::kHalf) {
    g_tensor.set_dtype(::grps::protos::v1::DataType::DT_FLOAT16);
    g_tensor.mutable_flat_float16()->Resize(int(tensor_size), 0);
    for (int i = 0; i < tensor_size; i++) {
      g_tensor.set_flat_float16(i, flatten_tensor[long(offset + i)].item<float>());
    }
  } else if (tensor.dtype() == at::kFloat) {
    g_tensor.set_dtype(::grps::protos::v1::DataType::DT_FLOAT32);
    g_tensor.mutable_flat_float32()->Resize(int(tensor_size), 0);
    for (int i = 0; i < tensor_size; i++) {
      g_tensor.set_flat_float32(i, flatten_tensor[long(offset + i)].item<float>());
    }
  } else if (tensor.dtype() == at::kDouble) {
    g_tensor.set_dtype(::grps::protos::v1::DataType::DT_FLOAT64);
    g_tensor.mutable_flat_float64()->Resize(int(tensor_size), 0);
    for (int i = 0; i < tensor_size; i++) {
      g_tensor.set_flat_float64(i, flatten_tensor[long(offset + i)].item<double>());
    }
  } else {
    std::stringstream err;
    err << "torch tensor to generic tensor error, tensor: " << tensor_name << " unsupported dtype: " << tensor.dtype();
    LOG4(ERROR, err.str());
    throw ConverterException(err.str());
  }
}

void TorchTensorConverter::PreProcess(const ::grps::protos::v1::GrpsMessage& input,
                                      std::vector<std::pair<std::string, TensorWrapper>>& output,
                                      GrpsContext& context) {
#if TORCH_TENSOR_CONVERTER_DEBUG
  std::string str;
  google::protobuf::TextFormat::PrintToString(input, &str);
  LOG4(INFO, "torch tensor converter pre process, input: " << str);
#endif

  if (!input.has_gtensors()) {
    LOG4(ERROR, "torch tensor converter pre process error, input has no gtensors data.");
    throw ConverterException("torch tensor converter pre process error, input has no gtensors data.");
  }

  const auto& g_tensors = input.gtensors();

  // Check size.
  if (g_tensors.tensors_size() == 0) {
    LOG4(ERROR, "torch tensor converter pre process error, gtensors tensors size is 0.");
    throw ConverterException("torch tensor converter pre process error, gtensors tensors size is 0.");
  }

  // Check names.
  bool has_name = !g_tensors.tensors(0).name().empty();
  for (const auto& g_tensor : g_tensors.tensors()) {
    if ((has_name && g_tensor.name().empty()) || (!has_name && !g_tensor.name().empty())) {
      LOG4(ERROR,
           "torch tensor converter pre process error, gtensors tensors should all have name or all have no "
           "name(will use default name).");
      throw ConverterException(
        "torch tensor converter pre process error, gtensors tensors should all have name or all have no name(will "
        "use default name).");
    }
  }

  // Convert.
  for (int i = 0; i < g_tensors.tensors_size(); i++) {
    const auto& g_tensor = g_tensors.tensors(i);

    long long tensor_size = 1;
    std::vector<int64_t> shape;
    for (const auto& dim : g_tensor.shape()) {
      tensor_size *= dim;
      shape.push_back(dim);
    }

    auto tensor = std::make_shared<torch::Tensor>(torch::empty(shape, GTensorType2TorchType(g_tensor.dtype())));
    GTensor2TorchTensor(g_tensor, has_name ? g_tensor.name() : "", *tensor, tensor_size, 0);
    output.emplace_back(has_name ? g_tensor.name() : "", std::move(tensor));
  }

#if TORCH_TENSOR_CONVERTER_DEBUG
  for (const auto& [name, val] : output) {
    LOG4(INFO,
         "torch tensor converter pre process success, tensor name: " << name << ", tensor val: " << *val.torch_tensor);
  }
#endif
}

void TorchTensorConverter::PostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                                       ::grps::protos::v1::GrpsMessage& output,
                                       GrpsContext& context) {
#if TORCH_TENSOR_CONVERTER_DEBUG
  for (const auto& [name, val] : input) {
    LOG4(INFO, "torch tensor converter post process, tensor name: " << name << ", tensor val: " << *val.torch_tensor);
  }
#endif

  auto* g_tensors = output.mutable_gtensors();
  for (const auto& [name, val] : input) {
    ::grps::protos::v1::GenericTensor g_tensor;
    auto& tensor = *val.torch_tensor;
    // Set name.
    g_tensor.set_name(name);
    // Set shape.
    for (const auto& dim : tensor.sizes()) {
      g_tensor.add_shape(dim);
    }
    // Set data.
    TorchTensor2GTensor(val.torch_tensor->to(torch::kCPU), name, g_tensor, tensor.numel(), 0);
    *g_tensors->add_tensors() = std::move(g_tensor);
  }

#if TORCH_TENSOR_CONVERTER_DEBUG
  std::string str;
  google::protobuf::TextFormat::PrintToString(output, &str);
  LOG4(INFO, "torch tensor converter post process success, output: " << str);
#endif
}

void TorchTensorConverter::BatchPreProcess(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                           std::vector<std::pair<std::string, TensorWrapper>>& output,
                                           std::vector<GrpsContext*>& ctxs) {
#if TORCH_TENSOR_CONVERTER_DEBUG
  for (const auto& input : inputs) {
    std::string str;
    google::protobuf::TextFormat::PrintToString(*input, &str);
    LOG4(INFO, "torch tensor converter batch pre process, input: " << str);
  }
#endif

  if (inputs.size() != ctxs.size()) {
    LOG4(ERROR, "torch tensor converter batch pre process error, inputs size not match ctxs size.");
    throw ConverterException("torch tensor converter batch pre process error, inputs size not match ctxs size.");
  }

  // 1. Initialize batched tensors names, dtype and shape from all inputs.
  std::vector<std::string> tensor_names;
  std::vector<c10::ScalarType> tensor_dtypes;
  std::vector<std::vector<int64_t>> tensor_shapes;
  int64_t total_batch_size = 0;
  for (int i = 0; i < inputs.size(); i++) {
    const auto& g_tensors = inputs[i]->gtensors();
    // Check size.
    if (g_tensors.tensors_size() == 0) {
      LOG4(ERROR, "torch tensor converter batch pre process error, some one gtensors tensors size is 0.");
      throw ConverterException("torch tensor converter batch pre process error, some one gtensors tensors size is 0.");
    }

    // Check names and shape of each tensor.
    bool has_name = !g_tensors.tensors(0).name().empty();
    int64_t cur_batch_size = g_tensors.tensors(0).shape(0);
    for (const auto& g_tensor : g_tensors.tensors()) {
      if ((has_name && g_tensor.name().empty()) || (!has_name && !g_tensor.name().empty())) {
        LOG4(ERROR,
             "torch tensor converter batch pre process error, gtensors tensors should all have name or all have no "
             "name(will use default name).");
        throw ConverterException(
          "torch tensor converter pre batch process error, gtensors tensors should all have name or all have no "
          "name(will use default name).");
      }
      if (g_tensor.shape_size() <= 1) {
        LOG4(ERROR, "torch tensor converter batch pre process error, tensor shape size should be greater than 1.");
        throw ConverterException(
          "torch tensor converter batch pre process error, tensor shape size should be greater than 1.");
      }
      if (cur_batch_size != g_tensor.shape(0)) {
        LOG4(ERROR, "torch tensor converter batch pre process error, batch size of each tensor not match.");
        throw ConverterException(
          "torch tensor converter batch pre process error, batch size of each tensor not match.");
      }
    }

    if (i == 0) { // Initialize tensors names, dtypes and shapes use first input.
      for (int j = 0; j < g_tensors.tensors_size(); ++j) {
        tensor_names.emplace_back(has_name ? g_tensors.tensors(j).name() : "");
        tensor_dtypes.emplace_back(GTensorType2TorchType(g_tensors.tensors(j).dtype()));
        std::vector<int64_t> shape;
        for (int k = 0; k < g_tensors.tensors(j).shape_size(); ++k) {
          shape.emplace_back(g_tensors.tensors(j).shape(k));
        }
        tensor_shapes.emplace_back(std::move(shape));
      }
    } else { // Compare names, dtypes and shapes of follow inputs with first input.
      if (g_tensors.tensors_size() != tensor_names.size()) {
        LOG4(ERROR, "torch tensor converter batch pre process error, tensors size not match.");
        throw ConverterException("torch tensor converter batch pre process error, tensors size not match.");
      }

      for (int j = 0; j < g_tensors.tensors_size(); ++j) {
        // Check names.
        if (tensor_names[j] != (has_name ? g_tensors.tensors(j).name() : "")) {
          LOG4(ERROR, "torch tensor converter batch pre process error, tensor names not match.");
          throw ConverterException("torch tensor converter batch pre process error, tensor names not match.");
        }
        // Check dtypes.
        if (tensor_dtypes[j] != GTensorType2TorchType(g_tensors.tensors(j).dtype())) {
          LOG4(ERROR, "torch tensor converter batch pre process error, tensor dtypes not match.");
          throw ConverterException("torch tensor converter batch pre process error, tensor dtypes not match.");
        }
        // Check shapes[1:].
        if (tensor_shapes[j].size() != g_tensors.tensors(j).shape_size()) {
          LOG4(ERROR, "torch tensor converter batch pre process error, tensor shapes not match.");
          throw ConverterException("torch tensor converter batch pre process error, tensor shapes not match.");
        }
        for (int k = 1; k < g_tensors.tensors(j).shape_size(); ++k) {
          if (tensor_shapes[j][k] != g_tensors.tensors(j).shape(k)) {
            LOG4(ERROR, "torch tensor converter batch pre process error, tensor shapes not match.");
            throw ConverterException("torch tensor converter batch pre process error, tensor shapes not match.");
          }
        }
      }
    }
    total_batch_size += cur_batch_size;
  }
  for (size_t i = 0; i < tensor_names.size(); i++) {
    auto tensor_shape = tensor_shapes[i];
    tensor_shape[0] = total_batch_size;
    auto tensor = std::make_shared<torch::Tensor>(torch::empty(tensor_shape, tensor_dtypes[i]));
    output.emplace_back(tensor_names[i], std::move(tensor));
#if TORCH_TENSOR_CONVERTER_DEBUG
    LOG4(INFO, "torch tensor converter batch pre process, tensor name: "
                 << tensor_names[i] << ", dtype: " << tensor_dtypes[i] << ", shape: " << tensor_shape);
#endif
  }

  // 2. Insert data to batched tensors.
  std::vector<size_t> offsets(tensor_names.size(), 0);
  for (size_t i = 0; i < inputs.size(); i++) {
    const auto& g_tensors = inputs[i]->gtensors();
    for (int j = 0; j < g_tensors.tensors_size(); ++j) {
      size_t tensor_size = 1;
      for (const auto& dim : g_tensors.tensors(j).shape()) {
        tensor_size *= dim;
      }
      GTensor2TorchTensor(g_tensors.tensors(j), tensor_names[j], *output[j].second.torch_tensor, tensor_size,
                          offsets[j]);
      offsets[j] += tensor_size;
    }

    ctxs[i]->SetUserData<long long>(g_tensors.tensors(0).shape(0));
  }

#if TORCH_TENSOR_CONVERTER_DEBUG
  for (const auto& [name, tensor] : output) {
    LOG4(INFO,
         "torch tensor converter batch pre process, tensor name: " << name << ", tensor: " << *tensor.torch_tensor);
  }
#endif
}

void TorchTensorConverter::BatchPostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                                            std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                            std::vector<GrpsContext*>& ctxs) {
#if TORCH_TENSOR_CONVERTER_DEBUG
  for (const auto& [name, tensor] : input) {
    LOG4(INFO,
         "torch tensor converter batch post process, tensor name: " << name << ", tensor: " << *tensor.torch_tensor);
  }
#endif

  if (outputs.size() != ctxs.size()) {
    LOG4(ERROR, "torch tensor converter batch post process error, outputs size not match ctxs size.");
    throw ConverterException("torch tensor converter batch post process error, outputs size not match ctxs size.");
  }

  // Place all gpu tensor to cpu.
  for (auto& [name, tensor] : input) {
    *tensor.torch_tensor = tensor.torch_tensor->to(torch::kCPU);
  }

  std::vector<size_t> offsets(input.size(), 0);
  for (size_t i = 0; i < ctxs.size(); i++) {
    auto batch_size = ctxs[i]->GetUserData<long long>();
    for (size_t j = 0; j < input.size(); j++) {
      auto& [name, tensor_wrapper] = input[j];
      auto& tensor = *tensor_wrapper.torch_tensor;
      auto tensor_size = tensor.numel() / tensor.size(0) * batch_size;

      ::grps::protos::v1::GenericTensor g_tensor;
      // Set name.
      g_tensor.set_name(name);
      // Set shape.
      g_tensor.add_shape(batch_size);
      for (int k = 1; k < tensor.dim(); ++k) {
        g_tensor.add_shape(tensor.size(k));
      }
      // Set data.
      TorchTensor2GTensor(tensor, name, g_tensor, tensor_size, offsets[j]);
      *outputs[i]->mutable_gtensors()->add_tensors() = std::move(g_tensor);

      offsets[j] += tensor_size;
    }
  }

#if TORCH_TENSOR_CONVERTER_DEBUG
  for (const auto& output : outputs) {
    std::string str;
    google::protobuf::TextFormat::PrintToString(*output, &str);
    LOG4(INFO, "torch tensor converter batch post process, output: " << str);
  }
#endif
}

} // namespace netease::grps