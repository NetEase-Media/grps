/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/15
 * Brief  Node of DAG.
 */

#include "node.h"

#include <butil/time.h>

#include "logger/logger.h"

namespace netease::grps {
void ModelNode::Process(const ::grps::protos::v1::GrpsMessage& input,
                        ::grps::protos::v1::GrpsMessage& output,
                        netease::grps::GrpsContext& ctx) {
  auto begin = butil::gettimeofday_us();
  ctx.set_converter(converter_.get());
  ctx.set_inferer(model_inferer_.get());

  if (batcher_) {
    batcher_->Infer(input, output, ctx);
    return;
  }

  if (converter_) {
    std::vector<std::pair<std::string, TensorWrapper>> inp_tensors;
    std::vector<std::pair<std::string, TensorWrapper>> out_tensors;

    converter_->PreProcess(input, inp_tensors, ctx);
    if (ctx.has_err()) {
      return;
    }
    auto preprocess_end = butil::gettimeofday_us();

    model_inferer_->Infer(inp_tensors, out_tensors, ctx);
    if (ctx.has_err()) {
      return;
    }
    auto infer_end = butil::gettimeofday_us();

    output.Clear();
    converter_->PostProcess(out_tensors, output, ctx);
    if (ctx.has_err()) {
      return;
    }
    auto postprocess_end = butil::gettimeofday_us();

    LOG4(INFO, "Model(" << name_ << "), preprocess latency: " << preprocess_end - begin
                        << "us, infer latency: " << infer_end - preprocess_end
                        << "us, postprocess latency: " << postprocess_end - infer_end << "us");
  } else {
    output.Clear();
    model_inferer_->Infer(input, output, ctx);
    if (ctx.has_err()) {
      return;
    }
    auto infer_end = butil::gettimeofday_us();
    LOG4(INFO, "Model(" << name_ << "), infer latency: " << infer_end - begin << "us");
  }
}

void ModelNode::Process(const ::grps::protos::v1::GrpsMessage& input,
                        ::grps::protos::v1::GrpsMessage& output,
                        const std::shared_ptr<GrpsContext>& ctx_sp) {
  auto begin = butil::gettimeofday_us();
  ctx_sp->set_converter(converter_.get());
  ctx_sp->set_inferer(model_inferer_.get());

  if (batcher_) {
    batcher_->Infer(input, output, ctx_sp);
    return;
  }

  if (converter_) {
    std::vector<std::pair<std::string, TensorWrapper>> inp_tensors;
    std::vector<std::pair<std::string, TensorWrapper>> out_tensors;

    converter_->PreProcess(input, inp_tensors, *ctx_sp);
    if (ctx_sp->has_err()) {
      return;
    }
    auto preprocess_end = butil::gettimeofday_us();

    model_inferer_->Infer(inp_tensors, out_tensors, *ctx_sp);
    if (ctx_sp->has_err()) {
      return;
    }
    auto infer_end = butil::gettimeofday_us();

    output.Clear();
    converter_->PostProcess(out_tensors, output, *ctx_sp);
    if (ctx_sp->has_err()) {
      return;
    }
    auto postprocess_end = butil::gettimeofday_us();

    LOG4(INFO, "Model(" << name_ << "), preprocess latency: " << preprocess_end - begin
                        << "us, infer latency: " << infer_end - preprocess_end
                        << "us, postprocess latency: " << postprocess_end - infer_end << "us");
  } else {
    output.Clear();
    model_inferer_->Infer(input, output, *ctx_sp);
    if (ctx_sp->has_err()) {
      return;
    }
    auto infer_end = butil::gettimeofday_us();
    LOG4(INFO, "Model(" << name_ << "), infer latency: " << infer_end - begin << "us");
  }
}

void ModelNode::Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
                        std::vector<::grps::protos::v1::GrpsMessage>& output,
                        GrpsContext& ctx) {}

void ModelNode::Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
                        std::vector<::grps::protos::v1::GrpsMessage>& output,
                        const std::shared_ptr<GrpsContext>& ctx_sp) {}

void MergerNode::Process(const ::grps::protos::v1::GrpsMessage& input,
                         ::grps::protos::v1::GrpsMessage& output,
                         GrpsContext& ctx) {}

void MergerNode::Process(const ::grps::protos::v1::GrpsMessage& input,
                         ::grps::protos::v1::GrpsMessage& output,
                         const std::shared_ptr<GrpsContext>& ctx_sp) {}

void MergerNode::Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
                         std::vector<::grps::protos::v1::GrpsMessage>& output,
                         GrpsContext& ctx) {}

void MergerNode::Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
                         std::vector<::grps::protos::v1::GrpsMessage>& output,
                         const std::shared_ptr<GrpsContext>& ctx_sp) {}

void SplitterNode::Process(const ::grps::protos::v1::GrpsMessage& input,
                           ::grps::protos::v1::GrpsMessage& output,
                           GrpsContext& ctx) {}

void SplitterNode::Process(const ::grps::protos::v1::GrpsMessage& input,
                           ::grps::protos::v1::GrpsMessage& output,
                           const std::shared_ptr<GrpsContext>& ctx_sp) {}

void SplitterNode::Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
                           std::vector<::grps::protos::v1::GrpsMessage>& output,
                           GrpsContext& ctx) {}

void SplitterNode::Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
                           std::vector<::grps::protos::v1::GrpsMessage>& output,
                           const std::shared_ptr<GrpsContext>& ctx_sp) {}
} // namespace netease::grps
