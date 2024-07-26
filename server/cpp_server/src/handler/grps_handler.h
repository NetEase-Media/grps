/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/15
 * Brief  grps service handler.
 */

#pragma once

#include <brpc/controller.h>
#include <grpcpp/impl/codegen/sync_stream.h>

#include "grps.brpc.pb.h"
#include "grps.grpc.pb.h"

namespace netease::grps {
class GrpsHandler {
public:
  GrpsHandler() = default;
  virtual ~GrpsHandler() = default;

  virtual void Predict(::brpc::Controller* controller,
                       const ::grps::protos::v1::GrpsMessage* request,
                       ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void Online(::brpc::Controller* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void Offline(::brpc::Controller* controller,
                       const ::grps::protos::v1::GrpsMessage* request,
                       ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void CheckLiveness(::brpc::Controller* controller,
                             const ::grps::protos::v1::GrpsMessage* request,
                             ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void CheckReadiness(::brpc::Controller* controller,
                              const ::grps::protos::v1::GrpsMessage* request,
                              ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void ServerMetadata(::brpc::Controller* controller,
                              const ::grps::protos::v1::GrpsMessage* request,
                              ::grps::protos::v1::GrpsMessage* response) = 0;

  virtual void ModelMetadata(::brpc::Controller* controller,
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

  void Predict(::brpc::Controller* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response) override;

  void Predict(::grpc::ServerContext* grpc_ctx,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response);

  void PredictStreaming(::grpc::ServerContext* grpc_ctx,
                        const ::grps::protos::v1::GrpsMessage* request,
                        ::grpc::ServerWriter< ::grps::protos::v1::GrpsMessage>* writer);

  void Online(::brpc::Controller* controller,
              const ::grps::protos::v1::GrpsMessage* request,
              ::grps::protos::v1::GrpsMessage* response) override;

  void Offline(::brpc::Controller* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response) override;

  void CheckLiveness(::brpc::Controller* controller,
                     const ::grps::protos::v1::GrpsMessage* request,
                     ::grps::protos::v1::GrpsMessage* response) override;

  void CheckReadiness(::brpc::Controller* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) override;

  void ServerMetadata(::brpc::Controller* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) override;

  void ModelMetadata(::brpc::Controller* controller,
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

  void Predict(::brpc::Controller* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response) override;

  void PredictByHttp(::brpc::Controller* controller,
                     const ::grps::protos::v1::EmptyGrpsMessage* request,
                     ::grps::protos::v1::EmptyGrpsMessage* response,
                     ::google::protobuf::Closure* done);

  void Online(::brpc::Controller* controller,
              const ::grps::protos::v1::GrpsMessage* request,
              ::grps::protos::v1::GrpsMessage* response) override;

  void Offline(::brpc::Controller* controller,
               const ::grps::protos::v1::GrpsMessage* request,
               ::grps::protos::v1::GrpsMessage* response) override;

  void CheckLiveness(::brpc::Controller* controller,
                     const ::grps::protos::v1::GrpsMessage* request,
                     ::grps::protos::v1::GrpsMessage* response) override;

  void CheckReadiness(::brpc::Controller* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) override;

  void ServerMetadata(::brpc::Controller* controller,
                      const ::grps::protos::v1::GrpsMessage* request,
                      ::grps::protos::v1::GrpsMessage* response) override;

  void ModelMetadata(::brpc::Controller* controller,
                     const ::grps::protos::v1::GrpsMessage* request,
                     ::grps::protos::v1::GrpsMessage* response) override;

private:
  GrpsHttpHandler() = default;
};
} // namespace netease::grps