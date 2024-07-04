/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2024/06/11
 * Brief  Cuda helper.
 */

#pragma once

#include <cuda_runtime.h>

#include <iostream>
#include <thread>
#include <utility>

#include "logger/logger.h"

namespace netease::grps {
class CudaException : public std::exception {
public:
  explicit CudaException(std::string message) : message_(std::move(message)) {}
  ~CudaException() override = default;
  [[nodiscard]] const char* what() const noexcept override {
    static std::string err_message;
    err_message = "[CudaException] " + message_;
    return err_message.c_str();
  }

private:
  std::string message_;
};

inline void CudaCheck(cudaError_t ret) {
  if (ret != cudaSuccess) {
    auto err_str = std::string(cudaGetErrorString(ret));
    LOG4(ERROR, err_str);
    if (ret == cudaErrorMemoryAllocation) {
      throw CudaException("OOM, " + err_str);
    } else {
      abort(); // Unrecoverable error.
    }
  }
}

#if CUDA_VERSION < 10000
inline void CudaSleep(cudaStream_t stream, cudaError_t status, void* sleep)
#else
inline void CudaSleep(void* sleep)
#endif
{
  std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(*static_cast<int*>(sleep)));
}

class CudaEvent;

//!
//! \class CudaStream
//! \brief Managed CUDA stream
//!
class CudaStream {
public:
  CudaStream() { CudaCheck(cudaStreamCreate(&stream_)); }

  CudaStream(const CudaStream&) = delete;

  CudaStream& operator=(const CudaStream&) = delete;

  CudaStream(CudaStream&&) = delete;

  CudaStream& operator=(CudaStream&&) = delete;

  ~CudaStream() { CudaCheck(cudaStreamDestroy(stream_)); }

  [[nodiscard]] cudaStream_t Get() const { return stream_; }

  void Wait(CudaEvent& event);

  void sleep(int* ms) {
#if CUDA_VERSION < 10000
    CudaCheck(cudaStreamAddCallback(stream_, CudaSleep, ms, 0));
#else
    CudaCheck(cudaLaunchHostFunc(stream_, CudaSleep, ms));
#endif
  }

private:
  cudaStream_t stream_{};
};

//!
//! \class CudaEvent
//! \brief Managed CUDA event
//!
class CudaEvent {
public:
  explicit CudaEvent(bool blocking = true) {
    const unsigned int flags = blocking ? cudaEventBlockingSync : cudaEventDefault;
    CudaCheck(cudaEventCreateWithFlags(&event_, flags));
  }

  CudaEvent(const CudaEvent&) = delete;

  CudaEvent& operator=(const CudaEvent&) = delete;

  CudaEvent(CudaEvent&&) = delete;

  CudaEvent& operator=(CudaEvent&&) = delete;

  ~CudaEvent() { CudaCheck(cudaEventDestroy(event_)); }

  cudaEvent_t Get() const { return event_; }

  void Record(const CudaStream& stream) { CudaCheck(cudaEventRecord(event_, stream.Get())); }

  void Synchronize() { CudaCheck(cudaEventSynchronize(event_)); }

  // Returns time elapsed time in milliseconds
  float operator-(const CudaEvent& e) const {
    float time{0};
    CudaCheck(cudaEventElapsedTime(&time, e.Get(), Get()));
    return time;
  }

private:
  cudaEvent_t event_{};
};

inline void CudaStream::Wait(CudaEvent& event) {
  CudaCheck(cudaStreamWaitEvent(stream_, event.Get(), 0));
}

//!
//! \class CudaBuffer
//! \brief Managed buffer for host and device
//!
template <typename A, typename D>
class CudaBuffer {
public:
  CudaBuffer() = default;

  CudaBuffer(const CudaBuffer&) = delete;

  CudaBuffer& operator=(const CudaBuffer&) = delete;

  CudaBuffer(CudaBuffer&& rhs) noexcept {
    Reset(rhs.ptr_, rhs.size_);
    rhs.ptr_ = nullptr;
    rhs.size_ = 0;
  }

  CudaBuffer& operator=(CudaBuffer&& rhs) noexcept {
    if (this != &rhs) {
      Reset(rhs.ptr_, rhs.size_);
      rhs.ptr_ = nullptr;
      rhs.size_ = 0;
    }
    return *this;
  }

  ~CudaBuffer() { Reset(); }

  explicit CudaBuffer(size_t size) {
    A()(&ptr_, size);
    size_ = size;
  }

  void Allocate(size_t size) {
    Reset();
    A()(&ptr_, size);
    size_ = size;
  }

  void Reset(void* ptr = nullptr, size_t size = 0) {
    if (ptr_) {
      D()(ptr_);
    }
    ptr_ = ptr;
    size_ = size;
  }

  [[nodiscard]] void* Get() const { return ptr_; }

private:
  void* ptr_{nullptr};
  size_t size_{0};
};

struct DeviceAllocator {
  void operator()(void** ptr, size_t size) { CudaCheck(cudaMalloc(ptr, size)); }
};

struct DeviceDeallocator {
  void operator()(void* ptr) { CudaCheck(cudaFree(ptr)); }
};

struct HostAllocator {
  // void operator()(void** ptr, size_t size) { CudaCheck(cudaMallocHost(ptr, size)); }
  void operator()(void** ptr, size_t size) { *ptr = malloc(size); }
};

struct HostDeallocator {
  // void operator()(void* ptr) { CudaCheck(cudaFreeHost(ptr)); }
  void operator()(void* ptr) { free(ptr); }
};

using DeviceBuffer = CudaBuffer<DeviceAllocator, DeviceDeallocator>;

using HostBuffer = CudaBuffer<HostAllocator, HostDeallocator>;

inline void H2D(CudaStream& stream, HostBuffer& host_buffer, DeviceBuffer& device_buffer, size_t size) {
  CudaCheck(cudaMemcpyAsync(device_buffer.Get(), host_buffer.Get(), size, cudaMemcpyHostToDevice, stream.Get()));
}

inline void D2H(CudaStream& stream, DeviceBuffer& device_buffer, HostBuffer& host_buffer, size_t size) {
  CudaCheck(cudaMemcpyAsync(host_buffer.Get(), device_buffer.Get(), size, cudaMemcpyDeviceToHost, stream.Get()));
}
} // namespace netease::grps
