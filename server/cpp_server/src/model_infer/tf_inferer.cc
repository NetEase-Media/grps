/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/04/26
 * Brief  tensorflow model inferer implementation.
 */

#include "tf_inferer.h"

#include <tensorflow/c/c_api.h>
#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/signature_constants.h>
#include <tensorflow/cc/saved_model/tag_constants.h>
#include <tensorflow/core/framework/tensor.h>

#include <regex>

#include "logger/logger.h"
#include "mem_manager/tf_gpu_mem_mgr.h"

namespace netease::grps {
TfModelInferer::TfModelInferer() : ModelInferer() {}

TfModelInferer::~TfModelInferer() = default;

void TfModelInferer::Init(const std::string& path, const std::string& device, const YAML::Node& args) {
  ModelInferer::Init(path, device, args);
  if (device == "cpu") {
    //
  } else if (device == "cuda" || device == "gpu") {
    // == cuda:0
    device_ = "gpu:0";
  } else if (std::regex_match(device, std::regex("^cuda:\\d+$"))) { // device: cuda:0, cuda:1, ...
    // change to gpu:[num]
    device_ = "gpu:" + device.substr(5);
  } else if (std::regex_match(device, std::regex("^gpu:\\d+$"))) {
    //
  } else if (device == "original") { // original device specified when exported model.
    //
  } else {
    LOG4(ERROR, "Init tensorflow model inferer failed, device: "
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
            TF_Status* status = TF_NewStatus();
            TF_LoadLibrary(path_str.c_str(), status);
            TF_Code code = TF_GetCode(status);
            std::string status_msg(TF_Message(status));
            TF_DeleteStatus(status);
            if (TF_OK != code) {
              std::string err_str = "Load customized op lib failed, path: ";
              err_str.append(path_str).append(", error: ").append(status_msg);
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

void TfModelInferer::Load() {
  auto session_options = tensorflow::SessionOptions();
  auto run_options = tensorflow::RunOptions();

  if (device_ == "cpu") {
    session_options.config.mutable_device_count()->insert({"GPU", 0});
  } else {
    int device_idx = 0;
    if (device_ != "original") {
      session_options.config.mutable_device_count()->insert({"GPU", 1});
    }
    if (std::regex_match(device_, std::regex("^gpu:\\d+$"))) {
      session_options.config.mutable_gpu_options()->set_visible_device_list(device_.substr(4));
      device_idx = std::stoi(device_.substr(4));
    }
    session_options.config.set_allow_soft_placement(true);
    session_options.config.mutable_gpu_options()->set_allow_growth(true);
    float mem_limit_rate = TfGpuMemMgr::GetMemLimitRate(device_idx);
    session_options.config.mutable_gpu_options()->set_per_process_gpu_memory_fraction(mem_limit_rate);
    if (mem_limit_rate != 1.0) {
      LOG4(INFO, "Tf model gpu memory has been limited to rate: " << TfGpuMemMgr::GetMemLimitRate(device_idx));
    }
  }

  bundle_ = std::make_unique<tensorflow::SavedModelBundle>();

  // Only support default 'serve' tag and default 'serving_default' signature.
  std::unordered_set<std::string> saved_model_tags{tensorflow::kSavedModelTagServe};
  std::string signature_def_key = tensorflow::kDefaultServingSignatureDefKey;

  // Load model and parse signature.
  try {
    tensorflow::Status status =
      tensorflow::LoadSavedModel(session_options, run_options, path_, saved_model_tags, bundle_.get());
    if (!status.ok()) {
      LOG4(ERROR, "Failed to load model from " << path_ << ", error: " << status.error_message());
      throw InfererException("Failed to load model from " + path_ + ", error: " + status.error_message());
    }

    const auto& signature_def = bundle_->meta_graph_def.signature_def();
    if (signature_def.empty()) {
      LOG4(ERROR, "Failed to load model from " << path_ << ", error: signature_def is empty");
      throw InfererException("Failed to load model from " + path_ + ", error: signature_def is empty");
    }
    if (signature_def.find(signature_def_key) == signature_def.end()) {
      LOG4(ERROR, "Failed to load model from " << path_ << ", error: signature_def_key 'serving_default' not found");
      throw InfererException("Failed to load model from " + path_ +
                             ", error: signature_def_key 'serving_default' not found");
    }
    const auto& signature = signature_def.at(signature_def_key);
    const auto& inputs = signature.inputs();
    const auto& outputs = signature.outputs();
    if (inputs.empty() || outputs.empty()) {
      LOG4(ERROR, "Failed to load model from " << path_ << ", error: inputs or outputs is empty");
      throw InfererException("Failed to load model from " + path_ + ", error: inputs or outputs is empty");
    }

    for (const auto& input : inputs) {
      inp_name_to_tensor_name_.emplace(input.first, input.second.name());
      inp_tensor_names_.emplace_back(input.second.name());
      LOG4(INFO, "Add input tensor: " << input.first << " -> " << input.second.name());
    }
    for (const auto& output : outputs) {
      tensor_name_to_out_name_.emplace(output.second.name(), output.first);
      out_tensor_names_.emplace_back(output.second.name());
      LOG4(INFO, "Add output tensor: " << output.first << " -> " << output.second.name());
    }
  } catch (const google::protobuf::FatalException& e) {
    LOG4(ERROR, "Failed to load model from " << path_ << ", error: " << e.what());
    throw InfererException("Failed to load model from " + path_ + ", error: " + e.what());
  } catch (const std::exception& e) {
    LOG4(ERROR, "Failed to load model from " << path_ << ", error: " << e.what());
    throw InfererException("Failed to load model from " + path_ + ", error: " + e.what());
  } catch (...) {
    LOG4(ERROR, "Failed to load model from " << path_);
    throw InfererException("Failed to load model from " + path_ + ", error: unknown");
  }

  LOG4(INFO, "Load tensorflow script model inferer success, model inferer path: " << path_ << ", device: " << device_);
}

void TfModelInferer::Infer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                           std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                           GrpsContext& context) {
  std::vector<std::pair<std::string, tensorflow::Tensor>> tf_inputs;
  std::vector<tensorflow::Tensor> tf_outputs;

  // Convert input name to tensor name.
  int idx = 0;
  for (const auto& [name, tensor_wrapper] : inputs) {
    if (!name.empty() && inp_name_to_tensor_name_.find(name) == inp_name_to_tensor_name_.end()) {
      LOG4(ERROR, "Failed to infer model, error: input name " << name << " not defined in model");
      throw InfererException("Failed to infer model, error: input name " + name + " not defined in model");
    }
    if (tensor_wrapper.tf_tensor == nullptr) {
      LOG4(ERROR, "Failed to infer model, error: input tensor " << name << " is null");
      throw InfererException("Failed to infer model, error: input tensor " + name + " is null");
    }

    if (idx >= inp_tensor_names_.size()) {
      std::stringstream err;
      err << "Failed to infer model, error: input count not match, input count: " << inputs.size()
          << ", model input count: " << inp_tensor_names_.size();
      LOG4(ERROR, err.str());
      throw InfererException(err.str());
    }

    tf_inputs.emplace_back(name.empty() ? inp_tensor_names_[idx] : inp_name_to_tensor_name_.at(name),
                           std::move(*tensor_wrapper.tf_tensor));
    idx++;
  }

  // Session run and convert output tensor to tensor wrapper.
  try {
    tensorflow::RunMetadata run_metadata;
    auto run_options = tensorflow::RunOptions();
    tensorflow::Status status =
      bundle_->session->Run(run_options, tf_inputs, out_tensor_names_, {}, &tf_outputs, &run_metadata);
    if (!status.ok()) {
      LOG4(ERROR, "Failed to infer model, error: " << status.error_message());
      throw InfererException("Failed to infer model, error: " + status.error_message());
    }

    if (tf_outputs.size() != out_tensor_names_.size()) {
      LOG4(ERROR, "Failed to infer model, error: output size not match");
      throw InfererException("Failed to infer model, error: output size not match");
    }

    outputs.clear();
    for (int i = 0; i < tf_outputs.size(); ++i) {
      if (i >= out_tensor_names_.size()) {
        std::stringstream ss;
        ss << "Failed to infer model, output count not match, output count: " << tf_outputs.size()
           << ", model output count: " << out_tensor_names_.size();
        LOG4(ERROR, ss.str());
        throw InfererException(ss.str());
      }
      outputs.emplace_back(tensor_name_to_out_name_.at(out_tensor_names_[i]), std::move(tf_outputs[i]));
    }
  } catch (const google::protobuf::FatalException& e) {
    LOG4(ERROR, "Failed to infer model, error: " << e.what());
    throw InfererException("Failed to infer model, error: " + std::string(e.what()));
  } catch (const std::exception& e) {
    LOG4(ERROR, "Failed to infer model, error: " << e.what());
    throw InfererException("Failed to infer model, error: " + std::string(e.what()));
  } catch (...) {
    LOG4(ERROR, "Failed to infer model");
    throw InfererException("Failed to infer model, error: unknown");
  }
}

void TfModelInferer::BatchInfer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                std::vector<GrpsContext*>& ctxs) {
  Infer(inputs, outputs, *ctxs[0]);
}

void TfModelInferer::InferWithProfiler(const std::string& profiler_path,
                                       const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                       std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                       GrpsContext& context) {
  throw InfererException("Not support now.");
}

void TfModelInferer::BatchInferWithProfiler(const std::string& profiler_path,
                                            const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                            std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                            std::vector<GrpsContext*>& ctxs) {
  throw InfererException("Not support now.");
}

// --------------------------------------- No converter mode [BEGIN] ---------------------------------------

void TfModelInferer::Infer(const ::grps::protos::v1::GrpsMessage& inputs,
                           ::grps::protos::v1::GrpsMessage& outputs,
                           GrpsContext& context) {
  std::vector<std::pair<std::string, TensorWrapper>> input_tensors;
  std::vector<std::pair<std::string, TensorWrapper>> output_tensors;
  tf_converter_.PreProcess(inputs, input_tensors, context);
  Infer(input_tensors, output_tensors, context);
  tf_converter_.PostProcess(output_tensors, outputs, context);
}

void TfModelInferer::InferWithProfiler(const std::string& profiler_path,
                                       const ::grps::protos::v1::GrpsMessage& inputs,
                                       ::grps::protos::v1::GrpsMessage& outputs,
                                       GrpsContext& context) {
  throw InfererException("Not support now.");
}

void TfModelInferer::BatchInfer(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                std::vector<netease::grps::GrpsContext*>& ctxs) {
  std::vector<std::pair<std::string, TensorWrapper>> input_tensors;
  std::vector<std::pair<std::string, TensorWrapper>> output_tensors;
  tf_converter_.BatchPreProcess(inputs, input_tensors, ctxs);
  BatchInfer(input_tensors, output_tensors, ctxs);
  tf_converter_.BatchPostProcess(output_tensors, outputs, ctxs);
}

void TfModelInferer::BatchInferWithProfiler(const std::string& profiler_path,
                                            std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                            std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                            std::vector<netease::grps::GrpsContext*>& ctxs) {
  throw InfererException("Not support now.");
}

// --------------------------------------- No converter mode [END] ---------------------------------------

} // namespace netease::grps