/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/21
 * Brief  Project constants.
 */

#pragma once

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Version.
#define GRPS_MAJOR 1
#define GRPS_MINOR 1
#define GRPS_PATCH 0

#define GRPS_VERSION "v" STR(GRPS_MAJOR) "." STR(GRPS_MINOR) "." STR(GRPS_PATCH)

// Some units.
#define MIB (1024 * 1024)
#define GIB (1024 * 1024 * 1024)

// Metrics name.
#define QPS "*qps"
#define REQ_FAIL_RATE "*fail_rate(%)"
#define REQ_LATENCY_AVG "*latency_avg(ms)"
#define REQ_LATENCY_MAX "*latency_max(ms)"
#define REQ_LATENCY_CDF "*latency_cdf(ms)"
#define GPU_OOM_COUNT "*gpu_oom_count"
#define CPU_USAGE_AVG "*cpu_usage(%)"
#define MEM_USAGE_AVG "*mem_usage(%)"
