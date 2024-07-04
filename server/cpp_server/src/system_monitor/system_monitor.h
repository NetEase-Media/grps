/*
 * Copyright 2022 netease. All rights reserved.
 * Author jiangwenxuan01@corp.netease.com
 * Date   2023/09/22
 * Brief  SystemMonitor used to monitor cpu, memory, gpu.
 */

#include <atomic>
#include <memory>
#include <vector>

#ifdef GRPS_CUDA_ENABLE
#include "mem_manager/gpu_mem_mgr.h"
#endif
namespace netease::grps {
class SystemMonitor {
public:
  class SystemMonitorException : public std::exception {
  public:
    explicit SystemMonitorException(std::string message) : message_(std::move(message)) {}
    ~SystemMonitorException() override = default;
    [[nodiscard]] const char* what() const noexcept override {
      static std::string err_message;
      err_message = "[SystemMonitorException] " + message_;
      return err_message.c_str();
    }

  private:
    std::string message_;
  };

  ~SystemMonitor() { Stop(); };
  SystemMonitor(const SystemMonitor&) = delete;
  SystemMonitor& operator=(const SystemMonitor&) = delete;
  SystemMonitor(SystemMonitor&&) = delete;
  SystemMonitor& operator=(SystemMonitor&&) = delete;

  static SystemMonitor& Instance() {
    static SystemMonitor instance;
    return instance;
  }

  void Init();

  void Start();

  void Stop();

  void MonitorFunc();

#ifdef GRPS_CUDA_ENABLE
  unsigned int GetGpuUsage(int gpu_id);
  unsigned long long GetGpuMemUsage(int gpu_id);
#endif

private:
  int interval_step_ = 0;                 // Interval(second) of per step.
  int stat_step_ = 0;                     //  System statistic interval steps count.
  int gc_step_ = 0;                       // Gpu memory garbage collection interval steps count.
  int mem_lim_mib_ = 0;                   // Gpu memory limit threshold with MiB.
  bool gpu_monitor_enable_ = false;       // Whether to enable gpu monitor.
  bool mem_manager_enable_ = false;       // Whether to enable gpu memory manager. Default is false.
  std::vector<int> devices_ = {};         // Gpu devices idx list.
  std::string mem_manager_type_ = "none"; // Gpu memory manager type. Default is "none".
  std::atomic<bool> stop_ = false;

#ifdef GRPS_CUDA_ENABLE
  std::shared_ptr<GpuMemMgr> gpu_mem_mgr_ = nullptr; // Gpu memory manager.
  std::vector<int> cuda_visible_devices_ = {};       // Cuda visible devices idx list.
#endif
  SystemMonitor() = default;
};
} // namespace netease::grps
