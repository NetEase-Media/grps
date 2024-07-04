/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2024/06/07
 * Brief  TensorRT cuda helper.
 */

#pragma once

#include <NvInfer.h>
#include <NvInferRuntime.h>
#include <NvInferVersion.h>
#include <cuda_runtime.h>

#include <iostream>
#include <thread>
#include <utility>

#include "logger/logger.h"
#include "model_infer/cuda_helper.h"

namespace netease::grps {
template <typename T>
struct TrtDestroyer {
  void operator()(T* t) { t->destroy(); }
};

template <typename T>
using TrtUniquePtr = std::unique_ptr<T, TrtDestroyer<T> >;

enum class EventType : int {
  kINPUT_START = 0,
  kINPUT_END = 1,
  kCOMPUTE_START = 2,
  kCOMPUTE_END = 3,
  kOUTPUT_START = 4,
  kOUTPUT_END = 5,
  kINPUT_CONSUMED = 6,
  kNUM = 7
};

using MultiTrtEvent = std::array<CudaEvent, static_cast<int>(EventType::kNUM)>;

template <typename T1, typename T2>
inline T1 RoundUp(T1 m, T2 n) {
  return ((m + n - 1) / n) * n;
}

/**
 * Calc volume of tensor.
 * @param d: Dimensions of tensor.
 * @return The volume of tensor.
 */
inline size_t Volume(const nvinfer1::Dims& d) {
  return std::accumulate(d.d, d.d + d.nbDims, 1, std::multiplies<>());
}

/**
 * Calc volume of tensor.
 * @param dims: Dimensions of tensor.
 * @param vec_dim:
 * @param comps
 * @return The volume of tensor.
 */
inline size_t Volume(nvinfer1::Dims dims, int vec_dim, int comps) {
  if (vec_dim != -1) {
    dims.d[vec_dim] = RoundUp(dims.d[vec_dim], comps);
  }
  return Volume(dims);
}

// Get tensorrt dtype size.
inline size_t TrtDtypeSize(nvinfer1::DataType data_type) {
  size_t data_type_size = 0;
  switch (data_type) {
#if NV_TENSORRT_MAJOR >= 7
    case nvinfer1::DataType::kBOOL:
#endif
    case nvinfer1::DataType::kINT8:
#if NV_TENSORRT_MAJOR >= 8 && NV_TENSORRT_MINOR >= 5
    case nvinfer1::DataType::kUINT8:
#endif
#if NV_TENSORRT_MAJOR >= 8 && NV_TENSORRT_MINOR >= 6
    case nvinfer1::DataType::kFP8:
#endif
      data_type_size = 1;
      break;
    case nvinfer1::DataType::kHALF:
      data_type_size = 2;
      break;
    case nvinfer1::DataType::kINT32:
    case nvinfer1::DataType::kFLOAT:
      data_type_size = 4;
      break;
    default:
      LOG4(ERROR, "Unsupported data type: " << static_cast<int>(data_type));
      break;
  }
  return data_type_size;
}
} // namespace netease::grps
