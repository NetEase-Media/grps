/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/21
 * Brief  Torch gpu memory manager.
 */

#pragma once

#include "mem_manager/gpu_mem_mgr.h"

namespace netease::grps {
class TorchGpuMemMgr : public GpuMemMgr {
public:
  explicit TorchGpuMemMgr(const std::vector<int>& devices);

  ~TorchGpuMemMgr() override = default;

  /**
   * Get gpu memory usage.
   * @return Gpu memory usage with MiB of all devices.
   */
  std::vector<long long> GetMemUsage() override;

  /**
   * Set gpu memory limit.
   * @param limit: Gpu memory limit threshold with MiB.
   */
  void SetMemLimit(float limit) override;

  /**
   * Gpu memory garbage collection.
   */
  void MemGc() override;
};
} // namespace netease::grps
