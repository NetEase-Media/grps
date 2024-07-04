/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2024/04/24
 * Brief  Batching for inference.
 */

#include "batcher.h"

#include <boost/asio/post.hpp>
#include <boost/thread/future.hpp>

#include "config/global_config.h"
#include "logger/logger.h"

namespace netease::grps {
void DynamicBatcher::Init(
  std::string name, int max_batch_size, int batch_timeout_us, Converter* converter, ModelInferer* inferer) {
  Batcher::Init(name, max_batch_size, batch_timeout_us, converter, inferer);
  worker_tp_ = std::make_unique<boost::asio::thread_pool>(GlobalConfig::Instance().server_config().max_concurrency);
  LOG4(INFO, "DynamicBatcher(" << name << ") init, max_batch_size: " << max_batch_size
                               << ", batch_timeout_us: " << batch_timeout_us);
}

void DynamicBatcher::Start() {
  LOG4(INFO, "DynamicBatcher(" << name_ << ") start");
  running_ = true;
  schedule_thread_ = std::thread([this] {
    while (running_) {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      queue_cv_.wait(lock, [this] { return !task_queue_.empty() || !running_; });
      if (!running_) {
        return;
      }

      std::vector<Task> tasks;
      while (!task_queue_.empty() && tasks.size() < max_batch_size_) {
        tasks.emplace_back(task_queue_.front());
        task_queue_.pop_front();
      }

      auto timeout_point_us = std::chrono::system_clock::now() + std::chrono::microseconds(batch_timeout_us_);
      while (tasks.size() < max_batch_size_ && std::chrono::system_clock::now() < timeout_point_us) {
        // Wait for more tasks, until batch_timeout_us_.
        queue_cv_.wait_until(lock, timeout_point_us, [this] { return !task_queue_.empty() || !running_; });
        if (!running_) {
          return;
        }
        while (!task_queue_.empty() && tasks.size() < max_batch_size_) {
          tasks.emplace_back(task_queue_.front());
          task_queue_.pop_front();
        }
      }

      if (tasks.empty()) {
        continue;
      }
      boost::asio::post(*worker_tp_, [this, tasks = std::move(tasks)]() mutable {
        std::vector<const ::grps::protos::v1::GrpsMessage*> inputs;
        std::vector<::grps::protos::v1::GrpsMessage*> outputs;
        std::vector<GrpsContext*> ctxs;
        for (const auto& task : tasks) {
          inputs.emplace_back(task.input);
          outputs.emplace_back(task.output);
          ctxs.emplace_back(task.ctx == nullptr ? task.ctx_sp.get() : task.ctx);
        }
        BatchInferProcess(inputs, outputs, ctxs);
      });
    }
  });
}

void DynamicBatcher::Stop() {
  LOG4(INFO, "DynamicBatcher(" << name_ << ") stop");
  running_ = false;
  queue_cv_.notify_all();
  if (schedule_thread_.joinable()) {
    schedule_thread_.join();
  }
}

void DynamicBatcher::Infer(const ::grps::protos::v1::GrpsMessage& input,
                           ::grps::protos::v1::GrpsMessage& output,
                           GrpsContext& ctx) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "DynamicBatcher(" << name_ << ") infer, input: " << input.DebugString());
#endif
  Task task = {&input, &output, &ctx, nullptr};
  boost::promise<void> promise;
  task.ctx->set_batcher_promise(&promise);
  auto future = promise.get_future();
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.emplace_back(task);
    queue_cv_.notify_one();
  }
  future.wait();
}

void DynamicBatcher::Infer(const ::grps::protos::v1::GrpsMessage& input,
                           ::grps::protos::v1::GrpsMessage& output,
                           const std::shared_ptr<GrpsContext>& ctx_sp) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "DynamicBatcher(" << name_ << ") infer, input: " << input.DebugString());
#endif
  Task task = {&input, &output, nullptr, ctx_sp};
  boost::promise<void> promise;
  task.ctx_sp->set_batcher_promise(&promise);
  auto future = promise.get_future();
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.emplace_back(task);
    queue_cv_.notify_one();
  }
  future.wait();
}

static bool AllErr(const std::vector<GrpsContext*>& ctxs) {
  bool all_err = true;
  for (const auto& ctx : ctxs) {
    if (!ctx->has_err()) {
      all_err = false;
      break;
    }
  }
  return all_err;
}

void DynamicBatcher::BatchInferProcess(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                                       std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                                       std::vector<netease::grps::GrpsContext*>& ctxs) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "DynamicBatcher(" << name_ << ") batch inference process, inputs size: " << inputs.size()
                               << ", outputs size: " << outputs.size() << ", ctxs size: " << ctxs.size());
#endif
  try {
    // Batch infer.
    std::vector<std::pair<std::string, TensorWrapper>> input_tensors;
    std::vector<std::pair<std::string, TensorWrapper>> output_tensors;
    if (converter_) {
      auto begin = butil::gettimeofday_us();
      converter_->BatchPreProcess(inputs, input_tensors, ctxs);
      if (AllErr(ctxs)) {
        goto NOTIFY;
      }
      auto preprocess_end = butil::gettimeofday_us();
      inferer_->BatchInfer(input_tensors, output_tensors, ctxs);
      if (AllErr(ctxs)) {
        goto NOTIFY;
      }
      auto infer_end = butil::gettimeofday_us();
      converter_->BatchPostProcess(output_tensors, outputs, ctxs);
      if (AllErr(ctxs)) {
        goto NOTIFY;
      }
      auto postprocess_end = butil::gettimeofday_us();
      LOG4(INFO, "DynamicBatcher(" << name_ << "), batch_size: " << inputs.size() << ", preprocess latency: "
                                   << preprocess_end - begin << "us, infer latency: " << infer_end - preprocess_end
                                   << "us, postprocess latency: " << postprocess_end - infer_end << "us");
    } else {
      auto begin = butil::gettimeofday_us();
      inferer_->BatchInfer(inputs, outputs, ctxs);
      if (AllErr(ctxs)) {
        goto NOTIFY;
      }
      auto infer_end = butil::gettimeofday_us();
      LOG4(INFO, "DynamicBatcher(" << name_ << "), batch_size: " << inputs.size()
                                   << ", infer latency: " << infer_end - begin << "us");
    }

  NOTIFY:
    // Notify.
    for (auto& ctx : ctxs) {
      ctx->BatcherPromiseNotify();
    }
  } catch (const std::exception& e) {
    LOG4(ERROR, "DynamicBatcher(" << name_ << ") batch inference process failed, " << e.what());
    for (auto& ctx : ctxs) {
      ctx->set_err_msg(e.what());
      ctx->BatcherPromiseNotify();
    }
  } catch (...) {
    std::string err = "DynamicBatcher(" + name_ + ") batch inference process failed, unknown error.";
    LOG4(ERROR, err);
    for (auto& ctx : ctxs) {
      ctx->set_err_msg(err);
      ctx->BatcherPromiseNotify();
    }
  }
}
} // namespace netease::grps