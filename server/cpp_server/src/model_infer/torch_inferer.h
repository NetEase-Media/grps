/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  torch model inferer implementation.
 */

#pragma once

#include <torch/script.h>

#include "converter/torch_tensor_converter.h"
#include "model_infer/inferer.h"

namespace netease::grps {
class TorchModelInferer : public ModelInferer {
public:
  explicit TorchModelInferer(bool infer_mode = true)
      : ModelInferer(), module_(), torch_dev_(torch::kCPU), inp_dev_(torch::kCPU), infer_mode_(infer_mode) {}
  ~TorchModelInferer() override = default;

  // Clone inferer for duplicated use. Don't edit this function.
  ModelInferer* Clone() override { return new TorchModelInferer(); }

  /**
   * @brief Init model inferer.
   * @param path: Model path, it can be a file path or a directory path.
   * @param device: Device to run model.
   * @param args: More args.
   * @throw InfererException: If init failed, throw InfererException and will be caught by server and show error message
   * to user when start service.
   */
  void Init(const std::string& path, const std::string& device, const YAML::Node& args) override;

  /**
   * @brief Load model.
   * @throw InfererException: If load failed, throw InfererException and will be caught by server and show error
   * message to user when start service.
   */
  void Load() override;

  /**
   * @brief Infer model.
   * @param inputs: Input tensor of model.
   * @param outputs: Output tensor of model.
   * @param ctx: Context of current request.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void Infer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
             std::vector<std::pair<std::string, TensorWrapper>>& outputs,
             GrpsContext& ctx) override;

  /**
   * Not support now.
   * @brief Infer model with profiler.
   * @param profiler_path: Profiler path.
   * @param inputs: Input tensor of model.
   * @param outputs: Output tensor of model.
   * @param ctx: Context of current request.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void InferWithProfiler(const std::string& profiler_path,
                         const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                         std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                         GrpsContext& ctx) override;

  /**
   * @brief Infer model in batch.
   * @param inputs: Input tensor of model in batch.
   * @param outputs: Output tensor of model in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void BatchInfer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                  std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                  std::vector<GrpsContext*>& ctxs) override;

  /**
   * Not support now.
   * @brief Infer model in batch with profiler.
   * @param profiler_path: Profiler path.
   * @param inputs: Input tensor of model in batch.
   * @param outputs: Output tensor of model in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void BatchInferWithProfiler(const std::string& profiler_path,
                              const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                              std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                              std::vector<GrpsContext*>& ctxs) override;

  // --------------------------------------- No converter mode [BEGIN] ---------------------------------------

  /**
   * Used when in `no converter mode`. Input and output are directly GrpsMessage.
   * @brief Infer model.
   * @param input: Input.
   * @param output: Output.
   * @param ctx: Context of current request.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             GrpsContext& ctx) override;

  /**
   * Not support now. Used when in `no converter mode`. Input and output are directly GrpsMessage.
   * @brief Infer model with profiler.
   * @param profiler_path: Profiler path.
   * @param input: Input.
   * @param output: Output.
   * @param ctx: Context of current request.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void InferWithProfiler(const std::string& profiler_path,
                         const ::grps::protos::v1::GrpsMessage& input,
                         ::grps::protos::v1::GrpsMessage& output,
                         GrpsContext& ctx) override;

  /**
   * Used when in `no converter mode`. Inputs and outputs are directly GrpsMessage vector.
   * @param inputs: Inputs in batch.
   * @param outputs: Outputs in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void BatchInfer(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                  std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                  std::vector<GrpsContext*>& ctxs) override;

  /**
   * Not support now. Used when in `no converter mode`. Inputs and outputs are directly GrpsMessage vector.
   * @param profiler_path: Profiler path.
   * @param inputs: Inputs in batch.
   * @param outputs: Outputs in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void BatchInferWithProfiler(const std::string& profiler_path,
                              std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                              std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                              std::vector<GrpsContext*>& ctxs) override;

  // --------------------------------------- No converter mode [END] ---------------------------------------

protected:
  torch::jit::script::Module module_;
  torch::Device torch_dev_;
  torch::Device inp_dev_;
  std::vector<std::string> inp_tensor_names_;
  std::vector<std::string> out_tensor_names_;
  TorchTensorConverter converter_; // Used when no converter mode.

  // If enable inference mode(libtorch version >=1.9.0) when infer. If false, will only use no grad mode when infer.
  // When enable inference mode, output tensor may be immutable.
  // Default enable.
  bool infer_mode_;
};
} // namespace netease::grps