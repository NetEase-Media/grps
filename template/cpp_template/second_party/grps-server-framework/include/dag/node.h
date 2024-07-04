/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/15
 * Brief  Node of DAG.
 */

#pragma once

#include <string>
#include <vector>

#include "batching/batcher.h"
#include "context/context.h"
#include "converter/converter.h"
#include "grps.pb.h"
#include "model_infer/inferer.h"

namespace netease::grps {
class Node {
public:
  class NodeException : public std::exception {
  public:
    explicit NodeException(std::string message) : message_(std::move(message)) {}
    ~NodeException() override = default;
    [[nodiscard]] const char* what() const noexcept override {
      static std::string err_message;
      err_message = "[NodeException] " + message_;
      return err_message.c_str();
    }

  private:
    std::string message_;
  };

  explicit Node(const std::string& name) : name_(name) {}

  virtual ~Node() = default;

  virtual void Process(const ::grps::protos::v1::GrpsMessage& input,
                       ::grps::protos::v1::GrpsMessage& output,
                       GrpsContext& ctx) = 0;

  virtual void Process(const ::grps::protos::v1::GrpsMessage& input,
                       ::grps::protos::v1::GrpsMessage& output,
                       const std::shared_ptr<GrpsContext>& ctx_sp) = 0;

  virtual void Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
                       std::vector<::grps::protos::v1::GrpsMessage>& output,
                       GrpsContext& ctx) = 0;

  virtual void Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
                       std::vector<::grps::protos::v1::GrpsMessage>& output,
                       const std::shared_ptr<GrpsContext>& ctx_sp) = 0;

  [[nodiscard]] const std::string& name() const { return name_; }

protected:
  std::string name_;
};

// Model node, which means the node is a model.
class ModelNode : public Node {
public:
  explicit ModelNode(const std::string& name)
      : Node(name), model_inferer_(nullptr), converter_(nullptr), batcher_(nullptr) {}

  ModelNode(const std::string& name,
            const std::shared_ptr<ModelInferer>& model_inferer,
            const std::shared_ptr<Converter>& converter,
            const std::shared_ptr<Batcher>& batcher)
      : Node(name), model_inferer_(model_inferer), converter_(converter), batcher_(batcher) {}

  ~ModelNode() override = default;

  void Process(const ::grps::protos::v1::GrpsMessage& input,
               ::grps::protos::v1::GrpsMessage& output,
               GrpsContext& ctx) override;

  void Process(const ::grps::protos::v1::GrpsMessage& input,
               ::grps::protos::v1::GrpsMessage& output,
               const std::shared_ptr<GrpsContext>& ctx_sp) override;

  void Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
               std::vector<::grps::protos::v1::GrpsMessage>& output,
               GrpsContext& ctx) override;

  void Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
               std::vector<::grps::protos::v1::GrpsMessage>& output,
               const std::shared_ptr<GrpsContext>& ctx_sp) override;

private:
  std::shared_ptr<ModelInferer> model_inferer_;
  std::shared_ptr<Converter> converter_;
  std::shared_ptr<Batcher> batcher_;
};

// TODO(zhaochaochao): Add merger node.
class MergerNode : public Node {
public:
  explicit MergerNode(const std::string& name) : Node(name) {}
  ~MergerNode() override = default;

  void Process(const ::grps::protos::v1::GrpsMessage& input,
               ::grps::protos::v1::GrpsMessage& output,
               GrpsContext& ctx) override;

  void Process(const ::grps::protos::v1::GrpsMessage& input,
               ::grps::protos::v1::GrpsMessage& output,
               const std::shared_ptr<GrpsContext>& ctx_sp) override;

  void Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
               std::vector<::grps::protos::v1::GrpsMessage>& output,
               GrpsContext& ctx) override;

  void Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
               std::vector<::grps::protos::v1::GrpsMessage>& output,
               const std::shared_ptr<GrpsContext>& ctx_sp) override;
};

// TODO(zhaochaochao): Add splitter node.
class SplitterNode : public Node {
public:
  explicit SplitterNode(const std::string& name) : Node(name) {}
  ~SplitterNode() override = default;

  void Process(const ::grps::protos::v1::GrpsMessage& input,
               ::grps::protos::v1::GrpsMessage& output,
               GrpsContext& ctx) override;

  void Process(const ::grps::protos::v1::GrpsMessage& input,
               ::grps::protos::v1::GrpsMessage& output,
               const std::shared_ptr<GrpsContext>& ctx_sp) override;

  void Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
               std::vector<::grps::protos::v1::GrpsMessage>& output,
               GrpsContext& ctx) override;

  void Process(const std::vector<::grps::protos::v1::GrpsMessage>& input,
               std::vector<::grps::protos::v1::GrpsMessage>& output,
               const std::shared_ptr<GrpsContext>& ctx_sp) override;
};
} // namespace netease::grps