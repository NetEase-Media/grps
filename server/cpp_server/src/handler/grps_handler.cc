/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/15
 * Brief  grps service handler.
 */

#include "grps_handler.h"

#include <brpc/server.h>
#include <google/protobuf/text_format.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <fstream>
#include <numeric>

#include "common/pb_utils.h"
#include "constant.h"
#include "context/context.h"
#include "executor/executor.h"
#include "logger/logger.h"
#include "monitor/monitor.h"

namespace netease::grps {
static bool online_status = false;

static inline void SetStatus(::grps::protos::v1::GrpsMessage* response,
                             int code,
                             const std::string& message,
                             const ::grps::protos::v1::Status_StatusFlag& status) {
  auto* mutable_status = response->mutable_status();
  mutable_status->set_code(code);
  mutable_status->set_msg(message);
  mutable_status->set_status(status);
}

void GrpsRpcHandler::Predict(::brpc::Controller* controller,
                             const ::grps::protos::v1::GrpsMessage* request,
                             ::grps::protos::v1::GrpsMessage* response) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "Predict");
#endif

  try {
    std::shared_ptr<GrpsContext> ctx_sp =
      std::make_shared<GrpsContext>(request, nullptr, nullptr, nullptr, controller, nullptr);
    auto& ctx = *ctx_sp;
    Executor::Instance().Infer(*request, *response, ctx_sp, request->model());
    if (ctx.has_err()) {
      SetStatus(response, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, ctx.err_msg(), ::grps::protos::v1::Status::FAILURE);
      MONITOR_AVG(REQ_FAIL_RATE, 100);
      LOG4(ERROR, "Predict failed: " << ctx.err_msg());
    } else {
      MONITOR_AVG(REQ_FAIL_RATE, 0);
      SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
    }
  } catch (const std::exception& e) {
    LOG4(ERROR, "Predict failed: " << e.what());
    std::string err_msg = e.what();
#ifdef GRPS_CUDA_ENABLE
    if (err_msg.find("CUDA out of memory") != std::string::npos || err_msg.find("OOM") != std::string::npos) {
      MONITOR_INC(GPU_OOM_COUNT, 1);
    }
#endif
    MONITOR_AVG(REQ_FAIL_RATE, 100);
    SetStatus(response, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, e.what(), ::grps::protos::v1::Status::FAILURE);
  }
}

void GrpsRpcHandler::Predict(::grpc::ServerContext* grpc_ctx,
                             const ::grps::protos::v1::GrpsMessage* request,
                             ::grps::protos::v1::GrpsMessage* response) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "Predict");
#endif

  try {
    std::shared_ptr<GrpsContext> ctx_sp =
      std::make_shared<GrpsContext>(request, nullptr, nullptr, nullptr, nullptr, grpc_ctx);
    auto& ctx = *ctx_sp;
    Executor::Instance().Infer(*request, *response, ctx_sp, request->model());
    if (ctx.has_err()) {
      SetStatus(response, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, ctx.err_msg(), ::grps::protos::v1::Status::FAILURE);
      MONITOR_AVG(REQ_FAIL_RATE, 100);
      LOG4(ERROR, "Predict failed: " << ctx.err_msg());
    } else {
      MONITOR_AVG(REQ_FAIL_RATE, 0);
      SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
    }
  } catch (const std::exception& e) {
    LOG4(ERROR, "Predict failed: " << e.what());
    std::string err_msg = e.what();
#ifdef GRPS_CUDA_ENABLE
    if (err_msg.find("CUDA out of memory") != std::string::npos || err_msg.find("OOM") != std::string::npos) {
      MONITOR_INC(GPU_OOM_COUNT, 1);
    }
#endif
    MONITOR_AVG(REQ_FAIL_RATE, 100);
    SetStatus(response, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, e.what(), ::grps::protos::v1::Status::FAILURE);
  }
}

void GrpsRpcHandler::PredictStreaming(::grpc::ServerContext* grpc_ctx,
                                      const ::grps::protos::v1::GrpsMessage* request,
                                      ::grpc::ServerWriter<::grps::protos::v1::GrpsMessage>* writer) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "PredictStreaming");
