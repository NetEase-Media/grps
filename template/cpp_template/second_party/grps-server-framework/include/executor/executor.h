/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  Executor to run complete infer, including outlier, converter, dag, model infer and etc.
 */

#pragma once

#include <string>

#include "context/context.h"
#include "converter/converter.h"
#include "dag/dag.h"
#include "grps.pb.h"
#include "model/model.h"
#include "model_infer/inferer.h"

namespace netease::grps {
class Executor {
public:
  class ExecutorException : public std::exception {
  public:
    explicit ExecutorException(std::string message) : message_(std::move(message)) {}
    ~ExecutorException() override = default;
    [[nodiscard]] const char* what() const noexcept override {
      static std::string err_message;
      err_message = "[ExecutorException] " + message_;
      return err_message.c_str();
    }

  private:
    std::string message_;
  };

  static Executor& Instance() {
    static Executor executor;
    return executor;
  }

  ~Executor() = default;
  Executor(const Executor&) = delete;
  Executor& operator=(const Executor&) = delete;
  Executor(Executor&&) = delete;
  Executor& operator=(Executor&&) = delete;

  void Init();

  /**
   * @brief Infer.
   * @param input: Input message from client.
   * @param output: Output message to client.
   * @param ctx: Context of current request.
   * @param model_name: Model name(with `name-version` format) to predict. If not define, will use default model dag
   * (defined in inference.yml) to predict.
   * @throw ExecutorException: If infer failed, throw ExecutorException and will be caught by server and return error
   * message to client.
   */
  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             GrpsContext& ctx,
             const std::string& model_name = "");

  /**
   * @brief Infer.
   * @param input: Input message from client.
   * @param output: Output message to client.
   * @param ctx_sp: Context shared ptr of current request.
   * @param model_name: Model name(with `name-version` format) to predict. If not define, will use default model dag
   * (defined in inference.yml) to predict.
   * @throw ExecutorException: If infer failed, throw ExecutorException and will be caught by server and return error
   * message to client.
   */
  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             const std::shared_ptr<GrpsContext>& ctx_sp,
             const std::string& model_name = "");

  void Terminate();

private:
  Executor() = default;

  void InitOutlier();
  void InitModels();
  void InitDag();

  std::unordered_map<std::string, Model> models_ = {};
  std::shared_ptr<InferDag> dag_ = nullptr;
};
} // namespace netease::grps
