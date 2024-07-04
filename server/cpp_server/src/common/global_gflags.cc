/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/06
 * Brief  global gflags.
 */

#include "common/global_gflags.h"

// Server configs.
DEFINE_int32(idle_timeout_sec, 60, "Connection will be closed if there is no read/write operations.");
DEFINE_int32(num_threads, 32, "pthreads");
DEFINE_string(pid_file, "./data/grps_server.pid", "pid record file");
