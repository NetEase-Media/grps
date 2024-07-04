/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  torch model inferer implementation.
 */

#include "torch_inferer.h"

#include <ATen/core/Tensor.h>
#include <dlfcn.h>
#include <torch/csrc/api/include/torch/version.h>

#include <regex>

#include "logger/logger.h"

namespace netease::grps {
void TorchModelInferer::Init(const std::string& path, const std::string& device, const YAML::Node& args) {
  ModelInferer::Init(path, device, args);
  if (device == "cpu") {
    torch_dev_ = torch::kCPU;
    inp_dev_ = torch::kCPU;
  } else if (device == "cuda" || device == "gpu") {
    // == cuda:0
    device_ = "cuda:0";
    torch_dev_ = torch::Device("cuda:0");
    inp_dev_ = torch::Device("cuda:0");
  } else if (std::regex_match(device, std::regex("^cuda:\\d+$"))) { // device: cuda:0, cuda:1, ...
    torch_dev_ = torch::Device(device);
    inp_dev_ = torch::Device(device);
  } else if (std::regex_match(device, std::regex("^gpu:\\d+$"))) {
    device_ = "cuda:" + device.substr(4);
    torch_dev_ = torch::Device("cuda:" + device.substr(4));
    inp_dev_ = torch::Device("cuda:" + device.substr(4));
  } else if (device.substr(0, 8) == "original") { // original device specified when exported model.
    device_ = "original";
    std::string inp_device = device.substr(9);
    if (inp_device != "cpu" && inp_device != "cuda" && inp_device != "gpu" &&
        !std::regex_match(inp_device, std::regex("^cuda:\\d+$")) &&
        !std::regex_match(inp_device, std::regex("^gpu:\\d+$"))) {
      LOG4(ERROR, "Invalid inp_device: " << inp_device << ", should be cpu, cuda, gpu, cuda:[num] or gpu:[num].");
      throw InfererException("Invalid inp_device: " + inp_device +
                             ", should be cpu, cuda, gpu, cuda:[num] or gpu:[num]");
    }
    if (std::regex_match(inp_device, std::regex("^gpu:\\d+$"))) {
      inp_device = "cuda:" + inp_device.substr(4);
    }
    inp_dev_ = torch::Device(inp_device);
  } else {
    LOG4(ERROR, "Init torch script model inferer failed, device: "
                  << device << " is not supported, should be cpu, cuda, gpu, cuda:[num], gpu:[num] or original.");
    throw InfererException("Device is not supported, device: " + device);
  }

  if (args && !args.IsNull() && args.IsMap()) {
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
}

void TorchModelInferer::Load() {
  try {
    if (device_ == "original") {
      module_ = torch::jit::load(path_);
    } else {
      module_ = torch::jit::load(path_, torch_dev_);
    }
    module_.eval();

    auto inputs = module_.get_method("forward").function().getSchema().arguments();
    for (int i = 0; i < inputs.size(); i++) {
      if (i == 0) {
        continue; // Skip self.
      }
      inp_tensor_names_.emplace_back(inputs[i].name());
    }
    if (inp_tensor_names_.empty()) {
      LOG4(ERROR, "Load torch script model inferer failed, no input tensor.");
      throw InfererException("No input tensor.");
    }

    auto outputs = module_.get_method("forward").function().getSchema().returns();
    for (const auto& output : outputs) {
      out_tensor_names_.emplace_back(output.name());
    }
    if (out_tensor_names_.empty()) {
      LOG4(ERROR, "Load torch script model inferer failed, no output tensor.");
      throw InfererException("No output tensor.");
    }
  } catch (const c10::Error& e) {
    LOG4(ERROR, "Load torch script model inferer failed, error: " << e.what());
    throw InfererException(e.what());
  }
  LOG4(INFO, "Load torch script model inferer success, model inferer path: "
               << path_ << ", device: " << device_ << ", " << "input tensor count: " << inp_tensor_names_.size()
               << ", input tensor names: " << inp_tensor_names_ << ", output tensor count: " << out_tensor_names_.size()
               << ", output tensor names: " << out_tensor_names_);
}

void TorchModelInferer::Infer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                              std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                              GrpsContext& context) {
#if TORCH_VERSION_MAJOR > 1 || (TORCH_VERSION_MAJOR == 1 && TORCH_VERSION_MINOR >= 9)
  c10::InferenceMode inference_mode_guard(infer_mode_);
#endif
  torch::NoGradGuard no_grad_guard;

  // Prepare input tensor.
  if (inputs.empty()) {
    throw InfererException("No input tensor.");
  }
  std::vector<torch::jit::IValue> input_tensor;
  for (int i = 0; i < inp_tensor_names_.size(); ++i) {
    if (i >= inputs.size()) {
      throw InfererException("Input tensor is not enough, required: " + std::to_string(inp_tensor_names_.size()) +
                             ", actual: " + std::to_string(inputs.size()));
    }
    if (inputs[i].first.empty()) { // Use default name and sequence.
      if (inputs[i].second.torch_tensor == nullptr) {
        throw InfererException("Input tensor is null, index: " + std::to_string(i));
      }
      input_tensor.emplace_back(inputs[i].second.torch_tensor->to(inp_dev_));
    } else { // Find by name.
      auto iter = std::find_if(inputs.begin(), inputs.end(),
                               [name = inp_tensor_names_[i]](const auto& item) { return item.first == name; });
      if (iter == inputs.end()) {
        throw InfererException("Input tensor is not found, name: " + inp_tensor_names_[i]);
      }
      if (iter->second.torch_tensor == nullptr) {
        throw InfererException("Input tensor is null, name: " + inp_tensor_names_[i]);
      }
      input_tensor.emplace_back(iter->second.torch_tensor->to(inp_dev_));
    }
  }
  try {
    outputs.clear();
    auto output = module_(input_tensor);
    if (output.isTensor()) {
      outputs.emplace_back(out_tensor_names_[0], std::move(output.toTensor()));
    } else if (output.isTuple()) {
      auto tuple = output.toTuple();
      for (size_t i = 0; i < tuple->elements().size(); ++i) {
        const std::string& name = (i < out_tensor_names_.size() && !out_tensor_names_[i].empty())
                                    ? out_tensor_names_[i]
                                    : "output_" + std::to_string(i);
        outputs.emplace_back(name, std::move(const_cast<c10::IValue&>(tuple->elements()[i]).toTensor()));
      }
    } else if (output.isTensorList()) {
      auto tensor_list = output.toTensorList();
      for (size_t i = 0; i < tensor_list.size(); ++i) {
        const std::string& name = (i < out_tensor_names_.size() && !out_tensor_names_[i].empty())
                                    ? out_tensor_names_[i]
                                    : "output_" + std::to_string(i);
        outputs.emplace_back(name, std::move(tensor_list.get(i)));
      }
    } else if (output.isGenericDict()) {
      auto dict = output.toGenericDict();
      for (const auto& item : dict) {
        outputs.emplace_back(item.key().toString()->string(), std::move(item.value().toTensor()));
      }
    } else {
      LOG4(ERROR,
           "Infer torch script model inferer failed, output type is not supported. output type: " << output.tagKind());
      throw InfererException("Infer torch script model inferer failed, output type is not supported. output type: " +
                             output.tagKind());
    }
  } catch (const c10::Error& e) {
    LOG4(ERROR, "Infer torch script model inferer failed, error: " << e.what());
    throw InfererException(e.what());
  } catch (const std::exception& e) {
    LOG4(ERROR, "Infer torch script model inferer failed, error: " << e.what());
    throw InfererException(e.what());
  }
}

void TorchModelInferer::BatchInfer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                   std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                   std::vector<GrpsContext*>& ctxs) {
  Infer(inputs, outputs, *ctxs[0]);
}

