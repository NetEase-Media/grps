/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/15
 * Brief  Dag of inference.
 */

#pragma once

#include <string>
#include <unordered_map>

#include "config/global_config.h"
#include "context/context.h"
#include "converter/converter.h"
#include "dag/node.h"
#include "grps.pb.h"
#include "model/model.h"
#include "model_infer/inferer.h"

namespace netease::grps {
class InferDag {
public:
  class InferDagException : public std::exception {
  public:
    explicit InferDagException(std::string message) : message_(std::move(message)) {}
    ~InferDagException() override = default;
    [[nodiscard]] const char* what() const noexcept override {
      static std::string err_message;
      err_message = "[InferDagException] " + message_;
      return err_message.c_str();
    }

  private:
    std::string message_;
  };

  explicit InferDag(const std::string& name) : name_(name) {}
  virtual ~InferDag() = default;

  virtual void BuildDag(const std::vector<GlobalConfig::InferenceConfig::NodeConfig>& node_configs,
                        const std::unordered_map<std::string, Model>& models) = 0;

  virtual void Infer(const ::grps::protos::v1::GrpsMessage& input,
                     ::grps::protos::v1::GrpsMessage& output,
                     GrpsContext& ctx) = 0;

  virtual void Infer(const ::grps::protos::v1::GrpsMessage& input,
                     ::grps::protos::v1::GrpsMessage& output,
                     const std::shared_ptr<GrpsContext>& ctx_sp) = 0;

  [[nodiscard]] const std::string& name() const { return name_; }

protected:
  std::string name_;
};

// Sequential DAG, which means the models will be executed in sequence.
class SequentialDag : public InferDag {
public:
  explicit SequentialDag(const std::string& name) : InferDag(name) {}
  ~SequentialDag() override = default;

  void BuildDag(const std::vector<GlobalConfig::InferenceConfig::NodeConfig>& node_configs,
                const std::unordered_map<std::string, Model>& models) override;

  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             GrpsContext& ctx) override;

  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             const std::shared_ptr<GrpsContext>& ctx_sp) override;

private:
  std::vector<std::shared_ptr<Node>> sequence_;
};

// TODO(zhaochaochao): Graph DAG, which means the models will be executed in graph.
class GraphDag : public InferDag {
public:
  GraphDag(const std::string& name) : InferDag(name) {}
  ~GraphDag() override = default;

  void BuildDag(const std::vector<GlobalConfig::InferenceConfig::NodeConfig>& node_configs,
                const std::unordered_map<std::string, Model>& models) override;

  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             GrpsContext& ctx) override;

  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             const std::shared_ptr<GrpsContext>& ctx_sp) override;
};
} // namespace netease::grps