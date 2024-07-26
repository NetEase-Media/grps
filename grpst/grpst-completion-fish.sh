# Used to support fish completion for grpst command.

# Add subcommands completion and ignore files completion
complete -c grpst -n "__fish_use_subcommand" -a "create" -d "create project" -f
complete -c grpst -n "__fish_use_subcommand" -a "archive" -d "archive project" -f
complete -c grpst -n "__fish_use_subcommand" -a "start" -d "start server" -f
complete -c grpst -n "__fish_use_subcommand" -a "tf_serve" -d "quick deploy tensorflow model" -f
complete -c grpst -n "__fish_use_subcommand" -a "torch_serve" -d "quick deploy torch model" -f
complete -c grpst -n "__fish_use_subcommand" -a "trt_serve" -d "quick deploy tensorrt model" -f
complete -c grpst -n "__fish_use_subcommand" -a "stop" -d "quick server" -f
complete -c grpst -n "__fish_use_subcommand" -a "ps" -d "list grps server process" -f
complete -c grpst -n "__fish_use_subcommand" -a "logs" -d "show grps server logs" -f
complete -c grpst -n "__fish_use_subcommand" -a "--help" -d "show help" -f
complete -c grpst -n "__fish_use_subcommand" -a "-h" -d "show help" -f
complete -c grpst -n "__fish_use_subcommand" -a "--version" -d "show version" -f
complete -c grpst -n "__fish_use_subcommand" -a "-v" -d "show version" -f

# Add argument completion for create subcommands
complete -c grpst -n "__fish_seen_subcommand_from create" -a "--help" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from create" -a "-h" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from create" -a "--project_type" -d "project type"

# Add argument completion for archive subcommands
complete -c grpst -n "__fish_seen_subcommand_from archive" -a "--help" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from archive" -a "-h" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from archive" -a "--skip_unittest" -d "skip unittest"
complete -c grpst -n "__fish_seen_subcommand_from archive" -a "--output_path" -d "output path"

# Add argument completion for start subcommands
complete -c grpst -n "__fish_seen_subcommand_from start" -a "--help" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from start" -a "-h" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from start" -a "--name" -d "server name"
complete -c grpst -n "__fish_seen_subcommand_from start" -a "--conf_path" -d "server config path"
complete -c grpst -n "__fish_seen_subcommand_from start" -a "--timeout" -d "timeout"
complete -c grpst -n "__fish_seen_subcommand_from start" -a "--mpi_np" -d "mpi process count"

# Add argument completion for tf_serve subcommands
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--help" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "-h" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--name" -d "server name"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--interface_framework" -d "interface framework"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--port" -d "server port"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--customized_predict_http_path" -d "customized predict http path"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--device" -d "device"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--batching_type" -d "batching type"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--max_batch_size" -d "batching max batch size"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--batch_timeout_us" -d "batching timeout(us)"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--max_connections" -d "max connections limit"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--max_concurrency" -d "max concurrency limit"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--gpu_devices_idx" -d "gpu limit and monitor devices idx"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--gpu_mem_limit_mib" -d "gpu memory limit"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--customized_op_paths" -d "customized op paths"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--log_dir" -d "log dir"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--log_backup_count" -d "log backup count"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--output_path" -d "archive to mar output path"
complete -c grpst -n "__fish_seen_subcommand_from tf_serve" -a "--timeout" -d "timeout"

# Add argument completion for torch_serve subcommands
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--help" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "-h" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--name" -d "server name"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--interface_framework" -d "interface framework"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--port" -d "server port"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--customized_predict_http_path" -d "customized predict http path"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--device" -d "device"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--inp_device" -d "input device"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--batching_type" -d "batching type"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--max_batch_size" -d "batching max batch size"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--batch_timeout_us" -d "batching timeout(us)"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--max_connections" -d "max connections limit"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--max_concurrency" -d "max concurrency limit"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--gpu_devices_idx" -d "gpu limit and monitor devices idx"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--gpu_mem_limit_mib" -d "gpu memory limit"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--gpu_mem_gc_enable" -d "enable gpu memory gc"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--gpu_mem_gc_interval" -d "gpu memory gc interval"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--customized_op_paths" -d "customized op paths"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--log_dir" -d "log dir"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--log_backup_count" -d "log backup count"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--output_path" -d "archive to mar output path"
complete -c grpst -n "__fish_seen_subcommand_from torch_serve" -a "--timeout" -d "timeout"

# Add argument completion for trt_serve subcommands
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--help" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "-h" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--name" -d "server name"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--interface_framework" -d "interface framework"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--port" -d "server port"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--customized_predict_http_path" -d "customized predict http path"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--device" -d "device"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--streams" -d "multi streams"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--batching_type" -d "batching type"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--max_batch_size" -d "batching max batch size"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--batch_timeout_us" -d "batching timeout(us)"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--max_connections" -d "max connections limit"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--max_concurrency" -d "max concurrency limit"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--gpu_devices_idx" -d "gpu monitor devices idx"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--customized_op_paths" -d "customized op paths"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--log_dir" -d "log dir"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--log_backup_count" -d "log backup count"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--output_path" -d "archive to mar output path"
complete -c grpst -n "__fish_seen_subcommand_from trt_serve" -a "--timeout" -d "timeout"

# Add argument completion for stop subcommands
complete -c grpst -n "__fish_seen_subcommand_from stop" -a "--help" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from stop" -a "-h" -d "show help"

# Add argument completion for ps subcommands
complete -c grpst -n "__fish_seen_subcommand_from ps" -a "--help" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from ps" -a "-h" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from ps" -a "--name" -d "server name"

# Add argument completion for logs subcommands
complete -c grpst -n "__fish_seen_subcommand_from logs" -a "--help" -d "show help"
complete -c grpst -n "__fish_seen_subcommand_from logs" -a "-h" -d "show help"