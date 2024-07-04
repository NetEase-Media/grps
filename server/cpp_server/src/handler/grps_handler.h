/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/15
 * Brief  grps service handler.
 */

#pragma once

#include <grpcpp/impl/codegen/sync_stream.h>

#include "grps.brpc.pb.h"
#include "grps.grpc.pb.h"

namespace netease::grps {
class GrpsHandler {
public:
  GrpsHandler() = default;
  virtual ~GrpsHandler() = default;

  virtual void Predict(::google::protobuf::RpcController* controller,
                       const ::grps::protos::v1::GrpsMessage* request,
                       ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void Online(::google::protobuf::RpcController* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void Offline(::google::protobuf::RpcController* controller,
                       const ::grps::protos::v1::GrpsMessage* request,
                       ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void CheckLiveness(::google::protobuf::RpcController* controller,
                             const ::grps::protos::v1::GrpsMessage* request,
                             ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void CheckReadiness(::google::protobuf::RpcController* controller,
                              const ::grps::protos::v1::GrpsMessage* request,
                              ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void ServerMetadata(::google::protobuf::RpcController* controller,
                              const ::grps::protos::v1::GrpsMessage* request,
                              ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void ModelMetadata(::google::protobuf::RpcController* controller,
                             const ::grps::protos::v1::GrpsMessage* request,
                             ::grps::protos::v1::GrpsMessage* response) = 0;
};

class GrpsRpcHandler : public GrpsHandler {
public:
  ~GrpsRpcHandler() = default;
  GrpsRpcHandler(const GrpsRpcHandler&) = delete;
  GrpsRpcHandler& operator=(const GrpsRpcHandler&) = delete;
  GrpsRpcHandler(GrpsRpcHandler&&) = delete;
  GrpsRpcHandler& operator=(GrpsRpcHandler&&) = delete;

  static GrpsRpcHandler& Instance() {
    static GrpsRpcHandler instance;
    return instance;
  }

  void Predict(::google::protobuf::RpcController* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response) override;

  void PredictStreaming(::google::protobuf::RpcController* controller,
                        const ::grps::protos::v1::GrpsMessage* request,
                        ::grpc::ServerWriter< ::grps::protos::v1::GrpsMessage>* writer);

  void Online(::google::protobuf::RpcController* controller,
              const ::grps::protos::v1::GrpsMessage* request,
              ::grps::protos::v1::GrpsMessage* response) override;

  void Offline(::google::protobuf::RpcController* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response) override;

  void CheckLiveness(::google::protobuf::RpcController* controller,
                     const ::grps::protos::v1::GrpsMessage* request,
                     ::grps::protos::v1::GrpsMessage* response) override;

  void CheckReadiness(::google::protobuf::RpcController* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) override;

  void ServerMetadata(::google::protobuf::RpcController* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) override;

  void ModelMetadata(::google::protobuf::RpcController* controller,
                     const ::grps::protos::v1::GrpsMessage* request,
                     ::grps::protos::v1::GrpsMessage* response) override;

private:
  GrpsRpcHandler() = default;
};

class GrpsHttpHandler : public GrpsHandler {
public:
  ~GrpsHttpHandler() override = default;
  GrpsHttpHandler(const GrpsHttpHandler&) = delete;
  GrpsHttpHandler& operator=(const GrpsHttpHandler&) = delete;
  GrpsHttpHandler(GrpsHttpHandler&&) = delete;
  GrpsHttpHandler& operator=(GrpsHttpHandler&&) = delete;

  static GrpsHttpHandler& Instance() {
    static GrpsHttpHandler instance;
    return instance;
  }

  void Predict(::google::protobuf::RpcController* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response) override;

  void PredictByHttp(::google::protobuf::RpcController* controller,
                     const ::grps::protos::v1::EmptyGrpsMessage* request,
                     ::grps::protos::v1::EmptyGrpsMessage* response,
                     ::google::protobuf::Closure* done);

  void Online(::google::protobuf::RpcController* controller,
              const ::grps::protos::v1::GrpsMessage* request,
              ::grps::protos::v1::GrpsMessage* response) override;

  void Offline(::google::protobuf::RpcController* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response) override;

  void CheckLiveness(::google::protobuf::RpcController* controller,
                     const ::grps::protos::v1::GrpsMessage* request,
                     ::grps::protos::v1::GrpsMessage* response) override;

  void CheckReadiness(::google::protobuf::RpcController* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) override;

  void ServerMetadata(::google::protobuf::RpcController* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) override;

  void ModelMetadata(::google::protobuf::RpcController* controller,
                     const ::grps::protos::v1::GrpsMessage* request,
                     ::grps::protos::v1::GrpsMessage* response) override;

private:
  GrpsHttpHandler() = default;
};
} // namespace netease::grps