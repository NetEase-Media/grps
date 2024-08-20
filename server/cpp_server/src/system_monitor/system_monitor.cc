/*
 * Copyright 2022 netease. All rights reserved.
 * Author jiangwenxuan01@corp.netease.com
 * Date   2023/09/22
 * Brief  SystemMonitor used to monitor cpu, memory, gpu.
 */

#include "system_monitor.h"

#ifdef GRPS_CUDA_ENABLE
#include <nvml.h>
#endif
#include <unistd.h>

#include <regex>

#include "config/global_config.h"
#include "constant.h"
#include "logger/logger.h"
#include "monitor/monitor.h"
#ifdef GRPS_TF_ENABLE
#include "mem_manager/tf_gpu_mem_mgr.h"
#endif
#ifdef GRPS_TORCH_ENABLE
#include "mem_manager/torch_gpu_mem_mgr.h"
#endif

namespace netease::grps {
struct ThreadProc {
  unsigned long utime;  // time in user mode
  unsigned long stime;  // time in kernel mode
  unsigned long vm_rss; // resident set size (in kB)
  ThreadProc() : utime(0), stime(0), vm_rss(0) {}
};

// get the total cpu time from /proc/stat
static unsigned long long GetTotalCpuTime() {
  FILE* fp = fopen("/proc/stat", "r");
  if (fp == nullptr) {
    LOG4(FATAL, "Could not open /proc/stat file");
    abort();
  }
  char buf[1024];
  unsigned long long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0, guest = 0,
                     guest_nice = 0;
  // read the first line: cpu stats
  char* ret = fgets(buf, sizeof(buf) - 1, fp);
  if (ret == nullptr) {
    LOG4(FATAL, "Could not get /proc/stat file");
    fclose(fp);
    abort();
  }
  fclose(fp);
  sscanf(buf, "cpu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu", &user, &nice, &system, &idle,
         &iowait, &irq, &softirq, &steal, &guest, &guest_nice);
  // do not include guest and guest_nice since they are already included in user and nice
  return user + nice + system + idle + iowait + irq + softirq + steal;
}

static void GetPidCpuTime(ThreadProc& proc, const char* pid) {
  // read pid_stat file
  char path[32]; // must hold "/proc/" + the highest possible PID (2^22 = 4194304) + file name
  snprintf(path, sizeof(path), "/proc/%s/stat", pid);
  FILE* fp = fopen(path, "r");
  if (fp == nullptr) {
    LOG4(FATAL, "Could not open /proc/<pid>/stat file");
    abort();
  }
  char buf[4096];
  if (!fgets(buf, sizeof(buf), fp)) {
    LOG4(FATAL, "Could not read /proc/<pid>/stat file");
    fclose(fp);
    abort();
  }
  sscanf(buf,
         "%*d "                // PID
         "%*s "                // comm
         "%*c "                // state
         "%*d %*d %*d %*d %*d" // ppid, pgrp, session, tty_nr, tpgid
         "%*u "                // flags
         "%*u %*u %*u %*u"     // minflt, cminflt, majflt, cmajflt
         "%lu %lu "            // utime, stime
         "%*d %*d "            // cutime, cstime
         "%*d %*d %*d %*d "    // priority, nice, num_threads, itrealvalue
         "%*u "                // starttime
         "%*u "                // vsize
         "%ld",                // rss
         &proc.utime, &proc.stime, &proc.vm_rss);
  // convert from pages to kB
  proc.vm_rss *= getpagesize() / 1024;
  fclose(fp);
}

// get the total memory usage from /proc/meminfo
static unsigned long long GetTotalCpuMem() {
  unsigned long long total_mem = 0;
  FILE* fp = fopen("/proc/meminfo", "r");
  if (fp == nullptr) {
    LOG4(FATAL, "Could not open /proc/meminfo file");
    abort();
  }
  char line[1024] = {0};
  char vmrss_name[32]; // name of the VmRSS field
  if (fgets(line, sizeof(line), fp) != NULL) {
    sscanf(line, "%s %llu", vmrss_name, &total_mem);
    fclose(fp);
  } else {
    LOG4(FATAL, "Could not read /proc/meminfo file");
    fclose(fp);
    abort();
  }
  return total_mem;
}

void SystemMonitor::Init() {
  const auto& server_config = GlobalConfig::Instance().server_config();
  interval_step_ = 1;
  stat_step_ = 1;
  if (server_config._is_set.gpu) {
#ifdef GRPS_CUDA_ENABLE
    // get config_devices
    devices_ = server_config.gpu.devices;
    // get cuda_visible_devices
    const char* cuda_visible_devices = getenv("CUDA_VISIBLE_DEVICES");
    if (cuda_visible_devices != nullptr) {
      // regex check
      if (!std::regex_match(cuda_visible_devices, std::regex(R"(^\d+(,\d+)*$)"))) {
        throw SystemMonitorException("CUDA_VISIBLE_DEVICES is invalid, value: " + std::string(cuda_visible_devices));
      }
      std::stringstream ss(cuda_visible_devices);
      std::string item;
      while (std::getline(ss, item, ',')) {
        cuda_visible_devices_.push_back(std::stoi(item));
      }
    }

    gpu_monitor_enable_ = true;
    mem_manager_type_ = server_config.gpu.mem_manager_type;
    if (mem_manager_type_ == "torch") {
#ifdef GRPS_TORCH_ENABLE
      mem_manager_enable_ = true;
      gpu_mem_mgr_ = std::make_shared<TorchGpuMemMgr>(server_config.gpu.devices);
#else
      throw SystemMonitorException("Torch gpu memory manager is not supported.");
#endif
    } else if (mem_manager_type_ == "tensorflow") {
#ifdef GRPS_TF_ENABLE
      mem_manager_enable_ = true;
      gpu_mem_mgr_ = std::make_shared<TfGpuMemMgr>(server_config.gpu.devices);
#else
      throw SystemMonitorException("Tensorflow gpu memory manager is not supported.");
#endif
    } else if (mem_manager_type_ == "none") {
      mem_manager_enable_ = false;
    } else {
      throw SystemMonitorException("Gpu memory manager type is not supported, type: " + mem_manager_type_);
    }

    if (mem_manager_enable_) {
      // Set gpu memory garbage collection interval.
      if (server_config.gpu.mem_gc_enable) {
        if (server_config.gpu.mem_gc_interval_s < 1) {
          LOG4(FATAL, "Gpu memory gc interval should not less than 1.");
          abort();
        } else {
          gc_step_ = server_config.gpu.mem_gc_interval_s / interval_step_;
        }
      }
      // Set gpu memory limit.
      mem_lim_mib_ = server_config.gpu.mem_lim_mib;
      if (mem_lim_mib_ <= 0 && mem_lim_mib_ != -1) {
        throw SystemMonitorException("Gpu memory limit must be greater than 0 or equal to -1 (not limit).");
      }
      gpu_mem_mgr_->SetMemLimit(float(mem_lim_mib_));
      LOG4(INFO, "Gpu memory monitor init success, interval_step: "
                   << interval_step_ << ", stat_step: " << stat_step_
                   << ", gc_enable: " << server_config.gpu.mem_gc_enable << ", gc_step: " << gc_step_
                   << ", mem_lim_mib: " << mem_lim_mib_ << ", mem_manager_type: " << mem_manager_type_);
    } else {
      LOG4(INFO, "Gpu memory monitor init success, interval_step: " << interval_step_ << ", stat_step: " << stat_step_
                                                                    << ", mem_manager_type: " << mem_manager_type_);
    }

    // nvml init
    nvmlReturn_t result;
    result = nvmlInit();
    if (NVML_SUCCESS != result) {
      LOG4(ERROR, "Failed to init NVML: " << nvmlErrorString(result));
    }
    for (auto i = 0; i < devices_.size(); i++) {
      auto gpu_idx = devices_[i];
      auto gpu_mem_usage = GetGpuMemUsage(gpu_idx);
      MONITOR_AVG("*gpu" + std::to_string(gpu_idx) + "_mem_usage(MIB)", float(gpu_mem_usage));
      auto gpu_usage = GetGpuUsage(gpu_idx);
      MONITOR_AVG("*gpu" + std::to_string(gpu_idx) + "_usage(%)", float(gpu_usage));
    }
    nvmlShutdown();
#endif
  }
}

#ifdef GRPS_CUDA_ENABLE
unsigned int SystemMonitor::GetGpuUsage(int gpu_id) {
  if (cuda_visible_devices_.size() > 0) {
    gpu_id = cuda_visible_devices_[gpu_id];
  }
  nvmlReturn_t result;
  nvmlDevice_t device;

  unsigned int device_count;
  result = nvmlDeviceGetCount_v2(&device_count);
  if (NVML_SUCCESS != result) {
    LOG4(ERROR, "Failed to get device count, error: " << nvmlErrorString(result));
    return 0;
  }
  if (gpu_id >= device_count) {
    // LOG4(ERROR, "Invalid gpu id: " << gpu_id << ", device count: " << device_count);
    return 0;
  }
  result = nvmlDeviceGetHandleByIndex_v2(gpu_id, &device);
  if (NVML_SUCCESS != result) {
    LOG4(ERROR, "Failed to get device handle for device " << gpu_id << ", error: " << nvmlErrorString(result));
    return 0;
  }
  nvmlUtilization_t utilization;
  result = nvmlDeviceGetUtilizationRates(device, &utilization);
  if (NVML_SUCCESS != result) {
    LOG4(ERROR, "Failed to get memory info for device " << gpu_id << ", error: " << nvmlErrorString(result));
    return 0;
  }
  return utilization.gpu;
}

unsigned long long SystemMonitor::GetGpuMemUsage(int gpu_id) {
  if (cuda_visible_devices_.size() > 0) {
    gpu_id = cuda_visible_devices_[gpu_id];
  }
  nvmlReturn_t result;
  nvmlDevice_t device;

  unsigned int device_count;
  result = nvmlDeviceGetCount_v2(&device_count);
  if (NVML_SUCCESS != result) {
    LOG4(ERROR, "Failed to get device count, error: " << nvmlErrorString(result));
    return 0;
  }
  if (gpu_id >= device_count) {
    // LOG4(ERROR, "Invalid gpu idx: " << gpu_id << ", device count: " << device_count);
    return 0;
  }
  result = nvmlDeviceGetHandleByIndex_v2(gpu_id, &device);
  if (NVML_SUCCESS != result) {
    LOG4(ERROR, "Failed to get device handle for device " << gpu_id << ", error: " << nvmlErrorString(result));
    return 0;
  }
  nvmlMemory_t memory;
  result = nvmlDeviceGetMemoryInfo(device, &memory);
  if (NVML_SUCCESS != result) {
    LOG4(ERROR, "Failed to get memory info for device " << gpu_id << ", error: " << nvmlErrorString(result));
    return 0;
  }
  return memory.used / MIB;
}
#endif

void SystemMonitor::MonitorFunc() {
  int step = 1;
  int pid = getpid();
  unsigned long long prev_total_cpu_time = 0, curr_total_cpu_time = 0;
  ThreadProc prev_proc = ThreadProc(), curr_proc = ThreadProc();
#ifdef GRPS_CUDA_ENABLE
  // nvml init
  nvmlReturn_t result;
  result = nvmlInit();
  if (NVML_SUCCESS != result) {
    LOG4(ERROR, "Failed to init NVML: " << nvmlErrorString(result));
  }
#endif
  while (!stop_) {
    std::chrono::system_clock::time_point begin_time = std::chrono::system_clock::now();
    try {
      if (step % stat_step_ == 0) {
        // get cpu usage
        curr_total_cpu_time = GetTotalCpuTime();
        GetPidCpuTime(curr_proc, std::to_string(pid).c_str());
        if (prev_total_cpu_time != 0) {
          // calculate the cpu usage percentage of current process in interval * step time
          unsigned long long cpu_time = (curr_proc.utime + curr_proc.stime) - (prev_proc.utime + prev_proc.stime);
          long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
          float cpu_usage =
            100.0 * (float(cpu_time) / float(curr_total_cpu_time - prev_total_cpu_time)) * float(num_cpus);
          MONITOR_AVG(CPU_USAGE_AVG, cpu_usage);
        }
        prev_total_cpu_time = curr_total_cpu_time;

        // get memory usage %
        float mem_usage = float(curr_proc.vm_rss) / float(GetTotalCpuMem()) * 100.0;
        MONITOR_AVG(MEM_USAGE_AVG, mem_usage);
        prev_proc = curr_proc;

        if (gpu_monitor_enable_) {
          // gpu stat & garbage collection
#ifdef GRPS_CUDA_ENABLE
          // gpu stat
          for (auto i = 0; i < devices_.size(); i++) {
            auto gpu_idx = devices_[i];
            auto gpu_mem_usage = GetGpuMemUsage(gpu_idx);
            MONITOR_AVG("*gpu" + std::to_string(gpu_idx) + "_mem_usage(MIB)", float(gpu_mem_usage));
            auto gpu_usage = GetGpuUsage(gpu_idx);
            MONITOR_AVG("*gpu" + std::to_string(gpu_idx) + "_usage(%)", float(gpu_usage));
          }
          if (mem_manager_enable_) {
            // gpu memory garbage collection
            if (gc_step_ > 0 && step % gc_step_ == 0) {
              gpu_mem_mgr_->MemGc();
            }
          }
#endif
        }
      }
    } catch (const SystemMonitorException& e) {
      LOG4(ERROR, "SystemMonitor::MonitorFunc() exception: " << e.what());
    } catch (const std::exception& e) {
      LOG4(ERROR, "SystemMonitor::MonitorFunc() exception: " << e.what());
    } catch (...) {
      LOG4(ERROR, "SystemMonitor::MonitorFunc() exception: unknown");
    }
    //    std::this_thread::sleep_for(std::chrono::seconds(interval_step_));
    std::this_thread::sleep_until(begin_time + std::chrono::seconds(interval_step_));
    step++;
  }
#ifdef GRPS_CUDA_ENABLE
  nvmlShutdown();
#endif
}

void SystemMonitor::Start() {
  std::thread monitor_thread(&SystemMonitor::MonitorFunc, this);
  monitor_thread.detach();
}

void SystemMonitor::Stop() {
  stop_.store(true);
}
} // namespace netease::grps
