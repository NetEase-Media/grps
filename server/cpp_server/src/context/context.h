/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/09/27
 * Brief  Grps context. Use grps context, you can save and get user data, send streaming response, set error message,
 *        or implement customized http.
 */

#pragma once

#include <brpc/controller.h>
// Avoid conflict with glog.
#undef DCHECK_EQ
#undef DCHECK_NE
#undef DCHECK_LE
#undef DCHECK_LT
#undef DCHECK_GE
#undef DCHECK_GT
#undef DCHECK
#undef DLOG
#undef DLOG_IF
#undef DVLOG
#undef LOG
#undef LOG_IF
#undef PLOG
#undef PLOG_IF
#undef VLOG
#undef VLOG_IF
#undef VLOG_IS_ON
#undef LOG_TO_STDERR
#undef LOG_FIRST_N
#undef LOG_EVERY_N
#undef LOG_ASSERT
#undef CHECK
#undef CHECK_OP
#undef CHECK_EQ
#undef CHECK_NE
#undef CHECK_LE
#undef CHECK_LT
#undef CHECK_GE
#undef CHECK_GT

#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/sync_stream.h>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <future>
#include <mutex>
#include <unordered_map>

#include "grps.pb.h"

namespace netease::grps {

class Converter;
class ModelInferer;
class TensorWrapper;

class GrpsContext {
public:
  explicit GrpsContext(const ::grps::protos::v1::GrpsMessage* request = nullptr,
                       ::grpc::ServerWriter<::grps::protos::v1::GrpsMessage>* rpc_stream_writer = nullptr,
                       butil::intrusive_ptr<brpc::ProgressiveAttachment>* http_stream_writer = nullptr,
                       brpc::Controller* http_controller = nullptr,
                       brpc::Controller* brpc_controller = nullptr,
                       grpc::ServerContext* grpc_server_ctx = nullptr)
      : request_(request)
      , rpc_stream_writer_(rpc_stream_writer)
      , http_stream_writer_(http_stream_writer)
      , http_controller_(http_controller)
      , brpc_controller_(brpc_controller)
      , grpc_server_ctx_(grpc_server_ctx) {
    if (http_stream_writer_ != nullptr) {
      auto* http_stream_writer_close = google::protobuf::NewCallback(this, &GrpsContext::HttpStreamingWriterCloseCb);
      http_stream_writer_->get()->NotifyOnStopped(http_stream_writer_close);
    }
  }

  ~GrpsContext() {
    std::lock_guard<std::mutex> lock(user_data_mutex_);
    if (user_data_ != nullptr) {
      user_data_deleter_(user_data_);
    }
  }

  // ---------------------------- User data function. ----------------------------

  // Set user data. T should be copyable and movable.
  // Multi-thread safe.
  template <typename T,
            typename = std::enable_if_t<std::is_copy_constructible<std::decay_t<T>>::value &&
                                        std::is_move_constructible<std::decay_t<T>>::value>>
  void SetUserData(T&& data) {
    std::lock_guard<std::mutex> lock(user_data_mutex_);
    using TDecayType = typename std::decay<T>::type;
    if (user_data_ != nullptr) {
      delete reinterpret_cast<TDecayType*>(user_data_);
    }
    user_data_ = new TDecayType(std::forward<T>(data));
    user_data_deleter_ = [](void* data) { delete reinterpret_cast<TDecayType*>(data); };
  }

  // Get user data. T should be copyable and movable.
  // If user data is not set, throw exception.
  // Multi-thread safe.
  template <typename T,
            typename = std::enable_if_t<std::is_copy_constructible<std::decay_t<T>>::value &&
                                        std::is_move_constructible<std::decay_t<T>>::value>>
  std::decay_t<T>& GetUserData() {
    std::lock_guard<std::mutex> lock(user_data_mutex_);
    using TDecayType = typename std::decay<T>::type;
    if (user_data_ == nullptr) {
      throw std::runtime_error("user data is null, but you are trying to get it.");
    }
    return *reinterpret_cast<TDecayType*>(user_data_);
  }

  // ---------------------------- Streaming function. ----------------------------

  // If request is streaming request.
  // Multi-thread safe.
  [[nodiscard]] bool IfStreaming() { return rpc_stream_writer_ != nullptr || http_stream_writer_ != nullptr; }

  // Streaming respond when using streaming request.
  // Multi-thread safe.
  // NOTE: If final is true, that means the msg is the last message of the streaming response. Your should never
  // respond any message after that. In batching request mode, you should never use request message and output
  // message anymore, those will be invalid.
  void StreamingRespond(const ::grps::protos::v1::GrpsMessage& message, bool final = false);

  // Streaming respond with postprocess when using streaming request.
  // Multi-thread safe.
  // NOTE: If final is true, that means the msg is the last message of the streaming response. Your should never
  // respond any message after that. In batching request mode, you should never use request message and output
  // message anymore, those will be invalid.
  void StreamingRespondWithPostprocess(const std::vector<std::pair<std::string, TensorWrapper>>& tensors,
                                       bool final = false);

  // ---------------------------- Error function. ----------------------------

  // Set has_err. If has_err, predict process will be terminated and will return error message to client.
  // NOTE: When batching mode, that one request has err should not affect other requests. Batching process will
  // continue.
  // Multi-thread safe.
  void set_has_err(bool has_err) { has_err_ = has_err; }

  // Get has_err.
  // Multi-thread safe.
  [[nodiscard]] bool has_err() const { return has_err_; }

