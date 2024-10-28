/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/09/27
 * Brief  Grps context.
 */

#include "context.h"

#include "common/pb_utils.h"
#include "converter/converter.h"
#include "logger/logger.h"
#include "model_infer/inferer.h"
#include "model_infer/tensor_wrapper.h"

namespace netease::grps {
void GrpsContext::StreamingRespond(const ::grps::protos::v1::GrpsMessage& message, bool final) {
  std::lock_guard<std::mutex> lock(streaming_mutex_);
  if (streaming_end_) {
    return;
  }

  if (rpc_stream_writer_ == nullptr && http_stream_writer_ == nullptr) {
    return;
  }

  if (!message.has_status() || message.status().status() != ::grps::protos::v1::Status::FAILURE) {
    // Add success status.
    auto& mutable_message = const_cast<::grps::protos::v1::GrpsMessage&>(message);
    mutable_message.mutable_status()->set_status(::grps::protos::v1::Status::SUCCESS);
    mutable_message.mutable_status()->set_code(brpc::HTTP_STATUS_OK);
    mutable_message.mutable_status()->set_msg("OK");
  }

  if (rpc_stream_writer_ != nullptr) { // Rpc streaming.
    try {
      rpc_stream_writer_->Write(message);
    } catch (const std::exception& e) {
      throw std::runtime_error("streaming respond failed, error: " + std::string(e.what()));
    } catch (...) {
      throw std::runtime_error("streaming respond failed, unknown error.");
    }
  } else { // Http streaming.
    switch (message.data_oneof_case()) {
      case ::grps::protos::v1::GrpsMessage::kBinData: {
        http_stream_writer_->get()->Write(message.bin_data().c_str(), message.bin_data().size());
        break;
      }
      case ::grps::protos::v1::GrpsMessage::kStrData:
      case ::grps::protos::v1::GrpsMessage::kGtensors:
      case ::grps::protos::v1::GrpsMessage::kGmap: {
        auto msg_json = Pb2json(message);
        http_stream_writer_->get()->Write(msg_json.c_str(), msg_json.size());
        break;
      }
      case ::grps::protos::v1::GrpsMessage::DATA_ONEOF_NOT_SET: {
        // LOG4(WARN, "Response data is empty");
        auto msg_json = Pb2json(message);
        http_stream_writer_->get()->Write(msg_json.c_str(), msg_json.size());
        break;
      }
      default: {
        LOG4(ERROR, "Response data type error");
        ::grps::protos::v1::GrpsMessage err_msg;
        err_msg.mutable_status()->set_code(brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR);
        err_msg.mutable_status()->set_msg("Response data type error");
        err_msg.mutable_status()->set_status(::grps::protos::v1::Status::FAILURE);
        auto msg_json = Pb2json(err_msg);
        http_stream_writer_->get()->Write(msg_json.c_str(), msg_json.size());
        break;
      }
    }
  }

  if (final) {
    streaming_end_ = true;
    if (batcher_promise_) {
      BatcherPromiseNotify();
    }
  }
}

void GrpsContext::StreamingRespondWithPostprocess(const std::vector<std::pair<std::string, TensorWrapper>>& tensors,
                                                  bool final) {
  if (converter_ == nullptr) {
    throw std::runtime_error("StreamingRespondWithPostprocess should only be used with converter");
  }
  ::grps::protos::v1::GrpsMessage message;
  converter_->PostProcess(tensors, message, *this);
  StreamingRespond(message, final);
}

void GrpsContext::CustomizedHttpStreamingRespond(const void* data, size_t size, bool final) {
  std::lock_guard<std::mutex> lock(streaming_mutex_);
  if (streaming_end_) {
    return;
  }

  if (http_stream_done_guard_ && !http_stream_done_guard_->empty()) {
    // Start streaming respond.
    http_stream_done_guard_->reset(nullptr);
  }

  if (http_controller_ == nullptr) {
    throw std::runtime_error("CustomizedHttpStreamingRespond should only be used with customized http");
  }
  if (http_stream_writer_ == nullptr) {
    return;
  }
  http_stream_writer_->get()->Write(data, size);

  if (final) {
    streaming_end_ = true;
    if (batcher_promise_) {
      BatcherPromiseNotify();
    }
  }
}

void GrpsContext::BatcherPromiseNotify() {
  boost::call_once(
    [this]() {
      if (batcher_promise_ == nullptr) {
        return;
      }
      batcher_promise_->set_value();
    },
    promise_notified_flag_);
}

bool GrpsContext::IfDisconnected() {
  if (http_stream_writer_ != nullptr) {
    return http_streaming_writer_close_;
  } else if (http_controller_ != nullptr) {
    return http_controller_->IsCanceled();
  } else if (brpc_controller_ != nullptr) {
    return brpc_controller_->IsCanceled();
  } else if (grpc_server_ctx_ != nullptr) {
    return grpc_server_ctx_->IsCancelled();
  } else {
    return false;
  }
}
} // namespace netease::grps