void TorchModelInferer::InferWithProfiler(const std::string& profiler_path,
                                          const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                          std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                          GrpsContext& context) {
  throw InfererException("Not supported now.");
}

void TorchModelInferer::BatchInferWithProfiler(const std::string& profiler_path,
                                               const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                               std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                               std::vector<GrpsContext*>& ctxs) {
  throw InfererException("Not supported now.");
}

// --------------------------------------- No converter mode [BEGIN] ---------------------------------------

void TorchModelInferer::Infer(const ::grps::protos::v1::GrpsMessage& inputs,
                              ::grps::protos::v1::GrpsMessage& outputs,
                              GrpsContext& context) {
  std::vector<std::pair<std::string, TensorWrapper>> input_tensors;
  std::vector<std::pair<std::string, TensorWrapper>> output_tensors;
  converter_.PreProcess(inputs, input_tensors, context);
  Infer(input_tensors, output_tensors, context);
  converter_.PostProcess(output_tensors, outputs, context);
}

void TorchModelInferer::InferWithProfiler(const std::string& profiler_path,
                                          const ::grps::protos::v1::GrpsMessage& inputs,
                                          ::grps::protos::v1::GrpsMessage& outputs,
                                          GrpsContext& context) {
  throw InfererException("Not supported now.");
}

void TorchModelInferer::BatchInfer(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                   std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                   std::vector<netease::grps::GrpsContext*>& ctxs) {
  std::vector<std::pair<std::string, TensorWrapper>> input_tensors;
  std::vector<std::pair<std::string, TensorWrapper>> output_tensors;
  converter_.BatchPreProcess(inputs, input_tensors, ctxs);
  BatchInfer(input_tensors, output_tensors, ctxs);
  converter_.BatchPostProcess(output_tensors, outputs, ctxs);
}

void TorchModelInferer::BatchInferWithProfiler(const std::string& profiler_path,
                                               std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                               std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                               std::vector<netease::grps::GrpsContext*>& ctxs) {
  throw InfererException("Not supported now.");
}

// --------------------------------------- No converter mode [END] ---------------------------------------

} // namespace netease::grps