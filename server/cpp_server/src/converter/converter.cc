/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  converter base.
 */

#include "converter.h"

namespace netease::grps {
bool ConverterRegistry::Register(const std::string& name, std::shared_ptr<Converter> converter) {
  return converter_map_.emplace(name, converter).second;
}

std::shared_ptr<Converter> ConverterRegistry::GetConverter(const std::string& name) {
  auto iter = converter_map_.find(name);
  if (iter == converter_map_.end()) {
    return nullptr;
  }
  return iter->second;
}
} // namespace netease::grps
