/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/21
 * Brief  Torch gpu memory manager.
 */

#include "mem_manager/torch_gpu_mem_mgr.h"

#ifdef GRPS_CUDA_ENABLE

#include <c10/cuda/CUDACachingAllocator.h>
#include <cuda_runtime_api.h>

#include "constant.h"
#include "logger/logger.h"

namespace netease::grps {
TorchGpuMemMgr::TorchGpuMemMgr(const std::vector<int>& devices) : GpuMemMgr(devices) {
  try {
    int device_count;
    cudaError_t error = cudaGetDeviceCount(&device_count);
    if (error != cudaSuccess) {
      throw GpuMemMgrException(cudaGetErrorString(error));
    }
    c10::cuda::CUDACachingAllocator::init(device_count);
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
    LOG4(INFO, "TorchGpuMemMgr init success, devices: " << devices_str);
  } catch (const std::exception& e) {
    throw GpuMemMgrException(e.what());
  }
}

std::vector<long long> TorchGpuMemMgr::GetMemUsage() {
  std::vector<long long> mem_usage;
  for (int i : devices_) {
    auto dev_stat = c10::cuda::CUDACachingAllocator::getDeviceStats(i);
    long long occupied = 0;
    for (const auto& stat : dev_stat.allocated_bytes) {
      occupied += stat.current;
    }
    long long reserved = 0;
    for (const auto& stat : dev_stat.reserved_bytes) {
      reserved += stat.current;
    }
    mem_usage.push_back((occupied + reserved) / MIB);
  }
  return mem_usage;
}

void TorchGpuMemMgr::SetMemLimit(float limit) {
  if (limit == -1) {
    LOG4(INFO, "Gpu memory limit is -1, will not limit gpu memory.");
    return;
  }
  try {
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
      c10::cuda::CUDACachingAllocator::setMemoryFraction(rate, i);
    }
  } catch (const std::exception& e) {
    throw GpuMemMgrException(e.what());
  }
}

void TorchGpuMemMgr::MemGc() {
  try {
    c10::cuda::CUDACachingAllocator::emptyCache();
    LOG4(INFO, "Gpu memory garbage collection");
  } catch (const std::exception& e) {
    throw GpuMemMgrException(e.what());
  }
}
} // namespace netease::grps
#else
namespace netease::grps {
TorchGpuMemMgr::TorchGpuMemMgr(const std::vector<int>& devices) : GpuMemMgr(devices) {}

std::vector<long long> TorchGpuMemMgr::GetMemUsage() {
  throw GpuMemMgrException("TorchGpuMemMgr::GetMemUsage() is not supported");
}

void TorchGpuMemMgr::SetMemLimit(float limit) {
  throw GpuMemMgrException("TorchGpuMemMgr::SetMemLimit() is not supported");
}

void TorchGpuMemMgr::MemGc() {
  throw GpuMemMgrException("TorchGpuMemMgr::MemGc() is not supported");
}
} // namespace netease::grps
#endif
