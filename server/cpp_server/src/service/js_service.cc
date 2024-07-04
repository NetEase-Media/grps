/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/09/13
 * Brief  js service.
 */

#include "js_service.h"

#include <brpc/builtin/common.h>
#include <brpc/builtin/flot_min_js.h>
#include <brpc/builtin/jquery_min_js.h>
#include <brpc/restful.h>
#include <brpc/server.h>

static const char* g_last_modified = "Wed, 16 Sep 2015 01:25:30 GMT";

namespace netease::grps {
static void SetExpires(brpc::HttpHeader* header, time_t seconds) {
  char buf[256];
  time_t now = time(0);
  brpc::Time2GMT(now, buf, sizeof(buf));
  header->SetHeader("Date", buf);
  brpc::Time2GMT(now + seconds, buf, sizeof(buf));
  header->SetHeader("Expires", buf);
}

void JsServiceImpl::JqueryMinJs(::google::protobuf::RpcController* controller,
                                const ::grps::protos::v1::EmptyGrpsMessage* request,
                                ::grps::protos::v1::EmptyGrpsMessage* response,
                                ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto* cntl = (brpc::Controller*)controller;
  cntl->http_response().set_content_type("application/javascript");
  SetExpires(&cntl->http_response(), 600);

  const std::string* ims = cntl->http_request().GetHeader("If-Modified-Since");
  if (ims != NULL && *ims == g_last_modified) {
    cntl->http_response().set_status_code(brpc::HTTP_STATUS_NOT_MODIFIED);
    return;
  }
  cntl->http_response().SetHeader("Last-Modified", g_last_modified);

  if (SupportGzip(cntl)) {
    cntl->http_response().SetHeader("Content-Encoding", "gzip");
    cntl->response_attachment().append(brpc::jquery_min_js_iobuf_gzip());
  } else {
    cntl->response_attachment().append(brpc::jquery_min_js_iobuf());
  }
}

void JsServiceImpl::FloatMinJs(::google::protobuf::RpcController* controller,
                               const ::grps::protos::v1::EmptyGrpsMessage* request,
                               ::grps::protos::v1::EmptyGrpsMessage* response,
                               ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto* cntl = (brpc::Controller*)controller;
  cntl->http_response().set_content_type("application/javascript");
  SetExpires(&cntl->http_response(), 80000);

  const std::string* ims = cntl->http_request().GetHeader("If-Modified-Since");
  if (ims != nullptr && *ims == g_last_modified) {
    cntl->http_response().set_status_code(brpc::HTTP_STATUS_NOT_MODIFIED);
    return;
  }
  cntl->http_response().SetHeader("Last-Modified", g_last_modified);

  if (SupportGzip(cntl)) {
    cntl->http_response().SetHeader("Content-Encoding", "gzip");
    cntl->response_attachment().append(brpc::flot_min_js_iobuf_gzip());
  } else {
    cntl->response_attachment().append(brpc::flot_min_js_iobuf());
  }
}
} // namespace netease::grps