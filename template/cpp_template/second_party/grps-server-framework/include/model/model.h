/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2024/04/26
 * Brief  Model definition.
 */

#pragma once

#include <utility>

#include "batching/batcher.h"
#include "converter/converter.h"
#include "model_infer/inferer.h"

namespace netease::grps {
struct Model {
  std::string name_;
  std::string version_;
  std::shared_ptr<Converter> converter_;
  std::shared_ptr<ModelInferer> inferer_;
  std::shared_ptr<Batcher> batcher_;
  Model() : converter_(nullptr), inferer_(nullptr), batcher_(nullptr) {}
  Model(std::string name,
        std::string version,
        std::shared_ptr<Converter> converter,
        std::shared_ptr<ModelInferer> inferer,
        std::shared_ptr<Batcher> batcher)
      : name_(std::move(name))
      , version_(std::move(version))
      , converter_(std::move(converter))
      , inferer_(std::move(inferer))
      , batcher_(std::move(batcher)) {}
};
} // namespace netease::grps