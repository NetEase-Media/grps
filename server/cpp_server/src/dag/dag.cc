/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/15
 * Brief  Dag of inference.
 */

#include "dag/dag.h"

#include "logger/logger.h"

namespace netease::grps {
void SequentialDag::BuildDag(const std::vector<GlobalConfig::InferenceConfig::NodeConfig>& node_configs,
                             const std::unordered_map<std::string, Model>& models) {
  LOG4(INFO, "Build sequential dag: " << name_);
  for (const auto& node : node_configs) {
    if (node.type == "model") {
      auto model = models.find(node.model);
      if (model == models.end()) {
        LOG4(ERROR, "Model not found: " << node.model);
        throw InferDagException("Model not found: " + node.model);
      }
      sequence_.emplace_back(std::make_shared<ModelNode>(node.name, model->second.inferer_, model->second.converter_,
                                                         model->second.batcher_));
    } else {
      LOG4(ERROR, "Unknown node type: " << node.type);
      throw InferDagException("Unknown node type: " + node.type);
    }
  }
  std::stringstream ss;
  for (const auto& node : sequence_) {
    ss << node->name() << " -> ";
  }
  LOG4(INFO, "Build sequential dag successfully, sequence is: " << ss.str());
}

void SequentialDag::Infer(const ::grps::protos::v1::GrpsMessage& input,
                          ::grps::protos::v1::GrpsMessage& output,
                          GrpsContext& ctx) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "Infer sequential dag: " << name_);
#endif
  for (int i = 0; i < sequence_.size(); ++i) {
    if (i == 0) {
      sequence_[i]->Process(input, output, ctx);
    } else {
      sequence_[i]->Process(output, output, ctx);
    }
    if (ctx.has_err()) {
      return;
    }
  }
}

void SequentialDag::Infer(const ::grps::protos::v1::GrpsMessage& input,
                          ::grps::protos::v1::GrpsMessage& output,
                          const std::shared_ptr<GrpsContext>& ctx_sp) {
#ifdef GRPS_DEBUG
  LOG4(INFO, "Infer sequential dag: " << name_);
#endif
  for (int i = 0; i < sequence_.size(); ++i) {
    if (i == 0) {
      sequence_[i]->Process(input, output, ctx_sp);
    } else {
      sequence_[i]->Process(output, output, ctx_sp);
    }
    if (ctx_sp->has_err()) {
      return;
    }
  }
}

void GraphDag::BuildDag(const std::vector<GlobalConfig::InferenceConfig::NodeConfig>& node_configs,
                        const std::unordered_map<std::string, Model>& models) {}

void GraphDag::Infer(const ::grps::protos::v1::GrpsMessage& input,
                     ::grps::protos::v1::GrpsMessage& output,
                     GrpsContext& ctx) {}

void GraphDag::Infer(const ::grps::protos::v1::GrpsMessage& input,
                     ::grps::protos::v1::GrpsMessage& output,
                     const std::shared_ptr<GrpsContext>& ctx_sp) {}
} // namespace netease::grps