/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/04/27
 * Brief  Tensorflow gpu memory manager.
 */

#include <unordered_map>

#include "mem_manager/gpu_mem_mgr.h"

namespace netease::grps {
class TfGpuMemMgr : public GpuMemMgr {
public:
  explicit TfGpuMemMgr(const std::vector<int>& devices);
  ~TfGpuMemMgr() override = default;

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
   * Get gpu memory limit rate.
   * @param device_idx: Device index.
   * @return Gpu memory limit rate.
   */
  static float GetMemLimitRate(int device_idx);

  /**
   * Gpu memory garbage collection.
   */
  void MemGc() override;

private:
  static std::unordered_map<int, float> device_mem_limit_rate_;
};
} // namespace netease::grps