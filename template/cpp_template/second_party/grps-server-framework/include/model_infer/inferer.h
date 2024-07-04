/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  Define deep learning model inferer base class for different deep learning framework. Including
 *        model load and model infer. Customized model inferer class should inherit this class.
 */

#pragma once

#include <yaml-cpp/yaml.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "context/context.h"
#include "model_infer/tensor_wrapper.h"

// Register inferer, if inferer_name has been registered, it will be replaced by new inferer.
#define REGISTER_INFERER(InfererClass, inferer_name)                                                  \
  static std::shared_ptr<ModelInferer> g_##inferer_name##_inferer = std::make_shared<InfererClass>(); \
  static bool g_##inferer_name##_inferer_registered =                                                 \
    (ModelInfererRegistry::Instance().Register(#inferer_name, g_##inferer_name##_inferer))

namespace netease::grps {
class ModelInferer {
public:
  class InfererException : public std::exception {
  public:
    explicit InfererException(std::string message) : message_(std::move(message)) {}
    ~InfererException() override = default;
    [[nodiscard]] const char* what() const noexcept override {
      static std::string err_message;
      err_message = "[InfererException] " + message_;
      return err_message.c_str();
    }

  private:
    std::string message_;
  };

  ModelInferer() = default;

  virtual ~ModelInferer() = default;

  // Clone inferer for duplicated use.
  virtual ModelInferer* Clone() = 0;

  /**
   * @brief Init model inferer.
   * @param path: Model path, it can be a file path or a directory path.
   * @param device: Device to run model.
   * @param args: More args.
   * @throw InfererException: If init failed, can throw InfererException and will be caught by server and show error
   * message to user when start service.
   */
  virtual void Init(const std::string& path, const std::string& device, const YAML::Node& args) {
    path_ = path;
    device_ = device;
    args_ = args;
  }

  /**
   * @brief Load model.
   * @throw InfererException: If load failed, can throw InfererException and will be caught by server and show error
   * message to user when start service.
   */
  virtual void Load() { throw InfererException("Load not implemented."); }

  /**
   * @brief Infer model.
   * @param inputs: Input tensor of model.
   * @param outputs: Output tensor of model.
   * @param ctx: Context of current request.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  virtual void Infer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                     std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                     GrpsContext& ctx) {
    throw InfererException("Infer not implemented.");
  }

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
  virtual void InferWithProfiler(const std::string& profiler_path,
                                 const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                 std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                 GrpsContext& ctx) {
    throw InfererException("InferWithProfiler not implemented.");
  }

  /**
   * @brief Infer model in batch.
   * @param inputs: Input tensor of model in batch.
   * @param outputs: Output tensor of model in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  virtual void BatchInfer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                          std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                          std::vector<GrpsContext*>& ctxs) {
    throw InfererException("BatchInfer not implemented.");
  }

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
  virtual void BatchInferWithProfiler(const std::string& profiler_path,
                                      const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                                      std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                                      std::vector<GrpsContext*>& ctxs) {
    throw InfererException("BatchInferWithProfiler not implemented.");
  }

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
  virtual void Infer(const ::grps::protos::v1::GrpsMessage& input,
                     ::grps::protos::v1::GrpsMessage& output,
                     GrpsContext& ctx) {
    throw InfererException("Infer not implemented.");
  }

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
  virtual void InferWithProfiler(const std::string& profiler_path,
                                 const ::grps::protos::v1::GrpsMessage& input,
                                 ::grps::protos::v1::GrpsMessage& output,
                                 GrpsContext& ctx) {
    throw InfererException("InferWithProfiler not implemented.");
  }

  /**
   * Used when in `no converter mode`. Inputs and outputs are directly GrpsMessage vector.
   * @param inputs: Inputs in batch.
   * @param outputs: Outputs in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  virtual void BatchInfer(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                          std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                          std::vector<GrpsContext*>& ctxs) {
    throw InfererException("BatchInfer not implemented.");
  }

  /**
   * Not support now. Used when in `no converter mode`. Inputs and outputs are directly GrpsMessage vector.
   * @param profiler_path: Profiler path.
   * @param inputs: Inputs in batch.
   * @param outputs: Outputs in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  virtual void BatchInferWithProfiler(const std::string& profiler_path,
                                      std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                      std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                      std::vector<GrpsContext*>& ctxs) {
    throw InfererException("BatchInferWithProfiler not implemented.");
  }

  // --------------------------------------- No converter mode [END] ---------------------------------------

protected:
  std::string path_;
  std::string device_;
  YAML::Node args_;
};

class ModelInfererRegistry {
public:
  ~ModelInfererRegistry() = default;
  ModelInfererRegistry(const ModelInfererRegistry&) = delete;
  ModelInfererRegistry& operator=(const ModelInfererRegistry&) = delete;
  ModelInfererRegistry(ModelInfererRegistry&&) = delete;
  ModelInfererRegistry& operator=(ModelInfererRegistry&&) = delete;

  static ModelInfererRegistry& Instance() {
    static ModelInfererRegistry instance;
    return instance;
  }

  bool Register(const std::string& name, const std::shared_ptr<ModelInferer>& inferer);

  std::shared_ptr<ModelInferer> GetInferer(const std::string& name);

private:
  ModelInfererRegistry() = default;

  std::unordered_map<std::string, std::shared_ptr<ModelInferer>> g_inferer_map_;
};
} // namespace netease::grps