#endif

  ::grps::protos::v1::GrpsMessage response;
  try {
    auto ctx_sp = std::make_shared<GrpsContext>(request, writer, nullptr, nullptr, nullptr, grpc_ctx);
    Executor::Instance().Infer(*request, response, ctx_sp, request->model());
    if (ctx_sp->has_err()) {
      SetStatus(&response, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, ctx_sp->err_msg(),
                ::grps::protos::v1::Status::FAILURE);
      MONITOR_AVG(REQ_FAIL_RATE, 100);
      LOG4(ERROR, "Predict failed: " << ctx_sp->err_msg());
    } else {
      MONITOR_AVG(REQ_FAIL_RATE, 0);
      return;
    }
  } catch (const std::exception& e) {
    LOG4(ERROR, "PredictStreaming failed: " << e.what());
    std::string err_msg = e.what();
#ifdef GRPS_CUDA_ENABLE
    if (err_msg.find("CUDA out of memory") != std::string::npos || err_msg.find("OOM") != std::string::npos) {
      MONITOR_INC(GPU_OOM_COUNT, 1);
    }
#endif
    MONITOR_AVG(REQ_FAIL_RATE, 100);
    SetStatus(&response, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, e.what(), ::grps::protos::v1::Status::FAILURE);
  }
  writer->Write(response);
}

void GrpsRpcHandler::Online(::brpc::Controller* controller,
                            const ::grps::protos::v1::GrpsMessage* request,
                            ::grps::protos::v1::GrpsMessage* response) {
  online_status = true;
  SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
}

void GrpsRpcHandler::Offline(::brpc::Controller* controller,
                             const ::grps::protos::v1::GrpsMessage* request,
                             ::grps::protos::v1::GrpsMessage* response) {
  online_status = false;
  SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
}

void GrpsRpcHandler::CheckLiveness(::brpc::Controller* controller,
                                   const ::grps::protos::v1::GrpsMessage* request,
                                   ::grps::protos::v1::GrpsMessage* response) {
  SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
}

void GrpsRpcHandler::CheckReadiness(::brpc::Controller* controller,
                                    const ::grps::protos::v1::GrpsMessage* request,
                                    ::grps::protos::v1::GrpsMessage* response) {
  if (online_status) {
    SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
  } else {
    SetStatus(response, brpc::HTTP_STATUS_SERVICE_UNAVAILABLE, "Service Unavailable",
              ::grps::protos::v1::Status::FAILURE);
  }
}