  // Set err_msg that will be returned to client.
  // Multi-thread safe.
  void set_err_msg(const std::string& err_msg) {
    std::lock_guard<std::mutex> lock(err_msg_mutex_);
    has_err_ = true;
    err_msg_ = err_msg;
  }

  // Get err_msg.
  // Multi-thread safe.
  [[nodiscard]] std::string err_msg() {
    std::lock_guard<std::mutex> lock(err_msg_mutex_);
    return err_msg_;
  }

  // ---------------------------- Customized http function. ----------------------------

  // Get http_controller. Only used when using http interface. Otherwise, is nullptr.
  // Not that if using streaming request, user should use CustomizedHttpStreamingRespond to respond.
  // -------- Get request --------
  // Get content type as follows:
  // const auto& content_type = ctx.http_controller()->http_request().content_type();
  // Get http body as follows:
  // const auto& http_body = ctx.http_controller()->request_attachment().to_string();
  // Get header as follows:
  // const auto& header = ctx.http_controller()->http_request().GetHeader("header_name");
  // -------- Set response --------
  // Set header as follows:
  // ctx.http_controller()->http_response().SetHeader("header_name", "header_value");
  // Set content type as follows:
  // ctx.http_controller()->http_response().set_content_type("application/json");
  // Set http body as follows:
  // ctx.http_controller()->response_attachment().append("http_body");
  // Set status code as follows:
  // ctx.http_controller()->http_response().set_status_code(brpc::HTTP_STATUS_OK);
  [[nodiscard]] brpc::Controller* http_controller() { return http_controller_; }

  // Streaming respond when using streaming request and customized predict http.
  // Multi-thread safe.
  // NOTE:
  // After first CustomizedHttpStreamingRespond call, you will not be able to use http_controller_ anymore.
  // If final is true, that means the msg is the last message of the streaming response. Your should never
  // respond any message after that. In batching request mode, you should never use request message and output
  // message anymore, those will be invalid.
  void CustomizedHttpStreamingRespond(const void* data, size_t size, bool final = false);

  // ---------------------------- Batching function. ----------------------------

  // [Only call by grps framework] Set batcher promise.
  void set_batcher_promise(boost::promise<void>* batcher_promise) { batcher_promise_ = batcher_promise; }

  // [Only call by grps framework] Get batcher promise.
  [[nodiscard]] boost::promise<void>* batcher_promise() const { return batcher_promise_; }

  // [Only call by grps framework] Notify batcher that current request is finished. Used in batching process mode.
  // Multi-thread safe.
  void BatcherPromiseNotify();

  // ---------------------------- Other function. ----------------------------

  // Get request from client.
  [[nodiscard]] const ::grps::protos::v1::GrpsMessage* request() const { return request_; }

  // [Only call by grps framework] Set converter.
  void set_converter(Converter* converter) { converter_ = converter; }

  // Get converter.
  [[nodiscard]] Converter* converter() const { return converter_; }

  // [Only call by grps framework] Set model inferer.
  void set_inferer(ModelInferer* model_inferer) { model_inferer_ = model_inferer; }

  // Get model inferer.
  [[nodiscard]] ModelInferer* inferer() const { return model_inferer_; }

  // Get grpc server context. Only used when using grpc interface. Otherwise, is nullptr.
  [[nodiscard]] grpc::ServerContext* grpc_server_ctx() const { return grpc_server_ctx_; }

  // Get brpc controller. Only used when using brpc interface. Otherwise, is nullptr.
  [[nodiscard]] brpc::Controller* brpc_controller() const { return brpc_controller_; }

  // [Only call by grps framework] Http streaming writer close callback.
  void HttpStreamingWriterCloseCb() { http_streaming_writer_close_ = true; }

  // If connection with client is broken.
  [[nodiscard]] bool IfDisconnected();

  // [Only call by grps framework] Set http_stream_done_guard.
  void set_http_stream_done_guard(brpc::ClosureGuard* http_stream_done_guard) {
    http_stream_done_guard_ = http_stream_done_guard;
  }

private:
  // request.
  const ::grps::protos::v1::GrpsMessage* request_;

  // streaming writer.
  ::grpc::ServerWriter<::grps::protos::v1::GrpsMessage>* rpc_stream_writer_;
  butil::intrusive_ptr<brpc::ProgressiveAttachment>* http_stream_writer_;
  brpc::ClosureGuard* http_stream_done_guard_ = nullptr;
  bool streaming_end_ = false;
  std::mutex streaming_mutex_;
  std::atomic<bool> http_streaming_writer_close_ = false;

  // http controller, Only used when using http interface. Otherwise, is nullptr.
  brpc::Controller* http_controller_;

  // brpc controller, Only used when using brpc interface. Otherwise, is nullptr.
  brpc::Controller* brpc_controller_;

  // grpc server context. Only used when using grpc interface. Otherwise, is nullptr.
  grpc::ServerContext* grpc_server_ctx_;

  // user data.
  void* user_data_ = nullptr;
  std::function<void(void*)> user_data_deleter_;
  std::mutex user_data_mutex_;

  // err msg.
  std::atomic<bool> has_err_ = false;
  std::string err_msg_;
  std::mutex err_msg_mutex_;

  // Used to notify batcher that current request is finished.
  boost::promise<void>* batcher_promise_ = nullptr;
  boost::once_flag promise_notified_flag_ = BOOST_ONCE_INIT;

  Converter* converter_ = nullptr;
  ModelInferer* model_inferer_ = nullptr;
};
} // namespace netease::grps