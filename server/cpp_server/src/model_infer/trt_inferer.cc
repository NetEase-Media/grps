/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2024/06/04
 * Brief  TensorRT model inferer implementation.
 */

#include "trt_inferer.h"

#include <NvInferRuntime.h>
#include <dlfcn.h>

#include <fstream>
#include <regex>

#include "logger/logger.h"

#define TRT_INFERER_DEBUG 0

namespace netease::grps {
class TrtLogger : public nvinfer1::ILogger {
public:
  void log(Severity severity, const char* msg) noexcept override {
    switch (severity) {
      case Severity::kINTERNAL_ERROR:
        LOG4(ERROR, "[TRT-IE] " << msg);
        break;
      case Severity::kERROR:
        LOG4(ERROR, "[TRT-E] " << msg);
        break;
      case Severity::kWARNING:
        LOG4(WARN, "[TRT-W] " << msg);
        break;
      case Severity::kINFO:
        LOG4(INFO, "[TRT-I] " << msg);
        break;
      case Severity::kVERBOSE:
        // LOG4(INFO, "[TRT-V] " << msg);
        break;
    }
  }
};

static TrtLogger g_trt_logger;

void TrtModelInferer::TrtDeviceBinding::Allocate(const nvinfer1::Dims& true_dims) {
  true_dims_ = true_dims;
  volume_ = Volume(true_dims, vec_dim_, comps_);
  size_t new_buffer_size = volume_ * data_type_size_;
  if (new_buffer_size > buffer_capacity_) {
    buffer_.Allocate(new_buffer_size);
    buffer_capacity_ = new_buffer_size;
  }
  buffer_size_ = new_buffer_size;
}

void TrtModelInferer::TrtDeviceBinding::FromHost(TrtHostBinding& host_binding, CudaStream& stream) {
  // Check dims.
  if (dims_.nbDims != host_binding.dims().nbDims) {
    auto err_msg = std::string("Dims not match, binding: ") + name_ + ", dims: " + std::to_string(dims_.nbDims) +
                   ", host dims: " + std::to_string(host_binding.dims().nbDims);
    LOG4(ERROR, err_msg);
    throw InfererException(err_msg);
  }
  for (int i = 0; i < dims_.nbDims; ++i) {
    if (dims_.d[i] != -1 && dims_.d[i] != host_binding.dims().d[i]) {
      auto err_msg = std::string("Dims not match, binding: ") + name_ + ", binding dim[" + std::to_string(i) +
                     "]: " + std::to_string(dims_.d[i]) + ", host dim[" + std::to_string(i) +
                     "]: " + std::to_string(host_binding.dims().d[i]);
      LOG4(ERROR, err_msg);
      throw InfererException(err_msg);
    } else if (host_binding.dims().d[i] > max_dims_.d[i]) {
      auto err_msg = std::string("Dims not match, binding: ") + name_ + ", host dim[" + std::to_string(i) +
                     "]: " + std::to_string(host_binding.dims().d[i]) + ", binding max dim[" + std::to_string(i) +
                     "]: " + std::to_string(max_dims_.d[i]);
      LOG4(ERROR, err_msg);
      throw InfererException(err_msg);
    }
  }
  // Check dtype.
  if (data_type_ != host_binding.data_type()) {
    auto err_msg = std::string("Data type not match, binding: ") + name_ +
                   ", binding dtype: " + std::to_string(static_cast<int>(data_type_)) +
                   ", host dtype: " + std::to_string(static_cast<int>(host_binding.data_type()));
    LOG4(ERROR, err_msg);
    throw InfererException(err_msg);
  }

  // Allocate buffer if not enough.
  Allocate(host_binding.dims());
  if (host_binding.buffer_size() != buffer_size()) {
    auto err_msg = std::string("Dims not match, binding: ") + name_ +
                   ", buffer size: " + std::to_string(buffer_size()) +
                   ", host buffer size: " + std::to_string(host_binding.buffer_size());
    LOG4(ERROR, err_msg);
    throw InfererException(err_msg);
  }

  // Copy data from host.
  H2D(stream, host_binding.buffer(), buffer(), host_binding.buffer_size());
}

void TrtModelInferer::TrtDeviceBinding::ToHost(CudaStream& stream, TrtHostBinding& host_binding) {
  D2H(stream, buffer(), host_binding.buffer(), buffer_size());
}

TrtModelInferer::TrtModelInferer() : ModelInferer() {}
TrtModelInferer::~TrtModelInferer() = default;

void TrtModelInferer::Init(const std::string& path, const std::string& device, const YAML::Node& args) {
  ModelInferer::Init(path, device, args);

  if (device == "cuda" || device == "gpu") {
    // == cuda:0
    device_id_ = 0;
  } else if (std::regex_match(device, std::regex("^cuda:\\d+$"))) { // device: cuda:0, cuda:1, ...
    // change to gpu:[num]
    device_id_ = std::stoi(device.substr(5));
  } else if (std::regex_match(device, std::regex("^gpu:\\d+$"))) {
    device_id_ = std::stoi(device.substr(4));
  } else if (device == "original") { // original device specified when exported model.
    device_id_ = 0;
  } else {
    LOG4(ERROR, "Init tensorrt model inferer failed, device: "
                  << device << " is not supported, should be cpu, cuda, gpu, cuda:[num], gpu:[num] or original.");
    throw InfererException("Device is not supported, device: " + device);
  }

  // Set device.
  CudaCheck(cudaSetDevice(device_id_));

  if (args && !args.IsNull() && args.IsMap()) {
    if (args["dla_cores"] && args["dla_cores"].IsScalar()) {
      dla_cores_ = args["dla_cores"].as<int>();
      if (dla_cores_ < 0) {
        LOG4(ERROR, "Init tensorrt model inferer failed, dla_cores: " << dla_cores_ << " should be >= 0.");
        throw InfererException("dla_cores should be >= 0.");
      }
    }
    if (args["streams"] && args["streams"].IsScalar()) {
      streams_ = args["streams"].as<int>();
      if (streams_ <= 0) {
        LOG4(ERROR, "Init tensorrt model inferer failed, streams: " << streams_ << " should be > 0.");
        throw InfererException("streams should be > 0.");
      }
    }

    // Load customized op lib if exists.
    auto customized_op_paths = args["customized_op_paths"];
    if (customized_op_paths && !customized_op_paths.IsNull() && customized_op_paths.IsSequence()) {
      for (const auto& op_path : customized_op_paths) {
        if (op_path && !op_path.IsNull() && op_path.IsScalar()) {
          auto path_str = op_path.as<std::string>();
          if (!path_str.empty()) {
            auto* handler = dlopen(path_str.c_str(), RTLD_LAZY);
            if (handler == nullptr) {
              std::string err_str = "Load customized op lib failed, path: ";
              err_str.append(path_str).append(", error: ").append(dlerror());
              LOG4(ERROR, err_str);
              throw InfererException(err_str);
            }
            LOG4(INFO, "Load customized op lib success, path: " << path_str);
          }
        }
      }
    }
  }
  cur_stream_ = 0;
}

void TrtModelInferer::Load() {
  // Load engine file.
  std::ifstream engine_file(path_, std::ios::binary);
  if (!engine_file) {
    LOG4(ERROR, "Error loading engine file: " << path_);
    throw InfererException("Error loading engine file: " + path_);
  }
  engine_file.seekg(0, std::ifstream::end);
  size_t fsize = static_cast<size_t>(engine_file.tellg());
  engine_file.seekg(0, std::ifstream::beg);
  std::vector<char> engine_data(fsize);
  engine_file.read(engine_data.data(), (long long)(fsize));
  if (!engine_file) {
    LOG4(ERROR, "Error loading engine file: " << path_);
    throw InfererException("Error loading engine file: " + path_);
  }

  // Create runtime.
  runtime_ = TrtUniquePtr<nvinfer1::IRuntime>{nvinfer1::createInferRuntime(g_trt_logger)};
  if (dla_cores_ != -1) {
    runtime_->setDLACore(dla_cores_);
  }

  // Load all instances.
  for (int i = 0; i < streams_; i++) {
    auto instance = std::make_unique<Instance>();
    // Deserialize engine.
    instance->engine_ =
      TrtUniquePtr<nvinfer1::ICudaEngine>{runtime_->deserializeCudaEngine(engine_data.data(), fsize, nullptr)};
    if (!instance->engine_) {
      LOG4(ERROR, "Error deserializing engine file: " << path_);
      throw InfererException("Error deserializing engine file: " + path_);
    }

    // Load bindings.
    for (int j = 0; j < instance->engine_->getNbBindings(); ++j) {
      auto dims = instance->engine_->getBindingDimensions(j);
      auto max_dims = instance->engine_->bindingIsInput(j)
                        ? instance->engine_->getProfileDimensions(j, 0, nvinfer1::OptProfileSelector::kMAX)
                        : dims;
      auto name = instance->engine_->getBindingName(j);
      auto dtype = instance->engine_->getBindingDataType(j);
      auto vec_dim = instance->engine_->getBindingVectorizedDim(j);
      auto comps = instance->engine_->getBindingComponentsPerElement(j);
      auto is_input_binding = instance->engine_->bindingIsInput(j);
      auto is_shape_binding = instance->engine_->isShapeBinding(j);
      instance->bindings_.emplace_back(name, dims, max_dims, dtype, vec_dim, comps, is_input_binding, is_shape_binding);
      LOG4(INFO, "Trt instance(" << i << ") add binding: " << instance->bindings_.back().DebugString());
    }

    // Create context.
    instance->trt_context_ = TrtUniquePtr<nvinfer1::IExecutionContext>{instance->engine_->createExecutionContext()};
    instances_.emplace_back(std::move(instance));
  }
}

void TrtModelInferer::Infer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                            std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                            netease::grps::GrpsContext& ctx) {
  int idx = cur_stream_.load();
  while (!cur_stream_.compare_exchange_weak(idx, (idx + 1) % streams_)) {
    idx = cur_stream_.load();
  }
  auto& instance = instances_[idx];
  std::unique_lock<std::mutex> input_free_lock(instance->mutex_);

  CudaCheck(cudaSetDevice(device_id_));

#if TRT_INFERER_DEBUG
  auto& inp_start_event = instance->multi_event_[static_cast<int>(EventType::kINPUT_START)];
  auto& inp_end_event = instance->multi_event_[static_cast<int>(EventType::kINPUT_END)];
  auto& compute_start_event = instance->multi_event_[static_cast<int>(EventType::kCOMPUTE_START)];
  auto& compute_end_event = instance->multi_event_[static_cast<int>(EventType::kCOMPUTE_END)];
  auto& out_start_event = instance->multi_event_[static_cast<int>(EventType::kOUTPUT_START)];
  auto& out_end_event = instance->multi_event_[static_cast<int>(EventType::kOUTPUT_END)];
#endif

  // 1. Prepare bindings.
#if TRT_INFERER_DEBUG
  inp_start_event.Record(instance->stream_);
#endif
  std::vector<void*> dev_ptrs;
  for (int i = 0; i < instance->bindings_.size(); i++) {
    auto& binding = instance->bindings_[i];
    if (binding.is_input_binding()) {
      auto it = inputs.end();
      if (inputs[i].first.empty()) { // Use default name and sequence.
        if (i >= inputs.size()) {
          auto err_msg = std::string("Input not found, binding: ") + binding.name();
          LOG4(ERROR, err_msg);
          throw InfererException(err_msg);
        }
        it = inputs.begin() + i;
      } else { // Find by name.
        it = std::find_if(inputs.begin(), inputs.end(),
                          [&binding](const auto& input) { return input.first == binding.name(); });
        if (it == inputs.end()) {
          auto err_msg = std::string("Input not found, binding: ") + binding.name();
          LOG4(ERROR, err_msg);
          throw InfererException(err_msg);
        }
      }
      auto& host_binding = *(it->second.trt_host_binding);

      if (binding.is_shape_binding()) { // Set shape binding.
        if (host_binding.data_type() != nvinfer1::DataType::kINT32) {
          auto err_msg = std::string("Shape binding should be int32, binding idx: ") + std::to_string(i) +
                         ", binding name: " + binding.name();
          LOG4(ERROR, err_msg);
          throw InfererException(err_msg);
        }
        if (binding.dims().nbDims > 1) {
          auto err_msg = std::string("Shape binding should be scalar or 1D vector, binding idx: ") + std::to_string(i) +
                         ", binding name: " + binding.name();
          LOG4(ERROR, err_msg);
          throw InfererException(err_msg);
        } else if (binding.dims().nbDims == 0 && host_binding.volume() != 1) {
          auto err_msg = std::stringstream();
          err_msg << "Shape binding should be scalar, but got input size is " << host_binding.volume()
                  << ", binding idx: " << i << ", binding name: " << binding.name();
          LOG4(ERROR, err_msg.str());
          throw InfererException(err_msg.str());
        } else if (binding.dims().nbDims == 1 && binding.dims().d[0] != host_binding.volume()) {
          auto err_msg = std::stringstream();
          err_msg << "Shape binding should be 1D vector with size " << binding.dims().d[0] << ", but got input size is "
                  << host_binding.volume() << ", binding idx: " << i << ", binding name: " << binding.name();
          LOG4(ERROR, err_msg.str());
          throw InfererException(err_msg.str());
        }
        auto* shape = reinterpret_cast<int32_t*>(host_binding.buffer().Get());
        instance->trt_context_->setInputShapeBinding(i, shape);
      }

      binding.FromHost(host_binding, instance->stream_);
      instance->trt_context_->setBindingDimensions(i, binding.true_dims());
    } else {
      auto true_dims = instance->trt_context_->getBindingDimensions(i);
      binding.Allocate(true_dims);
    }
    dev_ptrs.push_back(binding.buffer().Get());
  }
#if TRT_INFERER_DEBUG
  inp_end_event.Record(instance->stream_);
#endif

  // 2. Execute.
#if TRT_INFERER_DEBUG
  compute_start_event.Record(instance->stream_);
#endif
  instance->trt_context_->enqueueV2(dev_ptrs.data(), instance->stream_.Get(), nullptr);
#if TRT_INFERER_DEBUG
  compute_end_event.Record(instance->stream_);
#endif

  // 3. Output to host.
#if TRT_INFERER_DEBUG
  out_start_event.Record(instance->stream_);
#endif
  for (auto& binding : instance->bindings_) {
    if (!binding.is_input_binding()) {
      outputs.emplace_back(binding.name(), TrtHostBinding(binding.name(), binding.true_dims(), binding.data_type(),
                                                          binding.vec_dim(), binding.comps()));
      binding.ToHost(instance->stream_, *outputs.back().second.trt_host_binding);
    }
  }
#if TRT_INFERER_DEBUG
  out_end_event.Record(instance->stream_);
#endif

  cudaStreamSynchronize(instance->stream_.Get());

#if TRT_INFERER_DEBUG
  LOG4(INFO, "Trt instance: " << idx << ", H2D: " << (inp_end_event - inp_start_event) << " ms, "
                              << "Compute: " << (compute_end_event - compute_start_event) << " ms, "
                              << "D2H: " << (out_end_event - out_start_event) << " ms.");
#endif
}

