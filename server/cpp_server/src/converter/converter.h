/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  Converter base definition of model, including pre-process and post-process. Customized converter should
 *        inherit this class.
 */

#pragma once

#include <yaml-cpp/yaml.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "context/context.h"
#include "grps.pb.h"
#include "model_infer/tensor_wrapper.h"

// Register converter, if converter_name has been registered, it will be replaced by new converter.
#define REGISTER_CONVERTER(ConverterClass, converter_name)                                               \
  static std::shared_ptr<Converter> g_##converter_name##_converter = std::make_shared<ConverterClass>(); \
  static bool g_##converter_name##_converter_registered =                                                \
    (ConverterRegistry::Instance().Register(#converter_name, g_##converter_name##_converter))

namespace netease::grps {
class Converter {
public:
  class ConverterException : public std::exception {
  public:
    explicit ConverterException(std::string message) : message_(std::move(message)) {}
    ~ConverterException() override = default;
    [[nodiscard]] const char* what() const noexcept override {
      static std::string err_message;
      err_message = "[ConverterException] " + message_;
      return err_message.c_str();
    }

  private:
    std::string message_;
  };

  Converter() = default;

  virtual ~Converter() = default;

  // Clone converter for duplicated use.
  virtual Converter* Clone() = 0;

  /**
   * @brief Init converter.
   * @param path: Path.
   * @param args: More args.
   * @throw ConverterException: If init failed, can throw ConverterException and will be caught by server and show error
   * message to user when start service.
   */
  virtual void Init(const std::string& path, const YAML::Node& args) {
    path_ = path;
    args_ = args;
  }

  /**
   * @brief PreProcess input message.
   * @param input: Input message from client or previous model(multi model sequential mode).
   * @param output: Input tensor of model inferer.
   * @param ctx: Context of current request.
   * @throw ConverterException: If pre-process failed, can throw ConverterException and will be caught by server and
   * return error message to client.
   */
  virtual void PreProcess(const ::grps::protos::v1::GrpsMessage& input,
                          std::vector<std::pair<std::string, TensorWrapper>>& output,
                          GrpsContext& ctx) {
    throw ConverterException("PreProcess not implemented.");
  }

  /**
   * @brief PostProcess output tensor.
   * @param input: Output tensor of model inferer.
   * @param output: Output message to client or next model(multi model sequential mode).
   * @param ctx: Context of current request.
   * @throw ConverterException: If post-process failed, can throw ConverterException and will be caught by server and
   * return error message to client.
   */
  virtual void PostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                           ::grps::protos::v1::GrpsMessage& output,
                           GrpsContext& ctx) {
    throw ConverterException("PostProcess not implemented.");
  }

  /**
   * @brief Batch pre-process input message.
   * @param inputs: Inputs in batch messages from client or previous model(multi model sequential mode).
   * @param output: Input tensor of model inferer.
   * @param ctxs: Contexts of each request in batch.
   * @throw ConverterException: If pre-process failed, can throw ConverterException and will be caught by server and
   * return error message to client.
   */
  virtual void BatchPreProcess(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                               std::vector<std::pair<std::string, TensorWrapper>>& output,
                               std::vector<GrpsContext*>& ctxs) {
    throw ConverterException("BatchPreProcess not implemented.");
  }

  /**
   * @brief Batch post-process output tensor.
   * @param input: Output tensor of model inferer.
   * @param outputs: Output messages in batch to client or next model(multi model sequential mode).
   * @param ctxs: Contexts of each request in batch.
   * @throw ConverterException: If post-process failed, can throw ConverterException and will be caught by server and
   * return error message to client.
   */
  virtual void BatchPostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                                std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                std::vector<GrpsContext*>& ctxs) {
    throw ConverterException("BatchPostProcess not implemented.");
  }

protected:
  std::string path_;
  YAML::Node args_;
};

class ConverterRegistry {
public:
  ~ConverterRegistry() = default;
  ConverterRegistry(const ConverterRegistry&) = delete;
  ConverterRegistry& operator=(const ConverterRegistry&) = delete;
  ConverterRegistry(ConverterRegistry&&) = delete;
  ConverterRegistry& operator=(ConverterRegistry&&) = delete;

  static ConverterRegistry& Instance() {
    static ConverterRegistry instance;
    return instance;
  }

  bool Register(const std::string& name, std::shared_ptr<Converter> converter);

  std::shared_ptr<Converter> GetConverter(const std::string& name);

private:
  ConverterRegistry() = default;

  std::unordered_map<std::string, std::shared_ptr<Converter>> converter_map_;
};

} // namespace netease::grps
