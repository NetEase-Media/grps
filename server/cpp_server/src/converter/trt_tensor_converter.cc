/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2024/06/04
 * Brief  TensorRT tensor converters. Converter grps msg to trt tensor when preprocess, and convert trt tensor to
 * grps msg when postprocess.
 */

#include "trt_tensor_converter.h"

#include "model_infer/tensor_wrapper.h"

#define TRT_TENSOR_CONVERTER_DEBUG 0

#if TRT_TENSOR_CONVERTER_DEBUG
#include "google/protobuf/text_format.h"
#endif

namespace netease::grps {
#define LIST_TO_TRT_TENSOR(list, tensor, tensor_dtype, offset)            \
  do {                                                                    \
    auto tensor_data = static_cast<tensor_dtype*>(tensor.buffer().Get()); \
    if (tensor_size == list.size()) {                                     \
      for (int i = 0; i < tensor_size; ++i) {                             \
        tensor_data[i + offset] = tensor_dtype(list.at(i));               \
      }                                                                   \
    } else {                                                              \
      err_list_size = list.size();                                        \
      goto SIZE_ERROR;                                                    \
    }                                                                     \
  } while (0)

void TrtTensorConverter::Init(const std::string& path, const YAML::Node& args) {
  Converter::Init(path, args);
  LOG4(INFO, "Trt tensor converter init, path: " << path << ", args: " << args);
}

nvinfer1::DataType TrtTensorConverter::GTensorType2TrtType(::grps::protos::v1::DataType dtype) {
  switch (dtype) {
#if NV_TENSORRT_MAJOR >= 8 && NV_TENSORRT_MINOR >= 5
    case ::grps::protos::v1::DataType::DT_UINT8:
      return nvinfer1::DataType::kUINT8;
#endif
    case ::grps::protos::v1::DataType::DT_INT8:
      return nvinfer1::DataType::kINT8;
    case ::grps::protos::v1::DataType::DT_INT32:
      return nvinfer1::DataType::kINT32;
    case ::grps::protos::v1::DataType::DT_FLOAT32:
      return nvinfer1::DataType::kFLOAT;
    default:
      auto err = "trt tensor converter dtype error, unsupported dtype: " + std::to_string(dtype);
      LOG4(ERROR, err);
      throw ConverterException(err);
  }
}

void TrtTensorConverter::GTensor2TrtTensor(const ::grps::protos::v1::GenericTensor& g_tensor,
                                           const std::string& tensor_name,
                                           TrtHostBinding& tensor,
                                           size_t tensor_size,
                                           size_t offset) {
  long long err_list_size = 0;
  switch (g_tensor.dtype()) {
#if NV_TENSORRT_MAJOR >= 8 && NV_TENSORRT_MINOR >= 5
    case ::grps::protos::v1::DataType::DT_UINT8: {
      LIST_TO_TRT_TENSOR(g_tensor.flat_uint8(), tensor, uint8_t, offset);
      break;
    }
#endif
    case ::grps::protos::v1::DataType::DT_INT8: {
      LIST_TO_TRT_TENSOR(g_tensor.flat_int8(), tensor, int8_t, offset);
      break;
    }
    case ::grps::protos::v1::DataType::DT_INT32: {
      LIST_TO_TRT_TENSOR(g_tensor.flat_int32(), tensor, int32_t, offset);
      break;
    }
    case ::grps::protos::v1::DataType::DT_FLOAT32: {
      LIST_TO_TRT_TENSOR(g_tensor.flat_float32(), tensor, float, offset);
      break;
    }
    default:
      std::string error_msg = "generic tensor to trt tensor error, tensor: ";
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
  std::string error_msg = "generic tensor to trt tensor error, tensor: ";
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

void TrtTensorConverter::TrtTensor2GTensor(TrtHostBinding& tensor,
                                           const std::string& tensor_name,
                                           ::grps::protos::v1::GenericTensor& g_tensor,
                                           size_t tensor_size,
                                           size_t offset) {
  switch (tensor.data_type()) {
#if NV_TENSORRT_MAJOR >= 8 && NV_TENSORRT_MINOR >= 5
    case nvinfer1::DataType::kUINT8: {
      g_tensor.set_dtype(::grps::protos::v1::DataType::DT_UINT8);
      g_tensor.mutable_flat_uint8()->Resize(int(tensor_size), 0);
      const auto* flat = static_cast<uint8_t*>(tensor.buffer().Get());
      for (int i = 0; i < int(tensor_size); ++i) {
        g_tensor.set_flat_uint8(i, flat[long(offset + i)]);
      }
      break;
    }
#endif
    case nvinfer1::DataType::kINT8: {
      g_tensor.set_dtype(::grps::protos::v1::DataType::DT_INT8);
      g_tensor.mutable_flat_int8()->Resize(int(tensor_size), 0);
      const auto* flat = static_cast<int8_t*>(tensor.buffer().Get());
      for (int i = 0; i < int(tensor_size); ++i) {
        g_tensor.set_flat_int8(i, flat[long(offset + i)]);
      }
      break;
    }
    case nvinfer1::DataType::kINT32: {
      g_tensor.set_dtype(::grps::protos::v1::DataType::DT_INT32);
      g_tensor.mutable_flat_int32()->Resize(int(tensor_size), 0);
      const auto* flat = static_cast<int32_t*>(tensor.buffer().Get());
      for (int i = 0; i < int(tensor_size); ++i) {
        g_tensor.set_flat_int32(i, flat[long(offset + i)]);
      }
      break;
    }
    case nvinfer1::DataType::kFLOAT: {
      g_tensor.set_dtype(::grps::protos::v1::DataType::DT_FLOAT32);
      g_tensor.mutable_flat_float32()->Resize(int(tensor_size), 0);
      const auto* flat = static_cast<float*>(tensor.buffer().Get());
      for (int i = 0; i < int(tensor_size); ++i) {
        g_tensor.set_flat_float32(i, flat[long(offset + i)]);
      }
      break;
    }
    default: {
      std::string error_msg = "trt tensor to generic tensor error, tensor: ";
      error_msg += tensor_name;
      error_msg += " unsupported dtype: ";
      error_msg += std::to_string(int(tensor.data_type()));
      LOG4(ERROR, error_msg);
      throw ConverterException(error_msg);
    }
  }
}

void TrtTensorConverter::PreProcess(const ::grps::protos::v1::GrpsMessage& input,
                                    std::vector<std::pair<std::string, TensorWrapper>>& output,
                                    GrpsContext& ctx) {
#if TRT_TENSOR_CONVERTER_DEBUG
  std::string str;
  google::protobuf::TextFormat::PrintToString(input, &str);
  LOG4(INFO, "trt tensor converter pre process, input: " << str);
#endif
  if (input.has_gtensors()) { // generic tensor pb.
    const auto& g_tensors = input.gtensors();

    // Check size.
    if (g_tensors.tensors_size() == 0) {
      LOG4(ERROR, "trt tensor converter pre process error, gtensors tensors size is 0.");
      throw ConverterException("trt tensor converter pre process error, gtensors tensors size is 0.");
    }

    // Check names.
    bool has_name = !g_tensors.tensors(0).name().empty();
    for (const auto& g_tensor : g_tensors.tensors()) {
      if ((has_name && g_tensor.name().empty()) || (!has_name && !g_tensor.name().empty())) {
        LOG4(ERROR,
             "trt tensor converter pre process error, gtensors tensors should all have name or all have no "
             "name(will use default name).");
        throw ConverterException(
          "trt tensor converter pre process error, gtensors tensors should all have name or all have no name(will "
          "use default name).");
      }
    }

    for (int i = 0; i < g_tensors.tensors_size(); ++i) {
      auto& g_tensor = g_tensors.tensors(i);
      long long tensor_size = 1;
      nvinfer1::Dims shape{};
      for (int j = 0; j < g_tensor.shape_size(); j++) {
        tensor_size *= g_tensor.shape(j);
        shape.nbDims++;
        shape.d[j] = int(g_tensor.shape(j));
      }
      auto tensor =
        std::make_shared<TrtHostBinding>(g_tensor.name().c_str(), shape, GTensorType2TrtType(g_tensor.dtype()));
      GTensor2TrtTensor(g_tensor, has_name ? g_tensor.name() : "", *tensor, tensor_size, 0);
      output.emplace_back(has_name ? g_tensor.name() : "", std::move(tensor));
    }
  } else {
    LOG4(ERROR, "trt tensor converter pre process error, input has no gtensors.");
    throw ConverterException("trt tensor converter pre process error, input has no gtensors.");
  }

#if TRT_TENSOR_CONVERTER_DEBUG
  for (const auto& tensor : output) {
    LOG4(INFO, "trt tensor converter pre process, tensor name: " << tensor.first << ", tensor: "
                                                                 << tensor.second.trt_host_binding->DebugString());
  }
#endif
}

void TrtTensorConverter::PostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                                     ::grps::protos::v1::GrpsMessage& output,
                                     GrpsContext& ctx) {
#if TRT_TENSOR_CONVERTER_DEBUG
  for (const auto& tensor : input) {
    LOG4(INFO, "trt tensor converter post process, tensor name: " << tensor.first << ", tensor: "
                                                                  << tensor.second.trt_host_binding->DebugString());
  }
#endif
  for (const auto& [name, tensor_wrapper] : input) {
    auto& tensor = *tensor_wrapper.trt_host_binding;
    ::grps::protos::v1::GenericTensor g_tensor;

    // Set name.
    g_tensor.set_name(name);

    // Set shape.
    const auto& shape = tensor.dims();
    for (int i = 0; i < shape.nbDims; ++i) {
      g_tensor.add_shape(shape.d[i]);
    }

    // Set data.
    TrtTensor2GTensor(tensor, name, g_tensor, tensor.volume(), 0);
    *output.mutable_gtensors()->add_tensors() = std::move(g_tensor);
  }

#if TRT_TENSOR_CONVERTER_DEBUG
  std::string str;
  google::protobuf::TextFormat::PrintToString(output, &str);
  LOG4(INFO, "trt tensor converter post process, output: " << str);
#endif
}

void TrtTensorConverter::BatchPreProcess(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                         std::vector<std::pair<std::string, TensorWrapper>>& output,
                                         std::vector<GrpsContext*>& ctxs) {
#if TRT_TENSOR_CONVERTER_DEBUG
  for (const auto& input : inputs) {
    std::string str;
    google::protobuf::TextFormat::PrintToString(*input, &str);
    LOG4(INFO, "trt tensor converter batch pre process, input: " << str);
  }
#endif

  if (inputs.size() != ctxs.size()) {
    LOG4(ERROR, "trt tensor converter batch pre process error, inputs size not match ctxs size.");
    throw ConverterException("trt tensor converter batch pre process error, inputs size not match ctxs size.");
  }

  // 1. Initialize batched tensors names, dtype and shape from all inputs.
  std::vector<std::string> tensor_names;
  std::vector<nvinfer1::DataType> tensor_dtypes;
  std::vector<nvinfer1::Dims> tensor_shapes;
  size_t total_batch_size = 0;
  for (int i = 0; i < inputs.size(); i++) {
    const auto& g_tensors = inputs[i]->gtensors();
    // Check size.
    if (g_tensors.tensors_size() == 0) {
      LOG4(ERROR, "trt tensor converter batch pre process error, some one gtensors tensors size is 0.");
      throw ConverterException("trt tensor converter batch pre process error, some one gtensors tensors size is 0.");
    }

    // Check names and shape of each tensor.
    bool has_name = !g_tensors.tensors(0).name().empty();
    size_t cur_batch_size = g_tensors.tensors(0).shape(0);
    for (const auto& g_tensor : g_tensors.tensors()) {
      if ((has_name && g_tensor.name().empty()) || (!has_name && !g_tensor.name().empty())) {
        LOG4(ERROR,
             "trt tensor converter batch pre process error, gtensors tensors should all have name or all have no "
             "name(will use default name).");
        throw ConverterException(
          "trt tensor converter pre batch process error, gtensors tensors should all have name or all have no name"
          "(will use default name).");
      }
      if (g_tensor.shape_size() <= 1) {
        LOG4(ERROR, "trt tensor converter batch pre process error, tensor shape size should be greater than 1.");
        throw ConverterException(
          "trt tensor converter batch pre process error, tensor shape size should be greater than 1.");
      }
      if (cur_batch_size != g_tensor.shape(0)) {
        LOG4(ERROR, "trt tensor converter batch pre process error, batch size of each tensor not match.");
        throw ConverterException("trt tensor converter batch pre process error, batch size of each tensor not match.");
      }
    }

    if (i == 0) { // Initialize tensors names, dtypes and shapes use first input.
      for (int j = 0; j < g_tensors.tensors_size(); ++j) {
        tensor_names.emplace_back(has_name ? g_tensors.tensors(j).name() : "");
        tensor_dtypes.emplace_back(GTensorType2TrtType(g_tensors.tensors(j).dtype()));
        nvinfer1::Dims shape{};
        for (int k = 0; k < g_tensors.tensors(j).shape_size(); ++k) {
          shape.nbDims++;
          shape.d[k] = int(g_tensors.tensors(j).shape(k));
        }
        tensor_shapes.emplace_back(shape);
      }
    } else { // Compare names, dtypes and shapes of follow inputs with first input.
      if (g_tensors.tensors_size() != tensor_names.size()) {
        LOG4(ERROR, "trt tensor converter batch pre process error, tensors size not match.");
        throw ConverterException("trt tensor converter batch pre process error, tensors size not match.");
      }

      for (int j = 0; j < g_tensors.tensors_size(); ++j) {
        // Check names.
        if (tensor_names[j] != (has_name ? g_tensors.tensors(j).name() : "")) {
          LOG4(ERROR, "trt tensor converter batch pre process error, tensor names not match.");
          throw ConverterException("trt tensor converter batch pre process error, tensor names not match.");
        }
        // Check dtypes.
        if (tensor_dtypes[j] != GTensorType2TrtType(g_tensors.tensors(j).dtype())) {
          LOG4(ERROR, "trt tensor converter batch pre process error, tensor dtypes not match.");
          throw ConverterException("trt tensor converter batch pre process error, tensor dtypes not match.");
        }
        // Check shapes[1:].
        if (tensor_shapes[j].nbDims != g_tensors.tensors(j).shape_size()) {
          LOG4(ERROR, "trt tensor converter batch pre process error, tensor shapes not match.");
          throw ConverterException("trt tensor converter batch pre process error, tensor shapes not match.");
        }
        for (int k = 1; k < g_tensors.tensors(j).shape_size(); ++k) {
          if (tensor_shapes[j].d[k] != g_tensors.tensors(j).shape(k)) {
            LOG4(ERROR, "trt tensor converter batch pre process error, tensor shapes not match.");
            throw ConverterException("trt tensor converter batch pre process error, tensor shapes not match.");
          }
        }
      }
    }
    total_batch_size += cur_batch_size;
  }
  for (size_t i = 0; i < tensor_names.size(); i++) {
    auto tensor_shape = tensor_shapes[i];
    tensor_shape.d[0] = int(total_batch_size);
    auto tensor = std::make_shared<TrtHostBinding>(tensor_names[i].c_str(), tensor_shape, tensor_dtypes[i], -1, -1);
    output.emplace_back(tensor_names[i], std::move(tensor));
#if TRT_TENSOR_CONVERTER_DEBUG
    std::string shape_str("[");
    for (int j = 0; j < tensor_shape.nbDims; ++j) {
      shape_str += std::to_string(tensor_shape.d[j]) + ",";
    }
    shape_str += "]";
    LOG4(INFO, "trt tensor converter batch pre process, tensor name: "
                 << tensor_names[i] << ", dtype: " << int(tensor_dtypes[i]) << ", shape: " << shape_str);
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
      GTensor2TrtTensor(g_tensors.tensors(j), tensor_names[j], *(output[j].second.trt_host_binding), tensor_size,
                        offsets[j]);
      offsets[j] += tensor_size;
    }

    ctxs[i]->SetUserData<long long>(g_tensors.tensors(0).shape(0));
  }

#if TRT_TENSOR_CONVERTER_DEBUG
  for (const auto& [name, tensor] : output) {
    LOG4(INFO, "trt tensor converter batch pre process, tensor name: " << name << ", tensor: "
                                                                       << tensor.trt_host_binding->DebugString());
  }
#endif
}

void TrtTensorConverter::BatchPostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                                          std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                          std::vector<GrpsContext*>& ctxs) {
#if TRT_TENSOR_CONVERTER_DEBUG
  for (const auto& [name, tensor] : input) {
    LOG4(INFO, "trt tensor converter batch post process, tensor name: " << name << ", tensor: "
                                                                        << tensor.trt_host_binding->DebugString());
  }
#endif

  if (outputs.size() != ctxs.size()) {
    LOG4(ERROR, "trt tensor converter batch post process error, outputs size not match ctxs size.");
    throw ConverterException("trt tensor converter batch post process error, outputs size not match ctxs size.");
  }

  std::vector<size_t> offsets(input.size(), 0);
  for (size_t i = 0; i < ctxs.size(); i++) {
    auto batch_size = ctxs[i]->GetUserData<long long>();
    for (size_t j = 0; j < input.size(); j++) {
      auto& [name, tensor_wrapper] = input[j];
      auto& tensor = *tensor_wrapper.trt_host_binding;
      auto tensor_size = tensor.volume() / tensor.dims().d[0] * batch_size;

      ::grps::protos::v1::GenericTensor g_tensor;
      // Set name.
      g_tensor.set_name(name);
      // Set shape.
      g_tensor.add_shape(batch_size);
      for (int k = 1; k < tensor.dims().nbDims; ++k) {
        g_tensor.add_shape(tensor.dims().d[k]);
      }
      // Set data.
      TrtTensor2GTensor(tensor, name, g_tensor, tensor_size, offsets[j]);
      *outputs[i]->mutable_gtensors()->add_tensors() = std::move(g_tensor);

      offsets[j] += tensor_size;
    }
  }

#if TRT_TENSOR_CONVERTER_DEBUG
  for (const auto& output : outputs) {
    std::string str;
    google::protobuf::TextFormat::PrintToString(*output, &str);
    LOG4(INFO, "trt tensor converter batch post process, output: " << str);
  }
#endif
}

} // namespace netease::grps