void TrtModelInferer::BatchInfer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                 std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                 std::vector<GrpsContext*>& ctxs) {
  Infer(inputs, outputs, *ctxs[0]);
}

void TrtModelInferer::InferWithProfiler(const std::string& profiler_path,
                                        const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                        std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                        GrpsContext& context) {
  throw InfererException("Not support now.");
}

void TrtModelInferer::BatchInferWithProfiler(const std::string& profiler_path,
                                             const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                             std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                             std::vector<GrpsContext*>& ctxs) {
  throw InfererException("Not support now.");
}

// --------------------------------------- No converter mode [BEGIN] ---------------------------------------

void TrtModelInferer::Infer(const ::grps::protos::v1::GrpsMessage& inputs,
                            ::grps::protos::v1::GrpsMessage& outputs,
                            GrpsContext& context) {
  std::vector<std::pair<std::string, TensorWrapper>> input_tensors;
  std::vector<std::pair<std::string, TensorWrapper>> output_tensors;
  trt_converter_.PreProcess(inputs, input_tensors, context);
  Infer(input_tensors, output_tensors, context);
  trt_converter_.PostProcess(output_tensors, outputs, context);
}

void TrtModelInferer::InferWithProfiler(const std::string& profiler_path,
                                        const ::grps::protos::v1::GrpsMessage& inputs,
                                        ::grps::protos::v1::GrpsMessage& outputs,
                                        GrpsContext& context) {
  throw InfererException("Not support now.");
}

void TrtModelInferer::BatchInfer(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                 std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                 std::vector<netease::grps::GrpsContext*>& ctxs) {
  std::vector<std::pair<std::string, TensorWrapper>> input_tensors;
  std::vector<std::pair<std::string, TensorWrapper>> output_tensors;
  trt_converter_.BatchPreProcess(inputs, input_tensors, ctxs);
  BatchInfer(input_tensors, output_tensors, ctxs);
  trt_converter_.BatchPostProcess(output_tensors, outputs, ctxs);
}

void TrtModelInferer::BatchInferWithProfiler(const std::string& profiler_path,
                                             std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                             std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                             std::vector<netease::grps::GrpsContext*>& ctxs) {
  throw InfererException("Not support now.");
}

// --------------------------------------- No converter mode [END] ---------------------------------------

} // namespace netease::grps