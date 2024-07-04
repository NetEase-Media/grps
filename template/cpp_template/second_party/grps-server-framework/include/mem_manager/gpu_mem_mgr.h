/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/21
 * Brief  Gpu memory manager base class.
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace netease::grps {
class GpuMemMgr {
public:
  class GpuMemMgrException : public std::exception {
  public:
    explicit GpuMemMgrException(std::string message) : message_(std::move(message)) {}
    ~GpuMemMgrException() override = default;
    [[nodiscard]] const char* what() const noexcept override {
      static std::string err_message;
      err_message = "[GpuMemMgrException] " + message_;
      return err_message.c_str();
    }

  private:
    std::string message_;
  };

  GpuMemMgr() : devices_() {}
  explicit GpuMemMgr(const std::vector<int>& devices) : devices_(devices){};
  virtual ~GpuMemMgr() = default;

  /**
   * Get gpu memory usage.
   * @return Gpu memory usage with MiB of all devices.
   */
  virtual std::vector<long long> GetMemUsage() = 0;

  /**
   * Set gpu memory limit.
   * @param limit: Gpu memory limit threshold with MiB.
   */
  virtual void SetMemLimit(float limit) = 0;

  /**
   * Gpu memory garbage collection.
   */
  virtual void MemGc() = 0;

  /**
   * Get devices idx list.
   * @return devices idx list.
   */
  [[nodiscard]] const std::vector<int>& GetDevices() const { return devices_; }

protected:
  std::vector<int> devices_; // devices idx list.
};
} // namespace netease::grps
