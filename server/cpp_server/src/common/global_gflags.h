/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/06
 * Brief  global gflags.
 */

#pragma once

#include <gflags/gflags.h>

// Server configs.
DECLARE_int32(idle_timeout_sec);
DECLARE_int32(num_threads);
DECLARE_string(pid_file);
