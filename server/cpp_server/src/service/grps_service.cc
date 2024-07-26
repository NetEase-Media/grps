/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/06
 * Brief  grps service.
 */

#include "grps_service.h"

#include <brpc/controller.h>
#include <butil/time.h>

#include <boost/asio/post.hpp>
#include <future>

#include "constant.h"
#include "handler/grps_handler.h"
#include "logger/logger.h"
#include "monitor/monitor.h"

#define BRPC_HANDLER_PROCESS(FuncName)                               \
  {                                                                  \
    auto* cntl = dynamic_cast<brpc::Controller*>(controller);        \
    switch (handler_type_) {                                         \
      case HandlerType::kHTTP:                                       \
        http_handler_.FuncName(cntl, request, response);             \
        break;                                                       \
      case HandlerType::kRPC:                                        \
        rpc_handler_.FuncName(cntl, request, response);              \
        break;                                                       \
      default:                                                       \
        LOG4(ERROR, "Unsupported protocol: " << int(handler_type_)); \
        cntl->SetFailed(brpc::EREQUEST, "Unsupported protocol");     \
        return;                                                      \
    }                                                                \
  }

namespace netease::grps {
std::unique_ptr<::boost::asio::thread_pool> g_predict_threadpool = nullptr;

void GrpsBrpcServiceImpl::Predict(::google::protobuf::RpcController* controller,
                                  const ::grps::protos::v1::GrpsMessage* request,
                                  ::grps::protos::v1::GrpsMessage* response,
                                  ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);

  auto remote_side = dynamic_cast<brpc::Controller*>(controller)->remote_side();
  MONITOR_INC(QPS, 1);
  auto begin = butil::gettimeofday_us();

  boost::promise<void> promise;
  auto future = promise.get_future();
  boost::asio::post(*g_predict_threadpool, [&]() {
    BRPC_HANDLER_PROCESS(Predict);
    promise.set_value();
  });
  future.wait();

  auto latency = float(butil::gettimeofday_us() - begin) / 1000.0;
  MONITOR_AVG(REQ_LATENCY_AVG, latency);
  MONITOR_MAX(REQ_LATENCY_MAX, latency);
  MONITOR_CDF(REQ_LATENCY_CDF, latency);
  LOG4(INFO, "[Predict] from " << remote_side << ", latency: " << latency << "ms.");
}

void GrpsBrpcServiceImpl::PredictByHttp(::google::protobuf::RpcController* controller,
                                        const ::grps::protos::v1::EmptyGrpsMessage* request,
                                        ::grps::protos::v1::EmptyGrpsMessage* response,
                                        ::google::protobuf::Closure* done) {
  auto remote_side = dynamic_cast<brpc::Controller*>(controller)->remote_side();
  MONITOR_INC(QPS, 1);
  auto begin = butil::gettimeofday_us();
  auto* cntl = dynamic_cast<brpc::Controller*>(controller);
  if (cntl->request_protocol() != brpc::PROTOCOL_HTTP) {
    LOG4(ERROR, "Unsupported protocol: " << cntl->request_protocol());
    cntl->SetFailed(brpc::EREQUEST, "Unsupported protocol");
    return;
  }

  boost::promise<void> promise;
  auto future = promise.get_future();
  boost::asio::post(*g_predict_threadpool, [&]() {
    http_handler_.PredictByHttp(cntl, request, response, done);
    promise.set_value();
  });
  future.wait();

  auto latency = float(butil::gettimeofday_us() - begin) / 1000.0;
  MONITOR_AVG(REQ_LATENCY_AVG, latency);
  MONITOR_MAX(REQ_LATENCY_MAX, latency);
  MONITOR_CDF(REQ_LATENCY_CDF, latency);
  LOG4(INFO, "[Predict] from " << remote_side << ", latency: " << latency << "ms.");
}

void GrpsBrpcServiceImpl::Online(::google::protobuf::RpcController* controller,
                                 const ::grps::protos::v1::GrpsMessage* request,
                                 ::grps::protos::v1::GrpsMessage* response,
                                 ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto remote_side = dynamic_cast<brpc::Controller*>(controller)->remote_side();
  LOG4(INFO, "[Online] from " << remote_side);
  BRPC_HANDLER_PROCESS(Online);
}

void GrpsBrpcServiceImpl::Offline(::google::protobuf::RpcController* controller,
                                  const ::grps::protos::v1::GrpsMessage* request,
                                  ::grps::protos::v1::GrpsMessage* response,
                                  ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto remote_side = dynamic_cast<brpc::Controller*>(controller)->remote_side();
  LOG4(INFO, "[Offline] from " << remote_side);
  BRPC_HANDLER_PROCESS(Offline);
}

void GrpsBrpcServiceImpl::CheckLiveness(::google::protobuf::RpcController* controller,
                                        const ::grps::protos::v1::GrpsMessage* request,
                                        ::grps::protos::v1::GrpsMessage* response,
                                        ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto remote_side = dynamic_cast<brpc::Controller*>(controller)->remote_side();
  LOG4(INFO, "[CheckLiveness] from " << remote_side);
  BRPC_HANDLER_PROCESS(CheckLiveness);
}

void GrpsBrpcServiceImpl::CheckReadiness(::google::protobuf::RpcController* controller,
                                         const ::grps::protos::v1::GrpsMessage* request,
                                         ::grps::protos::v1::GrpsMessage* response,
                                         ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto remote_side = dynamic_cast<brpc::Controller*>(controller)->remote_side();
  LOG4(INFO, "[CheckReadiness] from " << remote_side);
  BRPC_HANDLER_PROCESS(CheckReadiness);
}

