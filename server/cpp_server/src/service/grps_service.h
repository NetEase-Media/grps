/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/06
 * Brief  grps service.
 */

#pragma once

#include <boost/asio/thread_pool.hpp>

#include "grps.brpc.pb.h"
#include "grps.grpc.pb.h"
#include "handler/grps_handler.h"

namespace netease::grps {

extern std::unique_ptr<::boost::asio::thread_pool> g_predict_threadpool;

// Grps brpc service(including http service).
class GrpsBrpcServiceImpl : public ::grps::protos::v1::GrpsBrpcService {
public:
  enum class HandlerType { kRPC, kHTTP };

  explicit GrpsBrpcServiceImpl(HandlerType handler_type)
      : rpc_handler_(GrpsRpcHandler::Instance())
      , http_handler_(GrpsHttpHandler::Instance())
      , handler_type_(handler_type) {}
  ~GrpsBrpcServiceImpl() override = default;

  void Predict(::google::protobuf::RpcController* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response,
               ::google::protobuf::Closure* done) override;

  void PredictByHttp(::google::protobuf::RpcController* controller,
                     const ::grps::protos::v1::EmptyGrpsMessage* request,
                     ::grps::protos::v1::EmptyGrpsMessage* response,
                     ::google::protobuf::Closure* done) override;

  void Online(::google::protobuf::RpcController* controller,
              const ::grps::protos::v1::GrpsMessage* request,
              ::grps::protos::v1::GrpsMessage* response,
              ::google::protobuf::Closure* done) override;

  void Offline(::google::protobuf::RpcController* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response,
               ::google::protobuf::Closure* done) override;

  void CheckLiveness(::google::protobuf::RpcController* controller,
                     const ::grps::protos::v1::GrpsMessage* request,
                     ::grps::protos::v1::GrpsMessage* response,
                     ::google::protobuf::Closure* done) override;

  void CheckReadiness(::google::protobuf::RpcController* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response,
                      ::google::protobuf::Closure* done) override;

  void ServerMetadata(::google::protobuf::RpcController* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response,
                      ::google::protobuf::Closure* done) override;

  void ModelMetadata(::google::protobuf::RpcController* controller,
                     const ::grps::protos::v1::GrpsMessage* request,
                     ::grps::protos::v1::GrpsMessage* response,
                     ::google::protobuf::Closure* done) override;

private:
  GrpsRpcHandler& rpc_handler_;
  GrpsHttpHandler& http_handler_;
  HandlerType handler_type_ = HandlerType::kRPC;
};

// Grps grpc service.
class GrpsGrpcServiceImpl : public ::grps::protos::v1::GrpsService::Service {
public:
  GrpsGrpcServiceImpl() : rpc_handler_(GrpsRpcHandler::Instance()) {}
  ~GrpsGrpcServiceImpl() override = default;

  ::grpc::Status Predict(::grpc::ServerContext* context,
                         const ::grps::protos::v1::GrpsMessage* request,
                         ::grps::protos::v1::GrpsMessage* response) override;

  ::grpc::Status PredictStreaming(::grpc::ServerContext* context,
                                  const ::grps::protos::v1::GrpsMessage* request,
                                  ::grpc::ServerWriter<::grps::protos::v1::GrpsMessage>* writer) override;

  ::grpc::Status Online(::grpc::ServerContext* context,
                        const ::grps::protos::v1::GrpsMessage* request,
                        ::grps::protos::v1::GrpsMessage* response) override;

  ::grpc::Status Offline(::grpc::ServerContext* context,
                         const ::grps::protos::v1::GrpsMessage* request,
                         ::grps::protos::v1::GrpsMessage* response) override;

  ::grpc::Status CheckLiveness(::grpc::ServerContext* context,
                               const ::grps::protos::v1::GrpsMessage* request,
                               ::grps::protos::v1::GrpsMessage* response) override;

  ::grpc::Status CheckReadiness(::grpc::ServerContext* context,
                                const ::grps::protos::v1::GrpsMessage* request,
                                ::grps::protos::v1::GrpsMessage* response) override;

  ::grpc::Status ServerMetadata(::grpc::ServerContext* context,
                                const ::grps::protos::v1::GrpsMessage* request,
                                ::grps::protos::v1::GrpsMessage* response) override;

  ::grpc::Status ModelMetadata(::grpc::ServerContext* context,
                               const ::grps::protos::v1::GrpsMessage* request,
                               ::grps::protos::v1::GrpsMessage* response) override;

private:
  GrpsRpcHandler& rpc_handler_;
};
} // namespace netease::grps