void GrpsRpcHandler::ServerMetadata(::brpc::Controller* controller,
                                    const ::grps::protos::v1::GrpsMessage* request,
                                    ::grps::protos::v1::GrpsMessage* response) {
  std::fstream input("./conf/server.yml", std::ios::in);
  std::string meta((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  std::string meta_pure = YAML::Dump(YAML::Load(meta)); // remove comments.
  input.close();
  response->set_str_data(std::move(meta_pure));
  input.open("./conf/inference.yml", std::ios::in);
  meta.assign((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  meta_pure = YAML::Dump(YAML::Load(meta)); // remove comments.
  input.close();
  response->mutable_str_data()->append("\n");
  response->mutable_str_data()->append(meta_pure);
}

void GrpsRpcHandler::ModelMetadata(::brpc::Controller* controller,
                                   const ::grps::protos::v1::GrpsMessage* request,
                                   ::grps::protos::v1::GrpsMessage* response) {
  const std::string& model_name = request->str_data();

  std::fstream input("./conf/inference.yml", std::ios::in);
  std::string server_meta((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  input.close();
  std::string model_meta;
  YAML::Node config = YAML::Load(server_meta);
  YAML::Node models = config["models"];
  for (const auto& model : models) {
    if (model["name"].as<std::string>() == model_name) {
      model_meta = YAML::Dump(model);
    }
    break;
  }

  if (model_meta.empty()) {
    SetStatus(response, brpc::HTTP_STATUS_NOT_FOUND, "Not Found", ::grps::protos::v1::Status::FAILURE);
    response->set_str_data("");
  } else {
    SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
    response->set_str_data(model_meta);
  }
}

void GrpsHttpHandler::Predict(::brpc::Controller* cntl,
                              const ::grps::protos::v1::GrpsMessage* request,
                              ::grps::protos::v1::GrpsMessage* response) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "Predict");
#endif
  const auto& content_type = cntl->http_request().content_type();

  // Predict.
  try {
    std::shared_ptr<GrpsContext> ctx_sp = std::make_shared<GrpsContext>(request, nullptr, nullptr, cntl);
    auto& ctx = *ctx_sp;
    Executor::Instance().Infer(*request, *response, ctx_sp, request->model());
    if (ctx.has_err()) {
      SetStatus(response, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, ctx.err_msg(), ::grps::protos::v1::Status::FAILURE);
      MONITOR_AVG(REQ_FAIL_RATE, 100);
      LOG4(ERROR, "Predict failed: " << ctx.err_msg());
    } else {
      MONITOR_AVG(REQ_FAIL_RATE, 0);
      SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
    }
  } catch (const std::exception& e) {
    LOG4(ERROR, "Predict failed: " << e.what());
    std::string err_msg = e.what();
#ifdef GRPS_CUDA_ENABLE
    if (err_msg.find("CUDA out of memory") != std::string::npos || err_msg.find("OOM") != std::string::npos) {
      MONITOR_INC(GPU_OOM_COUNT, 1);
    }
#endif
    MONITOR_AVG(REQ_FAIL_RATE, 100);
    SetStatus(response, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, e.what(), ::grps::protos::v1::Status::FAILURE);
  }

  // Set response to http body.
  cntl->http_response().set_content_type("application/json");
  cntl->response_attachment().append(Pb2json(*response));
}

// Parse shape and record val into flat vector from ndarray by recursive call.
static void ParseNDArrayShapeAndVal(const rapidjson::Value& nd_array,
                                    std::vector<int64_t>& shape,
                                    std::vector<float>& val,
                                    int dim_idx) {
  if (nd_array.IsArray()) {
    if (nd_array.Size() <= 0) {
      throw std::invalid_argument("NDArray some array size <= 0.");
    }
    if (shape.size() <= dim_idx) {
      shape.push_back(nd_array.Size());
    } else {
      if (shape[dim_idx] != nd_array.Size()) {
        throw std::invalid_argument("NDArray some array size not match.");
      }
    }
    for (const auto& arr : nd_array.GetArray()) {
      ParseNDArrayShapeAndVal(arr, shape, val, dim_idx + 1);
    }
  } else if (nd_array.IsNumber()) {
    val.push_back(nd_array.GetFloat());
  } else {
    LOG4(ERROR, "NDArray some array is not a number or array.");
    throw std::invalid_argument("NDArray some array is not a number or array.");
  }
}

// Parse NDArray from http body to generic tensor.
// NDArray is a multi dimensional array, dtype is default to float32.
// NDArray format:
// {
//   "ndarray": [[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]] // dynamic multi dimensional array
// }
static bool ParseNDArray(const rapidjson::Value& nd_array, ::grps::protos::v1::GrpsMessage& req, std::string& err) {
  if (!nd_array.IsArray()) {
    err = "NDArray is not an array";
    LOG4(ERROR, err);
    return false;
  }

  // Parse NDArray to shape and val.
  std::vector<int64_t> shape;
  std::vector<float> val;
  try {
    ParseNDArrayShapeAndVal(nd_array, shape, val, 0);
  } catch (const std::exception& e) {
    err = "Parse NDArray failed: ";
    err += e.what();
    LOG4(ERROR, err);
    return false;
  }

  // Convert to generic tensor.
  auto* g_tensors = req.mutable_gtensors();
  g_tensors->Clear();
  auto* g_tensor = g_tensors->add_tensors();
  g_tensor->set_dtype(::grps::protos::v1::DataType::DT_FLOAT32);
  for (const auto& dim : shape) {
    g_tensor->add_shape(dim);
  }
  for (const auto& v : val) {
    g_tensor->add_flat_float32(v);
  }
  return true;
}

static void GenNDArray(const std::vector<int64_t>& shape,
                       const std::vector<float>& val,
                       rapidjson::Value& nd_array,
                       rapidjson::Document::AllocatorType& allocator) {
  if (shape.size() == 1) {
    nd_array.SetArray();
    for (size_t i = 0; i < shape[0]; ++i) {
      nd_array.PushBack(val[i], allocator);
    }
  } else {
    nd_array.SetArray();
    size_t sub_arr_len = std::accumulate(shape.begin() + 1, shape.end(), 1, std::multiplies<>());
    for (size_t i = 0; i < shape[0]; ++i) {
      rapidjson::Value sub_arr;
      GenNDArray(std::vector<int64_t>(shape.begin() + 1, shape.end()),
                 std::vector<float>(val.begin() + i * sub_arr_len, val.begin() + (i + 1) * sub_arr_len), sub_arr,
                 allocator);
      nd_array.PushBack(sub_arr, allocator);
    }
  }
}

static bool GenNDArrayFromGrpsMsg(const ::grps::protos::v1::GrpsMessage& res,
                                  rapidjson::Value& nd_array,
                                  std::string& err,
                                  rapidjson::Document::AllocatorType& allocator) {
  if (res.gtensors().tensors_size() == 0) {
    LOG4(ERROR, "No tensors in output. Cannot convert to ndarray.");
    err = "No tensors in output. Cannot convert to ndarray.";
    return false;
  }

  if (res.gtensors().tensors_size() != 1) {
    LOG4(ERROR, "NDArray only support one tensor");
    err = "NDArray only support one tensor";
    return false;
  }

  const auto& g_tensor = res.gtensors().tensors(0);
  if (g_tensor.dtype() != ::grps::protos::v1::DataType::DT_FLOAT32) {
    err = "NDArray only support float32";
    LOG4(ERROR, err);
    return false;
  }

  if (g_tensor.shape_size() == 0 && g_tensor.flat_float32_size() == 1) { // scalar
    nd_array.SetFloat(g_tensor.flat_float32(0));
    return true;
  }

  if (g_tensor.shape_size() <= 0) {
    err = "NDArray shape size <= 0";
    LOG4(ERROR, err);
    return false;
  }
  if (g_tensor.flat_float32_size() <= 0) {
    err = "NDArray flat float size <= 0";
    LOG4(ERROR, err);
    return false;
  }
  if (g_tensor.flat_float32_size() !=
      std::accumulate(g_tensor.shape().begin(), g_tensor.shape().end(), 1, std::multiplies<>())) {
    err = "NDArray shape and flat float size not match";
    LOG4(ERROR, err);
    return false;
  }

  // Convert to NDArray.
  nd_array.SetArray();
  std::vector<int64_t> shape(g_tensor.shape().begin(), g_tensor.shape().end());
  std::vector<float> val(g_tensor.flat_float32().begin(), g_tensor.flat_float32().end());
  GenNDArray(shape, val, nd_array, allocator);
  return true;
}

static void CustomizedPredictHttp(brpc::Controller* cntl,
                                  brpc::ClosureGuard& done_guard,
                                  bool is_streaming,
                                  const std::string& model_name = "") {
  ::grps::protos::v1::GrpsMessage req;
  ::grps::protos::v1::GrpsMessage res;

  if (is_streaming) {
    cntl->http_response().set_content_type(
      GlobalConfig::Instance().server_config().interface.customized_predict_http.streaming_ctrl.res_content_type);
    // done_guard reset will clear cntl->request_attachment(), so we need to save it.
    butil::IOBuf tmp;
    tmp.append(cntl->request_attachment().movable());
    auto pa = cntl->CreateProgressiveAttachment();
    done_guard.reset(nullptr);
    cntl->request_attachment().append(tmp.movable()); // restore cntl->request_attachment().
    auto ctx_sp = std::make_shared<GrpsContext>(&req, nullptr, &pa, cntl);
    auto& ctx = *ctx_sp;
    try {
      Executor::Instance().Infer(req, res, ctx_sp, model_name);
      if (ctx.has_err()) {
        MONITOR_AVG(REQ_FAIL_RATE, 100);
        LOG4(ERROR, "Predict failed: " << ctx.err_msg());
      } else {
        MONITOR_AVG(REQ_FAIL_RATE, 0);
      }
    } catch (const std::exception& e) {
      std::string err_msg(e.what());
      LOG4(ERROR, "Predict failed: " << err_msg);
      MONITOR_AVG(REQ_FAIL_RATE, 100);
#ifdef GRPS_CUDA_ENABLE
      if (err_msg.find("CUDA out of memory") != std::string::npos || err_msg.find("OOM") != std::string::npos) {
        MONITOR_INC(GPU_OOM_COUNT, 1);
      }
#endif
      pa->Write(err_msg.c_str(), err_msg.size());
    }
    return;
  }

  try {
    std::shared_ptr<GrpsContext> ctx_sp = std::make_shared<GrpsContext>(&req, nullptr, nullptr, cntl);
    auto& ctx = *ctx_sp;
    Executor::Instance().Infer(req, res, ctx_sp, model_name);
    if (ctx.has_err()) {
      MONITOR_AVG(REQ_FAIL_RATE, 100);
      LOG4(ERROR, "Predict failed: " << ctx.err_msg());
    } else {
      MONITOR_AVG(REQ_FAIL_RATE, 0);
    }
  } catch (const std::exception& e) {
    std::string err_msg(e.what());
    LOG4(ERROR, "Predict failed: " << err_msg);
    MONITOR_AVG(REQ_FAIL_RATE, 100);
#ifdef GRPS_CUDA_ENABLE
    if (err_msg.find("CUDA out of memory") != std::string::npos || err_msg.find("OOM") != std::string::npos) {
      MONITOR_INC(GPU_OOM_COUNT, 1);
    }
#endif
    cntl->http_response().set_status_code(brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR);
    cntl->http_response().set_content_type("text/plain");
    cntl->response_attachment().append(err_msg);
    return;
  }
}

static bool IfHttpStreaming(::brpc::Controller* cntl) {
  bool is_streaming = false;
  auto& streaming_ctrl = GlobalConfig::Instance().server_config().interface.customized_predict_http.streaming_ctrl;
  if (streaming_ctrl.ctrl_mode == GlobalConfig::ServerConfig::StreamingCtrlMode::kQueryParam) {
    auto streaming_query_arg = cntl->http_request().uri().GetQuery(streaming_ctrl.ctrl_key);
    if (streaming_query_arg != nullptr) {
      // to lower
      std::string streaming_query_arg_lower;
      std::transform(streaming_query_arg->begin(), streaming_query_arg->end(),
                     std::back_inserter(streaming_query_arg_lower), ::tolower);
      if (streaming_query_arg_lower == "true") {
        is_streaming = true;
      }
    }
  } else if (streaming_ctrl.ctrl_mode == GlobalConfig::ServerConfig::StreamingCtrlMode::kHeaderParam) {
    auto streaming_header = cntl->http_request().GetHeader(streaming_ctrl.ctrl_key);
    if (streaming_header != nullptr) {
      // to lower
      std::string streaming_header_lower;
      std::transform(streaming_header->begin(), streaming_header->end(), std::back_inserter(streaming_header_lower),
                     ::tolower);
      if (streaming_header_lower == "true") {
        is_streaming = true;
      }
    }
  } else if (streaming_ctrl.ctrl_mode == GlobalConfig::ServerConfig::StreamingCtrlMode::kBodyParam) {
    rapidjson::Document body_doc;
    body_doc.Parse(cntl->request_attachment().to_string().c_str());
    if (body_doc.HasParseError()) {
      LOG4(ERROR, "Parse json failed: " << body_doc.GetParseError());
      return false;
    }
    auto streaming_body = body_doc.FindMember(streaming_ctrl.ctrl_key.c_str());
    if (streaming_body != body_doc.MemberEnd() && streaming_body->value.IsBool() && streaming_body->value.GetBool()) {
      is_streaming = true;
    }
  }
  return is_streaming;
}

void GrpsHttpHandler::PredictByHttp(::brpc::Controller* cntl,
                                    const ::grps::protos::v1::EmptyGrpsMessage* request,
                                    ::grps::protos::v1::EmptyGrpsMessage* response,
                                    ::google::protobuf::Closure* done) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "Predict");
#endif
  brpc::ClosureGuard done_guard(done);

  const auto& content_type = cntl->http_request().content_type();

  // Check if streaming request.
  bool is_streaming = IfHttpStreaming(cntl);

  // Check if ret ndarray.
  bool ret_ndarray = false;
  auto ret_ndarray_query_arg = cntl->http_request().uri().GetQuery("return-ndarray");
  if (ret_ndarray_query_arg != nullptr) {
    // to lower
    std::string ret_ndarray_query_arg_lower;
    std::transform(ret_ndarray_query_arg->begin(), ret_ndarray_query_arg->end(),
                   std::back_inserter(ret_ndarray_query_arg_lower), ::tolower);
    if (ret_ndarray_query_arg_lower == "true") {
      ret_ndarray = true;
    }
  }

  // Get model arg.
  std::string model;
  auto model_query_arg = cntl->http_request().uri().GetQuery("model");
  if (model_query_arg != nullptr) {
    model = *model_query_arg;
  }

  if (GlobalConfig::Instance().server_config()._is_set.customized_predict_http &&
      GlobalConfig::Instance().server_config().interface.customized_predict_http.customized_body) {
    CustomizedPredictHttp(cntl, done_guard, is_streaming, model);
    return;
  }

  // Is streaming and ret ndarray are not supported at the same time.
  if (is_streaming && ret_ndarray) {
    LOG4(ERROR, "Streaming and ret ndarray are not supported at the same time.");
    ::grps::protos::v1::GrpsMessage res;
    SetStatus(&res, brpc::HTTP_STATUS_BAD_REQUEST,
              "Bad Request, err: Streaming and ret ndarray are not supported at the same time.",
              ::grps::protos::v1::Status::FAILURE);
    cntl->response_attachment().append(Pb2json(res));
    MONITOR_AVG(REQ_FAIL_RATE, 100);
    return;
  }

  // Parse true request from http body.
  auto true_req_ptr = std::make_unique<::grps::protos::v1::GrpsMessage>();
  auto& true_req = *true_req_ptr;
  if (content_type == "application/json") {
    // Json2pb(cntl->request_attachment().to_string(), &true_req);
    auto body_doc_ptr = std::make_unique<rapidjson::Document>();
    auto& body_doc = *body_doc_ptr;
    body_doc.Parse(cntl->request_attachment().to_string().c_str());
    if (body_doc.HasParseError()) {
      LOG4(ERROR, "Parse json failed: " << body_doc.GetParseError());
      ::grps::protos::v1::GrpsMessage res;
      SetStatus(&res, brpc::HTTP_STATUS_BAD_REQUEST, "Bad Request, err: Parse json failed.",
                ::grps::protos::v1::Status::FAILURE);
      cntl->response_attachment().append(Pb2json(res));
      MONITOR_AVG(REQ_FAIL_RATE, 100);
      return;
    }
    if (body_doc.HasMember("str_data") || body_doc.HasMember("gtensors") || body_doc.HasMember("gmap")) {
      std::string err;
      Json2pb(cntl->request_attachment().to_string(), &true_req, &err);
      if (!err.empty()) {
        ::grps::protos::v1::GrpsMessage res;
        SetStatus(&res, brpc::HTTP_STATUS_BAD_REQUEST, "Bad Request, err: " + err, ::grps::protos::v1::Status::FAILURE);
        cntl->response_attachment().append(Pb2json(res));
        MONITOR_AVG(REQ_FAIL_RATE, 100);
        return;
      }
    } else if (body_doc.HasMember("ndarray")) {
      std::string err;
      if (body_doc["ndarray"].IsNumber()) { // Scalar
        auto* g_tensor = true_req.mutable_gtensors()->add_tensors();
        g_tensor->add_flat_float32(body_doc["ndarray"].GetFloat());
        g_tensor->set_dtype(::grps::protos::v1::DataType::DT_FLOAT32);
      } else if (body_doc["ndarray"].IsArray()) { // NDArray
        if (!ParseNDArray(body_doc["ndarray"], true_req, err)) {
          ::grps::protos::v1::GrpsMessage res;
          SetStatus(&res, brpc::HTTP_STATUS_BAD_REQUEST, "Bad Request, err: " + err,
                    ::grps::protos::v1::Status::FAILURE);
          cntl->response_attachment().append(Pb2json(res));
          MONITOR_AVG(REQ_FAIL_RATE, 100);
          return;
        }
      } else {
        LOG4(ERROR, "NDArray is not number or array.");
        ::grps::protos::v1::GrpsMessage res;
        SetStatus(&res, brpc::HTTP_STATUS_BAD_REQUEST, "Bad Request, err: NDArray is not number or array.",
                  ::grps::protos::v1::Status::FAILURE);
        cntl->response_attachment().append(Pb2json(res));
        MONITOR_AVG(REQ_FAIL_RATE, 100);
        return;
      }
    } else if (body_doc.HasMember("bin_data")) {
      LOG4(ERROR, "bin_data should use application/octet-stream format.");
      ::grps::protos::v1::GrpsMessage res;
      SetStatus(&res, brpc::HTTP_STATUS_BAD_REQUEST,
                "Bad Request, err: bin_data should use application/octet-stream format.",
                ::grps::protos::v1::Status::FAILURE);
      cntl->response_attachment().append(Pb2json(res));
      MONITOR_AVG(REQ_FAIL_RATE, 100);
      return;
    } else {
      LOG4(ERROR, "Have no legal member in json body.");
      ::grps::protos::v1::GrpsMessage res;
      SetStatus(&res, brpc::HTTP_STATUS_BAD_REQUEST, "Bad Request, err: Have no legal member in json body.",
                ::grps::protos::v1::Status::FAILURE);
      cntl->response_attachment().append(Pb2json(res));
      MONITOR_AVG(REQ_FAIL_RATE, 100);
      return;
    }
    // Get model name from json body if exist and override model query arg.
    if (body_doc.HasMember("model") && body_doc["model"].IsString()) {
      model = body_doc["model"].GetString();
    }
  } else if (content_type == "application/octet-stream") {
    auto bytes = cntl->request_attachment().to_string();
    true_req.set_bin_data(std::move(bytes));
  } else {
    LOG4(ERROR, "Content type is not supported: " << content_type);
    ::grps::protos::v1::GrpsMessage res;
    SetStatus(&res, brpc::HTTP_STATUS_BAD_REQUEST, "Bad Request, err: Content type is not supported.",
              ::grps::protos::v1::Status::FAILURE);
    cntl->response_attachment().append(Pb2json(res));
    MONITOR_AVG(REQ_FAIL_RATE, 100);
    return;
  }
#ifdef GRPS_DEBUG
  std::string pb_str;
  ::google::protobuf::TextFormat::PrintToString(true_req, &pb_str);
  LOG4(INFO, "True predict request: " << pb_str);
#endif

  // Streaming predict.
  auto true_res_ptr = std::make_unique<::grps::protos::v1::GrpsMessage>();
  auto& true_res = *true_res_ptr;
  if (is_streaming) {
    cntl->http_response().set_content_type(
      GlobalConfig::Instance().server_config().interface.customized_predict_http.streaming_ctrl.res_content_type);
    auto pa = cntl->CreateProgressiveAttachment();
    done_guard.reset(nullptr);
    auto ctx_sp = std::make_shared<GrpsContext>(&true_req, nullptr, &pa, cntl);
    auto& ctx = *ctx_sp;
    try {
      Executor::Instance().Infer(true_req, true_res, ctx_sp, model);
      if (ctx.has_err()) {
        SetStatus(&true_res, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, ctx.err_msg(),
                  ::grps::protos::v1::Status::FAILURE);
        MONITOR_AVG(REQ_FAIL_RATE, 100);
        LOG4(ERROR, "Predict failed: " << ctx.err_msg());
      } else {
        MONITOR_AVG(REQ_FAIL_RATE, 0);
        return;
      }
    } catch (const std::exception& e) {
      std::string err_msg(e.what());
      LOG4(ERROR, "Predict failed: " << err_msg);
      MONITOR_AVG(REQ_FAIL_RATE, 100);
#ifdef GRPS_CUDA_ENABLE
      if (err_msg.find("CUDA out of memory") != std::string::npos || err_msg.find("OOM") != std::string::npos) {
        MONITOR_INC(GPU_OOM_COUNT, 1);
      }
#endif
      SetStatus(&true_res, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, e.what(), ::grps::protos::v1::Status::FAILURE);
    }
    ctx.StreamingRespond(true_res);
    return;
  }

  // Predict.
  bool has_err = false;
  try {
    auto ctx_sp = std::make_shared<GrpsContext>(&true_req, nullptr, nullptr, cntl);
    auto& ctx = *ctx_sp;
    Executor::Instance().Infer(true_req, true_res, ctx_sp, model);
    if (ctx.has_err()) {
      SetStatus(&true_res, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, ctx.err_msg(), ::grps::protos::v1::Status::FAILURE);
      has_err = true;
      LOG4(ERROR, "Predict failed: " << ctx.err_msg());
    } else {
      SetStatus(&true_res, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
    }
  } catch (const std::exception& e) {
    std::string err_msg(e.what());
    LOG4(ERROR, "Predict failed: " << err_msg);
    MONITOR_AVG(REQ_FAIL_RATE, 100);
#ifdef GRPS_CUDA_ENABLE
    if (err_msg.find("CUDA out of memory") != std::string::npos || err_msg.find("OOM") != std::string::npos) {
      MONITOR_INC(GPU_OOM_COUNT, 1);
    }
#endif
    SetStatus(&true_res, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, e.what(), ::grps::protos::v1::Status::FAILURE);
    cntl->http_response().set_content_type("application/json");
    cntl->response_attachment().append(Pb2json(true_res));
    return;
  }

  // Set response to http body.
  if (ret_ndarray) { // return ndarray format.
    auto res_doc_ptr = std::make_unique<rapidjson::Document>();
    auto& res_doc = *res_doc_ptr;
    res_doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = res_doc.GetAllocator();
    res_doc.AddMember("status", rapidjson::Value(rapidjson::kObjectType), allocator);
    auto& status = res_doc["status"];
    status.AddMember("code", rapidjson::Value(true_res.status().code()), allocator);
    status.AddMember("msg", rapidjson::Value(true_res.status().msg().c_str(), allocator), allocator);
    status.AddMember("status", rapidjson::Value(rapidjson::kStringType), allocator);
    status["status"].SetString(::grps::protos::v1::Status::StatusFlag_Name(true_res.status().status()).c_str(),
                               allocator);
    res_doc.AddMember("ndarray", rapidjson::Value(rapidjson::kObjectType), allocator);
    auto& nd_array = res_doc["ndarray"];
    std::string err;
    if (!GenNDArrayFromGrpsMsg(true_res, nd_array, err, allocator)) {
      LOG4(ERROR, "Gen ndarray failed: " << err);
      SetStatus(&true_res, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, err, ::grps::protos::v1::Status::FAILURE);
      cntl->http_response().set_content_type("application/json");
      true_res.clear_str_data();
      true_res.clear_bin_data();
      true_res.clear_gtensors();
      cntl->response_attachment().append(Pb2json(true_res));
      has_err = true;
    } else {
      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      writer.SetIndent(' ', 1);
      res_doc.Accept(writer);
      cntl->http_response().set_content_type("application/json");
      cntl->response_attachment().append(buffer.GetString());
    }
  } else {
    switch (true_res.data_oneof_case()) {
      case ::grps::protos::v1::GrpsMessage::kBinData: {
        cntl->http_response().set_content_type("application/octet-stream");
        cntl->response_attachment().append(true_res.bin_data());
        break;
      }
      case ::grps::protos::v1::GrpsMessage::kStrData:
      case ::grps::protos::v1::GrpsMessage::kGtensors:
      case ::grps::protos::v1::GrpsMessage::kGmap: {
        cntl->http_response().set_content_type("application/json");
        cntl->response_attachment().append(Pb2json(true_res));
        break;
      }
      case ::grps::protos::v1::GrpsMessage::DATA_ONEOF_NOT_SET: {
        // LOG4(WARN, "Response data is empty");
        cntl->http_response().set_content_type("application/json");
        cntl->response_attachment().append(Pb2json(true_res));
        break;
      }
      default: {
        LOG4(ERROR, "Response data type error");
        true_res.Clear();
        SetStatus(&true_res, brpc::HTTP_STATUS_INTERNAL_SERVER_ERROR, "Response data type error",
                  ::grps::protos::v1::Status::FAILURE);
        cntl->http_response().set_content_type("application/json");
        cntl->response_attachment().append(Pb2json(true_res));
        has_err = true;
        break;
      }
    }
  }
  if (has_err) {
    MONITOR_AVG(REQ_FAIL_RATE, 100);
  } else {
    MONITOR_AVG(REQ_FAIL_RATE, 0);
  }
}

void GrpsHttpHandler::Online(::brpc::Controller* cntl,
                             const ::grps::protos::v1::GrpsMessage* request,
                             ::grps::protos::v1::GrpsMessage* response) {
  online_status = true;
  SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
  cntl->http_response().set_content_type("application/json");
  cntl->response_attachment().append(Pb2json(*response));
}

void GrpsHttpHandler::Offline(::brpc::Controller* cntl,
                              const ::grps::protos::v1::GrpsMessage* request,
                              ::grps::protos::v1::GrpsMessage* response) {
  online_status = false;
  SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
  cntl->http_response().set_content_type("application/json");
  cntl->response_attachment().append(Pb2json(*response));
}

void GrpsHttpHandler::CheckLiveness(::brpc::Controller* cntl,
                                    const ::grps::protos::v1::GrpsMessage* request,
                                    ::grps::protos::v1::GrpsMessage* response) {
  SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
  cntl->http_response().set_content_type("application/json");
  cntl->response_attachment().append(Pb2json(*response));
}

void GrpsHttpHandler::CheckReadiness(::brpc::Controller* cntl,
                                     const ::grps::protos::v1::GrpsMessage* request,
                                     ::grps::protos::v1::GrpsMessage* response) {
  if (online_status) {
    SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
  } else {
    SetStatus(response, brpc::HTTP_STATUS_SERVICE_UNAVAILABLE, "Service Unavailable",
              ::grps::protos::v1::Status::FAILURE);
  }
  cntl->http_response().set_content_type("application/json");
  cntl->response_attachment().append(Pb2json(*response));
}

void GrpsHttpHandler::ServerMetadata(::brpc::Controller* cntl,
                                     const ::grps::protos::v1::GrpsMessage* request,
                                     ::grps::protos::v1::GrpsMessage* response) {
  std::fstream input("./conf/server.yml", std::ios::in);
  std::string meta((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  std::string meta_pure = YAML::Dump(YAML::Load(meta)); // remove comments.
  input.close();
  response->set_str_data(std::move(meta_pure));
  input.open("./conf/inference.yml", std::ios::in);
  meta.assign((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  meta_pure = YAML::Dump(YAML::Load(meta)); // remove comments.
  input.close();
  response->mutable_str_data()->append("\n");
  response->mutable_str_data()->append(meta_pure);
  SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
  cntl->http_response().set_content_type("application/json");
  cntl->response_attachment().append(Pb2json(*response));
}

void GrpsHttpHandler::ModelMetadata(::brpc::Controller* cntl,
                                    const ::grps::protos::v1::GrpsMessage* request,
                                    ::grps::protos::v1::GrpsMessage* response) {
  const std::string& model_name = request->str_data();

  std::fstream input("./conf/inference.yml", std::ios::in);
  std::string server_meta((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  input.close();
  std::string model_meta;
  YAML::Node config = YAML::Load(server_meta);
  YAML::Node models = config["models"];
  for (const auto& model : models) {
    if (model["name"].as<std::string>() == model_name) {
      model_meta = YAML::Dump(model);
    }
    break;
  }

  if (model_meta.empty()) {
    SetStatus(response, brpc::HTTP_STATUS_NOT_FOUND, "Not Found", ::grps::protos::v1::Status::FAILURE);
    cntl->http_response().set_content_type("application/json");
    cntl->response_attachment().append(Pb2json(*response));
  } else {
    SetStatus(response, brpc::HTTP_STATUS_OK, "OK", ::grps::protos::v1::Status::SUCCESS);
    response->set_str_data(model_meta);
    cntl->http_response().set_content_type("application/json");
    cntl->response_attachment().append(Pb2json(*response));
  }
}
} // namespace netease::grps