void GrpsBrpcServiceImpl::ServerMetadata(::google::protobuf::RpcController* controller,
                                         const ::grps::protos::v1::GrpsMessage* request,
                                         ::grps::protos::v1::GrpsMessage* response,
                                         ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto remote_side = dynamic_cast<brpc::Controller*>(controller)->remote_side();
  LOG4(INFO, "[ServerMetadata] from " << remote_side);
  BRPC_HANDLER_PROCESS(ServerMetadata);
}

void GrpsBrpcServiceImpl::ModelMetadata(::google::protobuf::RpcController* controller,
                                        const ::grps::protos::v1::GrpsMessage* request,
                                        ::grps::protos::v1::GrpsMessage* response,
                                        ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto remote_side = dynamic_cast<brpc::Controller*>(controller)->remote_side();
  LOG4(INFO, "[ModelMetadata] from " << remote_side);
  BRPC_HANDLER_PROCESS(ModelMetadata);
}

::grpc::Status GrpsGrpcServiceImpl::Predict(::grpc::ServerContext* context,
                                            const ::grps::protos::v1::GrpsMessage* request,
                                            ::grps::protos::v1::GrpsMessage* response) {
  auto remote_side = context->peer();
  MONITOR_INC(QPS, 1);
  auto begin = butil::gettimeofday_us();

  boost::promise<void> promise;
  auto future = promise.get_future();
  boost::asio::post(*g_predict_threadpool, [&]() {
    rpc_handler_.Predict(context, request, response);
    promise.set_value();
  });
  future.wait();

  auto latency = float(butil::gettimeofday_us() - begin) / 1000.0;
  MONITOR_AVG(REQ_LATENCY_AVG, latency);
  MONITOR_MAX(REQ_LATENCY_MAX, latency);
  MONITOR_CDF(REQ_LATENCY_CDF, latency);
  LOG4(INFO, "[Predict] from " << remote_side << ", latency: " << latency << "ms.");
  return ::grpc::Status::OK;
}

::grpc::Status GrpsGrpcServiceImpl::PredictStreaming(::grpc::ServerContext* context,
                                                     const ::grps::protos::v1::GrpsMessage* request,
                                                     ::grpc::ServerWriter<::grps::protos::v1::GrpsMessage>* writer) {
  auto remote_side = context->peer();
  MONITOR_INC(QPS, 1);
  auto begin = butil::gettimeofday_us();

  boost::promise<void> promise;
  auto future = promise.get_future();
  boost::asio::post(*g_predict_threadpool, [&]() {
    rpc_handler_.PredictStreaming(context, request, writer);
    promise.set_value();
  });
  future.wait();

  auto latency = float(butil::gettimeofday_us() - begin) / 1000.0;
  MONITOR_AVG(REQ_LATENCY_AVG, latency);
  MONITOR_MAX(REQ_LATENCY_MAX, latency);
  MONITOR_CDF(REQ_LATENCY_CDF, latency);
  LOG4(INFO, "[PredictStreaming] from " << remote_side << ", latency: " << latency << "ms.");
  return ::grpc::Status::OK;
}

::grpc::Status GrpsGrpcServiceImpl::Online(::grpc::ServerContext* context,
                                           const ::grps::protos::v1::GrpsMessage* request,
                                           ::grps::protos::v1::GrpsMessage* response) {
  auto remote_side = context->peer();
  LOG4(INFO, "[Online] from " << remote_side);
  rpc_handler_.Online(nullptr, request, response);
  return ::grpc::Status::OK;
}

::grpc::Status GrpsGrpcServiceImpl::Offline(::grpc::ServerContext* context,
                                            const ::grps::protos::v1::GrpsMessage* request,
                                            ::grps::protos::v1::GrpsMessage* response) {
  auto remote_side = context->peer();
  LOG4(INFO, "[Offline] from " << remote_side);
  rpc_handler_.Offline(nullptr, request, response);
  return ::grpc::Status::OK;
}

::grpc::Status GrpsGrpcServiceImpl::CheckLiveness(::grpc::ServerContext* context,
                                                  const ::grps::protos::v1::GrpsMessage* request,
                                                  ::grps::protos::v1::GrpsMessage* response) {
  auto remote_side = context->peer();
  LOG4(INFO, "[CheckLiveness] from " << remote_side);
  rpc_handler_.CheckLiveness(nullptr, request, response);
  return ::grpc::Status::OK;
}

::grpc::Status GrpsGrpcServiceImpl::CheckReadiness(::grpc::ServerContext* context,
                                                   const ::grps::protos::v1::GrpsMessage* request,
                                                   ::grps::protos::v1::GrpsMessage* response) {
  auto remote_side = context->peer();
  LOG4(INFO, "[CheckReadiness] from " << remote_side);
  rpc_handler_.CheckReadiness(nullptr, request, response);
  return ::grpc::Status::OK;
}

::grpc::Status GrpsGrpcServiceImpl::ServerMetadata(::grpc::ServerContext* context,
                                                   const ::grps::protos::v1::GrpsMessage* request,
                                                   ::grps::protos::v1::GrpsMessage* response) {
  auto remote_side = context->peer();
  LOG4(INFO, "[ServerMetadata] from " << remote_side);
  rpc_handler_.ServerMetadata(nullptr, request, response);
  return ::grpc::Status::OK;
}

::grpc::Status GrpsGrpcServiceImpl::ModelMetadata(::grpc::ServerContext* context,
                                                  const ::grps::protos::v1::GrpsMessage* request,
                                                  ::grps::protos::v1::GrpsMessage* response) {
  auto remote_side = context->peer();
  LOG4(INFO, "[ModelMetadata] from " << remote_side);
  rpc_handler_.ModelMetadata(nullptr, request, response);
  return ::grpc::Status::OK;
}
} // namespace netease::grps
