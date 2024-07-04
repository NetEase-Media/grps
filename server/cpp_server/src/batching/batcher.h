/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2024/04/24
 * Brief  Batching for inference.
 */

#pragma once

#include <boost/asio/thread_pool.hpp>
#include <future>
#include <memory>
#include <queue>
#include <thread>

#include "context/context.h"
#include "converter/converter.h"
#include "grps.pb.h"
#include "model_infer/inferer.h"

namespace netease::grps {
class Batcher {
public:
  class BatcherException : public std::exception {
  public:
    explicit BatcherException(std::string message) : message_(std::move(message)) {}
    ~BatcherException() override = default;
    [[nodiscard]] const char* what() const noexcept override {
      static std::string err_message;
      err_message = "[BatcherException] " + message_;
      return err_message.c_str();
    }

  private:
    std::string message_;
  };

  struct Task {
    const ::grps::protos::v1::GrpsMessage* input;
    ::grps::protos::v1::GrpsMessage* output;
    GrpsContext* ctx;
    std::shared_ptr<GrpsContext> ctx_sp;
  };

  Batcher() = default;
  virtual ~Batcher() = default;

  virtual void Init(
    std::string name, int max_batch_size, int batch_timeout_us, Converter* converter, ModelInferer* inferer) {
    name_ = std::move(name);
    max_batch_size_ = max_batch_size;
    batch_timeout_us_ = batch_timeout_us;
    converter_ = converter;
    inferer_ = inferer;
    running_ = false;
  }

  virtual void Start() = 0;

  /**
   * @brief Infer in batching.
   * @param input: Input message from client.
   * @param output: Output message to client.
   * @param ctx: Context of current request.
   * @throw BatcherException: If infer failed, throw BatcherException and will be caught by server and return error
   * message to client.
   */
  virtual void Infer(const ::grps::protos::v1::GrpsMessage& input,
                     ::grps::protos::v1::GrpsMessage& output,
                     GrpsContext& ctx) = 0;

  /**
   * @brief Infer in batching.
   * @param input: Input message from client.
   * @param output: Output message to client.
   * @param ctx_sp: Context shared ptr of current request.
   * @throw BatcherException: If infer failed, throw BatcherException and will be caught by server and return error
   * message to client.
   */
  virtual void Infer(const ::grps::protos::v1::GrpsMessage& input,
                     ::grps::protos::v1::GrpsMessage& output,
                     const std::shared_ptr<GrpsContext>& ctx_sp) = 0;

  virtual void Stop() = 0;

protected:
  std::string name_;
  int max_batch_size_{};
  int batch_timeout_us_{};
  std::atomic<bool> running_{};
  Converter* converter_{};
  ModelInferer* inferer_{};
};

class DynamicBatcher : public Batcher {
public:
  DynamicBatcher() = default;
  ~DynamicBatcher() override = default;

  void Init(
    std::string name, int max_batch_size, int batch_timeout_us, Converter* converter, ModelInferer* inferer) override;

  void Start() override;

  /**
   * @brief Infer in batching.
   * @param input: Input message from client.
   * @param output: Output message to client.
   * @param ctx: Context of current request.
   * @throw BatcherException: If infer failed, throw BatcherException and will be caught by server and return error
   * message to client.
   */
  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             GrpsContext& ctx) override;

  /**
   * @brief Infer in batching.
   * @param input: Input message from client.
   * @param output: Output message to client.
   * @param ctx_sp: Context shared ptr of current request.
   * @throw BatcherException: If infer failed, throw BatcherException and will be caught by server and return error
   * message to client.
   */
  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             const std::shared_ptr<GrpsContext>& ctx_sp) override;

  void Stop() override;

private:
  void BatchInferProcess(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                         std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                         std::vector<netease::grps::GrpsContext*>& ctxs);

  std::deque<Task> task_queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::thread schedule_thread_;
  std::unique_ptr<boost::asio::thread_pool> worker_tp_;
};
} // namespace netease::grps