/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  Executor to run complete infer, including outlier, converter, dag, model infer and etc.
 */

#include "executor.h"

#include <google/protobuf/text_format.h>

#include "converter/converter.h"
#include "logger/logger.h"
#include "model_infer/inferer.h"
#ifdef GRPS_TF_ENABLE
#include "converter/tf_tensor_converter.h"
#include "model_infer/tf_inferer.h"
#endif
#ifdef GRPS_TORCH_ENABLE
#include "converter/torch_tensor_converter.h"
#include "model_infer/torch_inferer.h"
#endif
#ifdef GRPS_TRT_ENABLE
#include "converter/trt_tensor_converter.h"
#include "model_infer/trt_inferer.h"
#endif
#include "batching/batcher.h"
#include "config/global_config.h"

namespace netease::grps {
void Executor::Init() {
  LOG4(INFO, "Init executor.");
  InitOutlier();
  InitModels();
  InitDag();
}

void Executor::InitOutlier() {
  // TODO: Add outlier.
}

void Executor::InitModels() {
  LOG4(INFO, "Init models.");
  const auto& inference_config = GlobalConfig::Instance().inference_config();
  if (!inference_config._is_set.models) {
    LOG4(ERROR, "No model config.");
    throw ExecutorException("No model config.");
  }

  std::unordered_set<std::string> used_customized_inferers;
  std::unordered_set<std::string> used_customized_converters;
  for (const auto& [name, model] : inference_config.models) {
    if (models_.find(name) != models_.end()) {
      LOG4(ERROR, "Model " << name << " already exists.");
      throw ExecutorException("Model " + name + " already exists.");
    }

    std::shared_ptr<ModelInferer> inferer_ptr = nullptr;
    std::string inferer_name;
    if (model.inferer_type == "torch") {
#ifdef GRPS_TORCH_ENABLE
      inferer_ptr = std::make_shared<TorchModelInferer>();
#else
      throw ExecutorException("Torch model inferer is not supported.");
#endif
      inferer_name = "torch";
    } else if (model.inferer_type == "tensorflow") {
#ifdef GRPS_TF_ENABLE
      inferer_ptr = std::make_shared<TfModelInferer>();
#else
      throw ExecutorException("Tensorflow model inferer is not supported.");
#endif
      inferer_name = "tensorflow";
    } else if (model.inferer_type == "tensorrt") {
#ifdef GRPS_TRT_ENABLE
      inferer_ptr = std::make_shared<TrtModelInferer>();
#else
      throw ExecutorException("TensorRT model inferer is not supported.");
#endif
    } else if (model.inferer_type == "customized") {
      auto customized_inferer = ModelInfererRegistry::Instance().GetInferer(model.inferer_name);
      if (customized_inferer == nullptr) {
        LOG4(ERROR, "Not found customized inferer: " << model.inferer_name);
        throw ExecutorException("Not found customized inferer: " + model.inferer_name);
      }
      inferer_name = model.inferer_name;
      if (used_customized_inferers.find(inferer_name) != used_customized_inferers.end()) {
        // If customized inferer has been used, clone new.
        inferer_ptr = std::shared_ptr<ModelInferer>(customized_inferer->Clone());
      } else {
        inferer_ptr = customized_inferer;
        used_customized_inferers.insert(inferer_name);
      }
    } else {
      LOG4(ERROR, "Not support inferer type: " << model.inferer_type);
      throw ExecutorException("Not support inferer type: " + model.inferer_type);
    }

    std::string device;
    std::transform(model.device.begin(), model.device.end(), std::back_inserter(device), ::tolower);
    if (device == "original" && model.inferer_type == "torch") {
      device += "_" + model.inp_device;
    }
    std::transform(device.begin(), device.end(), device.begin(), ::tolower);

    inferer_ptr->Init(model.inferer_path, device, model.inferer_args);
    LOG4(INFO, "Init inferer: " << inferer_name << " successfully, path: " << model.inferer_path
                                << ", device: " << device << ", args: " << model.inferer_args << ".");
    inferer_ptr->Load();
    LOG4(INFO, "Load inferer: " << inferer_name << " successfully.");

    std::shared_ptr<Converter> converter_ptr = nullptr;
    std::string converter_name;
    if (model.converter_type == "torch") {
#ifdef GRPS_TORCH_ENABLE
      converter_ptr = std::make_shared<TorchTensorConverter>();
#else
      throw ExecutorException("Torch tensor converter is not supported.");
#endif
      converter_name = "torch";
    } else if (model.converter_type == "tensorflow") {
#ifdef GRPS_TF_ENABLE
      converter_ptr = std::make_shared<TfTensorConverter>();
#else
      throw ExecutorException("Tensorflow tensor converter is not supported.");
#endif
      converter_name = "tensorflow";
    } else if (model.converter_type == "tensorrt") {
#ifdef GRPS_TRT_ENABLE
      converter_ptr = std::make_shared<TrtTensorConverter>();
#else
      throw ExecutorException("TensorRT tensor converter is not supported.");
#endif
    } else if (model.converter_type == "none") {
      converter_ptr = nullptr;
      converter_name = "none";
    } else if (model.converter_type == "customized") {
      auto customized_converter = ConverterRegistry::Instance().GetConverter(model.converter_name);
      if (customized_converter == nullptr) {
        LOG4(ERROR, "Not found customized converter: " << model.converter_name);
        throw ExecutorException("Not found customized converter: " + model.converter_name);
      }
      converter_name = model.converter_name;
      if (used_customized_converters.find(converter_name) != used_customized_converters.end()) {
        // If customized converter has been used, clone new.
        converter_ptr = std::shared_ptr<Converter>(customized_converter->Clone());
      } else {
        converter_ptr = customized_converter;
        used_customized_converters.insert(converter_name);
      }
    } else {
      LOG4(ERROR, "Not support converter type: " << model.converter_type);
      throw ExecutorException("Not support converter type: " + model.converter_type);
    }

    if (converter_ptr != nullptr) {
      converter_ptr->Init(model.converter_path, model.converter_args);
      LOG4(INFO, "Init converter: " << converter_name << " successfully, path: " << model.converter_path
                                    << ", args: " << model.converter_args << ".");
    }

    std::shared_ptr<Batcher> batcher_ptr = nullptr;
    std::string batcher_name;
    if (model.batching.type == "none") {
      batcher_name = "none";
    } else if (model.batching.type == "dynamic") {
      batcher_ptr = std::make_shared<DynamicBatcher>();
      batcher_name = "dynamic";
    } else {
      LOG4(ERROR, "Not support batching type: " << model.batching.type);
      throw ExecutorException("Not support batching type: " + model.batching.type);
    }

    if (batcher_ptr != nullptr) {
      batcher_ptr->Init(name, model.batching.max_batch_size, model.batching.batch_timeout_us, converter_ptr.get(),
                        inferer_ptr.get());
      batcher_ptr->Start();
      LOG4(INFO, "Init and start batcher: " << batcher_name
                                            << " successfully, max_batch_size: " << model.batching.max_batch_size
                                            << ", batch_timeout_us: " << model.batching.batch_timeout_us);
    }

    models_[name] = Model(model.name, model.version, converter_ptr, inferer_ptr, batcher_ptr);
    LOG4(INFO, "Init model: " << name << " successfully, inferer: " << inferer_name << ", converter: " << converter_name
                              << ", batcher: " << batcher_name << ", version: " << model.version);
  }
}

void Executor::InitDag() {
  LOG4(INFO, "Init dag.");
  const auto inference_config = GlobalConfig::Instance().inference_config();
  if (!inference_config._is_set.dag) {
    LOG4(ERROR, "No dag config.");
    throw ExecutorException("No dag config.");
  }

  const auto dag_config = inference_config.dag;
  if (dag_config.type == "sequential") {
    dag_ = std::make_shared<SequentialDag>(dag_config.name);
    dag_->BuildDag(dag_config.nodes, models_);
  } else {
    LOG4(ERROR, "Not support dag type: " << dag_config.type);
    throw ExecutorException("Not support dag type: " + dag_config.type);
  }
}

void Executor::Infer(const ::grps::protos::v1::GrpsMessage& input,
                     ::grps::protos::v1::GrpsMessage& output,
                     GrpsContext& ctx,
                     const std::string& model_name) {
#ifdef GRPS_DEBUG
  std::string input_str;
  ::google::protobuf::TextFormat::PrintToString(input, &input_str);
  LOG4(INFO, "Infer input: " << input_str);
#endif

  if (model_name.empty()) {
    dag_->Infer(input, output, ctx);
  } else {
    auto iter = models_.find(model_name);
    if (iter == models_.end()) {
      LOG4(ERROR, "Not found model: " << model_name);
      throw ExecutorException("Not found model: " + model_name);
    }

    auto& model = iter->second;
    auto converter = model.converter_;
    auto inferer = model.inferer_;
    auto batcher = model.batcher_;
    ctx.set_converter(converter.get());
    ctx.set_inferer(inferer.get());

    if (batcher) {
      batcher->Infer(input, output, ctx);
      return;
    }

    auto begin = butil::gettimeofday_us();
    if (converter) {
      std::vector<std::pair<std::string, TensorWrapper>> inp_tensors;
      std::vector<std::pair<std::string, TensorWrapper>> out_tensors;

      converter->PreProcess(input, inp_tensors, ctx);
      if (ctx.has_err()) {
        return;
      }
      auto preprocess_end = butil::gettimeofday_us();

      inferer->Infer(inp_tensors, out_tensors, ctx);
      if (ctx.has_err()) {
        return;
      }
      auto infer_end = butil::gettimeofday_us();

      output.Clear();
      converter->PostProcess(out_tensors, output, ctx);
      if (ctx.has_err()) {
        return;
      }
      auto postprocess_end = butil::gettimeofday_us();

      LOG4(INFO, "Model(" << model_name << "), preprocess latency: " << preprocess_end - begin
                          << "us, infer latency: " << infer_end - preprocess_end
                          << "us, postprocess latency: " << postprocess_end - infer_end << "us");
    } else {
      output.Clear();
      inferer->Infer(input, output, ctx);
      if (ctx.has_err()) {
        return;
      }
      auto infer_end = butil::gettimeofday_us();
      LOG4(INFO, "Model(" << model_name << "), infer latency: " << infer_end - begin << "us");
    }
  }

#ifdef GRPS_DEBUG
  std::string output_str;
  ::google::protobuf::TextFormat::PrintToString(output, &output_str);
  LOG4(INFO, "Infer output: " << output_str);
#endif
}

void Executor::Infer(const ::grps::protos::v1::GrpsMessage& input,
                     ::grps::protos::v1::GrpsMessage& output,
                     const std::shared_ptr<GrpsContext>& ctx_sp,
                     const std::string& model_name) {
#ifdef GRPS_DEBUG
  std::string input_str;
  ::google::protobuf::TextFormat::PrintToString(input, &input_str);
  LOG4(INFO, "Infer input: " << input_str);
#endif

  if (model_name.empty()) {
    dag_->Infer(input, output, ctx_sp);
  } else {
    auto iter = models_.find(model_name);
    if (iter == models_.end()) {
      LOG4(ERROR, "Not found model: " << model_name);
      throw ExecutorException("Not found model: " + model_name);
    }

    auto& model = iter->second;
    auto converter = model.converter_;
    auto inferer = model.inferer_;
    auto batcher = model.batcher_;
    ctx_sp->set_converter(converter.get());
    ctx_sp->set_inferer(inferer.get());

    if (batcher) {
      batcher->Infer(input, output, ctx_sp);
      return;
    }

    auto begin = butil::gettimeofday_us();
    if (converter) {
      std::vector<std::pair<std::string, TensorWrapper>> inp_tensors;
      std::vector<std::pair<std::string, TensorWrapper>> out_tensors;

      converter->PreProcess(input, inp_tensors, *ctx_sp);
      if (ctx_sp->has_err()) {
        return;
      }
      auto preprocess_end = butil::gettimeofday_us();

      inferer->Infer(inp_tensors, out_tensors, *ctx_sp);
      if (ctx_sp->has_err()) {
        return;
      }
      auto infer_end = butil::gettimeofday_us();

      output.Clear();
      converter->PostProcess(out_tensors, output, *ctx_sp);
      if (ctx_sp->has_err()) {
        return;
      }
      auto postprocess_end = butil::gettimeofday_us();

      LOG4(INFO, "Model(" << model_name << "), preprocess latency: " << preprocess_end - begin
                          << "us, infer latency: " << infer_end - preprocess_end
                          << "us, postprocess latency: " << postprocess_end - infer_end << "us");
    } else {
      output.Clear();
      inferer->Infer(input, output, *ctx_sp);
      if (ctx_sp->has_err()) {
        return;
      }
      auto infer_end = butil::gettimeofday_us();
      LOG4(INFO, "Model(" << model_name << "), infer latency: " << infer_end - begin << "us");
    }
  }

#ifdef GRPS_DEBUG
  std::string output_str;
  ::google::protobuf::TextFormat::PrintToString(output, &output_str);
  LOG4(INFO, "Infer output: " << output_str);
#endif
}

void Executor::Terminate() {
  for (auto& [name, model] : models_) {
    if (model.batcher_) {
      model.batcher_->Stop();
    }
  }

  dag_.reset();
  models_.clear();
}
} // namespace netease::grps
