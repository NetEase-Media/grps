#!/bin/bash
# Used to support bash completion for grpst command.

# Define a completion spec for grpst
_grpst_complete() {
    local curr_arg=${COMP_WORDS[COMP_CWORD]}
    local sub_commands="create archive start tf_serve torch_serve trt_serve stop ps logs --help -h --version -v"

    # Add subcommands completion
    if [ $COMP_CWORD -eq 1 ]; then
        COMPREPLY=( $(compgen -W "$sub_commands" -- $curr_arg) )
    fi

    # Add argument completion for the subcommands
    if [ $COMP_CWORD -ge 2 ]; then
        case ${COMP_WORDS[1]} in
            create)
                COMPREPLY=( $(compgen -d -f -W "--help -h --project_type" -- $curr_arg) )
                ;;
            archive)
                COMPREPLY=( $(compgen -d -f  -W "--help -h --skip_unittest --output_path" -- $curr_arg) )
                ;;
            start)
                COMPREPLY=( $(compgen -d -f  -W "--help -h --name --conf_path --timeout" -- $curr_arg) )
                ;;
            tf_serve)
                COMPREPLY=( $(compgen -d -f  -W "--help -h --name --interface_framework --port --customized_predict_http_path --device --batching_type --max_batch_size --batch_timeout_us --max_connections --max_concurrency --gpu_devices_idx --gpu_mem_limit_mib --customized_op_paths --log_dir --log_backup_count --timeout --output_path" -- $curr_arg) )
                ;;
            torch_serve)
                COMPREPLY=( $(compgen -d -f  -W "--help -h --name --interface_framework --port --customized_predict_http_path --device --inp_device --batching_type --max_batch_size --batch_timeout_us --max_connections --max_concurrency --gpu_devices_idx --gpu_mem_limit_mib --gpu_mem_gc_enable --gpu_mem_gc_interval --customized_op_paths --log_dir --log_backup_count --timeout --output_path" -- $curr_arg) )
                ;;
            trt_serve)
                COMPREPLY=( $(compgen -d -f  -W "--help -h --name --interface_framework --port --customized_predict_http_path --device --streams --batching_type --max_batch_size --batch_timeout_us --max_connections --max_concurrency --gpu_devices_idx --customized_op_paths --log_dir --log_backup_count --timeout --output_path" -- $curr_arg) )
                ;;
            stop)
                COMPREPLY=( $(compgen -d -f  -W "--help -h" -- $curr_arg) )
                ;;
            ps)
                COMPREPLY=( $(compgen -d -f  -W "--help -h --name" -- $curr_arg) )
                ;;
            logs)
                COMPREPLY=( $(compgen -d -f  -W "--help -h" -- $curr_arg) )
                ;;
            *)
                ;;
        esac
    fi
}

# Register the completion spec
complete -F _grpst_complete grpst