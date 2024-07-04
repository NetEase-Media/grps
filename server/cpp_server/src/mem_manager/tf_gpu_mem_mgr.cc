/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/04/27
 * Brief  Tensorflow gpu memory manager.
 */

#include "tf_gpu_mem_mgr.h"

#ifdef GRPS_CUDA_ENABLE
#include <cuda_runtime_api.h>

#include "constant.h"
#include "logger/logger.h"

namespace netease::grps {
std::unordered_map<int, float> TfGpuMemMgr::device_mem_limit_rate_ = {};

TfGpuMemMgr::TfGpuMemMgr(const std::vector<int>& devices) : GpuMemMgr(devices) {
  try {
    int device_count;
    cudaError_t error = cudaGetDeviceCount(&device_count);
    if (error != cudaSuccess) {
      throw GpuMemMgrException(cudaGetErrorString(error));
    }
    for (int i : devices_) {
      if (i >= device_count || i < 0) {
        throw GpuMemMgrException("Invalid device idx: " + std::to_string(i) +
                                 ", device count: " + std::to_string(device_count));
      }
    }
    std::string devices_str;
    for (int i : devices_) {
      devices_str += std::to_string(i) + " ";
    }
    LOG4(INFO, "TfGpuMemMgr init success, devices: " << devices_str);
  } catch (const std::exception& e) {
    throw GpuMemMgrException(e.what());
  }
}

std::vector<long long> TfGpuMemMgr::GetMemUsage() {
  // Not support.
  return {};
}

void TfGpuMemMgr::SetMemLimit(float limit) {
  if (limit == -1) {
    LOG4(INFO, "Gpu memory limit is -1, will not limit gpu memory.");
    return;
  }

  for (int i : devices_) {
    cudaDeviceProp deviceProp{};
    const cudaError_t error = cudaGetDeviceProperties(&deviceProp, i);
    if (error != cudaSuccess) {
      throw GpuMemMgrException(cudaGetErrorString(error));
    }
    float rate = limit / float(deviceProp.totalGlobalMem / MIB);
    if (rate > 1.0) {
      rate = 1.0;
    }
    LOG4(INFO, "Setting gpu" << i << " memory limit to " << limit << " MIB, rate: " << rate << "...");
    device_mem_limit_rate_[i] = rate;
  }
}

float TfGpuMemMgr::GetMemLimitRate(int device_idx) {
  if (device_mem_limit_rate_.find(device_idx) == device_mem_limit_rate_.end()) {
    return 1.0;
  }
  return device_mem_limit_rate_[device_idx];
}

void TfGpuMemMgr::MemGc() {
  // Not support.
}
} // namespace netease::grps

#else
namespace netease::grps {
std::unordered_map<int, float> TfGpuMemMgr::device_mem_limit_rate_ = {};

TfGpuMemMgr::TfGpuMemMgr(const std::vector<int>& devices) : GpuMemMgr(devices) {}

std::vector<long long> TfGpuMemMgr::GetMemUsage() {
  throw GpuMemMgrException("TfGpuMemMgr::GetMemUsage() is not supported");
}

void TfGpuMemMgr::SetMemLimit(float limit) {
  throw GpuMemMgrException("TfGpuMemMgr::SetMemLimit() is not supported");
}

float TfGpuMemMgr::GetMemLimitRate(int device_idx) {
  throw GpuMemMgrException("TfGpuMemMgr::GetMemLimitRate() is not supported");
}

void TfGpuMemMgr::MemGc() {
  throw GpuMemMgrException("TfGpuMemMgr::MemGc() is not supported");
}
} // namespace netease::grps

#endif // GRPS_CUDA_ENABLE