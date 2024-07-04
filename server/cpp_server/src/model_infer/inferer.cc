/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  model inferer base.
 */

#include "inferer.h"

namespace netease::grps {
bool ModelInfererRegistry::Register(const std::string& name, const std::shared_ptr<ModelInferer>& inferer) {
  return g_inferer_map_.emplace(name, inferer).second;
}

std::shared_ptr<ModelInferer> ModelInfererRegistry::GetInferer(const std::string& name) {
  auto iter = g_inferer_map_.find(name);
  if (iter == g_inferer_map_.end()) {
    return nullptr;
  }
  return iter->second;
}
} // namespace netease::grps