/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/09/13
 * Brief  js service.
 */

#pragma once

#include "grps.brpc.pb.h"

namespace netease::grps {
class JsServiceImpl : public ::grps::protos::v1::JsService {
  void JqueryMinJs(::google::protobuf::RpcController* controller,
                   const ::grps::protos::v1::EmptyGrpsMessage* request,
                   ::grps::protos::v1::EmptyGrpsMessage* response,
                   ::google::protobuf::Closure* done) override;

  void FloatMinJs(::google::protobuf::RpcController* controller,
                  const ::grps::protos::v1::EmptyGrpsMessage* request,
                  ::grps::protos::v1::EmptyGrpsMessage* response,
                  ::google::protobuf::Closure* done) override;
};
} // namespace netease::grps