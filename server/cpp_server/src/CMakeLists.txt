# @copyright 2023 netease. All rights reserved.
# @author zhaochaochao at corp netease dot com
# @date   2023-02
# @brief  build grps_server binary
#
# prepare customized lib env.
set(CUSTOMIZED_LIB_DIR
        ${CMAKE_SOURCE_DIR}/third_party/grps-server-customized/lib/
        ${CMAKE_CURRENT_SOURCE_DIR}/customized/lib/
)

######################## Build framework lib [BEGIN] ########################
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/batching BATCHING_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/logger LOGGER_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/config CONFIG_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/context CONTEXT_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/dag DAG_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/executor EXECUTOR_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/monitor MONITOR_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/model MODEL_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/system_monitor SYSTEM_MONITOR_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/second_party/grps_apis/cpp_gens GRPS_APIS_SRCS)

set(CONVERTER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/converter/converter.cc)
if (TF_ENABLE)
    list(APPEND CONVERTER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/converter/tf_tensor_converter.cc)
endif ()
if (TORCH_ENABLE)
    list(APPEND CONVERTER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/converter/torch_tensor_converter.cc)
endif ()
if (TRT_ENABLE)
    list(APPEND CONVERTER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/converter/trt_tensor_converter.cc)
endif ()

set(MODEL_INFER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/inferer.cc)
if (TF_ENABLE)
    list(APPEND MODEL_INFER_SRCS ${MODEL_INFER_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/tf_inferer.cc)
endif ()
if (TORCH_ENABLE)
    list(APPEND MODEL_INFER_SRCS ${MODEL_INFER_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/torch_inferer.cc)
endif ()
if (TRT_ENABLE)
    list(APPEND MODEL_INFER_SRCS ${MODEL_INFER_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/trt_inferer.cc)
endif ()

if (TF_ENABLE)
    list(APPEND MEM_MANAGER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/mem_manager/tf_gpu_mem_mgr.cc)
endif ()
if (TORCH_ENABLE)
    list(APPEND MEM_MANAGER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/mem_manager/torch_gpu_mem_mgr.cc)
endif ()

add_library(grps-server-framework STATIC
        ${BATCHING_SRCS}
        ${LOGGER_SRCS}
        ${CONFIG_SRCS}
        ${CONTEXT_SRCS}
        ${CONVERTER_SRCS}
        ${DAG_SRCS}
        ${EXECUTOR_SRCS}
        ${MODEL_INFER_SRCS}
        ${MONITOR_SRCS}
        ${MEM_MANAGER_SRCS}
        ${MODEL_SRCS}
        ${SYSTEM_MONITOR_SRCS}
        ${GRPS_APIS_SRCS}
)

if (CUDA_ENABLE)
    set_target_properties(grps-server-framework PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif ()

######################## Build framework lib [END] ########################

######################## Build server [BEGIN] ########################
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/common COMMON_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/handler HANDLER_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/service SERVICE_SRCS)

add_executable(grps_server
        main.cc
        ${COMMON_SRCS}
        ${HANDLER_SRCS}
        ${SERVICE_SRCS}
)

# -flto -fno-fat-lto-objects
# set_property(TARGET grps_server PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

if (CUDA_ENABLE)
    set_target_properties(grps_server PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif ()

target_link_directories(grps_server BEFORE PUBLIC ${DEPEND_LINK_DIRECTORIES} ${CUSTOMIZED_LIB_DIR})
target_link_libraries(grps_server PUBLIC
        "-Wl,--whole-archive"
        libgrps-server-customized.so
        grps-server-framework
        "-Wl,--no-whole-archive"
        ${MPI_LIBRARIES}
        boost_system
        boost_thread
        brpc
        dw
        gpr
        grpc++_unsecure
        protobuf
        yaml-cpp
        jemalloc
        log4cxx
        aprutil-1
        apr-1
        pthread
        dl
)

if (CUDA_ENABLE)
    target_link_libraries(grps_server PUBLIC ${CUDA_LIBRARIES} ${CUDA_NVML_LIBRARIES})
endif ()
if (TF_ENABLE)
    target_link_libraries(grps_server PUBLIC ${TF_LIBRARIES})
endif ()
if (TORCH_ENABLE)
    target_link_libraries(grps_server PUBLIC ${TORCH_LIBRARIES})
endif ()
if (TRT_ENABLE)
    target_link_libraries(grps_server PUBLIC ${TRT_LIBRARIES})
endif ()

target_link_options(grps_server BEFORE PUBLIC
)

######################## Build server [END] ########################
install(TARGETS grps_server
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)

# install customized lib.
install(DIRECTORY ${CUSTOMIZED_LIB_DIR}
        DESTINATION lib
        FILES_MATCHING PATTERN "*.so*"
)

# define custom command to install grps-server-framework lib and headers
file(GLOB_RECURSE BATCHING_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/batching/*.h)
file(GLOB_RECURSE CONFIG_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/config/*.h)
file(GLOB_RECURSE CONTEXT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/context/*.h)
file(GLOB_RECURSE LOGGER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/logger/*.h)
file(GLOB_RECURSE DAG_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/dag/*.h)
file(GLOB_RECURSE EXECUTOR_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/executor/*.h)
file(GLOB_RECURSE MONITOR_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/monitor/*.h)
file(GLOB_RECURSE MEM_MANAGER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/mem_manager/*.h)
file(GLOB_RECURSE MODEL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/model/*.h)
file(GLOB_RECURSE SYSTEM_MONITOR_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/system_monitor/*.h)
file(GLOB_RECURSE GRPC_HEADERS ${CMAKE_SOURCE_DIR}/second_party/grps_apis/cpp_gens/grps.pb.h)
set(CONVERTER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/converter/converter.h)
if (TF_ENABLE)
    list(APPEND CONVERTER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/converter/tf_tensor_converter.h)
endif ()
if (TORCH_ENABLE)
    list(APPEND CONVERTER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/converter/torch_tensor_converter.h)
endif ()
if (TRT_ENABLE)
    list(APPEND CONVERTER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/converter/trt_tensor_converter.h)
endif ()

set(MODEL_INFER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/inferer.h ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/tensor_wrapper.h)
if (CUDA_ENABLE)
    list(APPEND MODEL_INFER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/cuda_helper.h)
endif ()
if (TF_ENABLE)
    list(APPEND MODEL_INFER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/tf_inferer.h)
endif ()
if (TORCH_ENABLE)
    list(APPEND MODEL_INFER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/torch_inferer.h)
endif ()
if (TRT_ENABLE)
    list(APPEND MODEL_INFER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/trt_inferer.h)
    list(APPEND MODEL_INFER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/model_infer/trt_cuda.h)
endif ()

add_custom_command(
        OUTPUT grps-server-framework-out
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/lib
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:grps-server-framework> ${CMAKE_INSTALL_PREFIX}/lib/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/batching
        COMMAND ${CMAKE_COMMAND} -E copy ${BATCHING_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/batching/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/config
        COMMAND ${CMAKE_COMMAND} -E copy ${CONFIG_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/config/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/context
        COMMAND ${CMAKE_COMMAND} -E copy ${CONTEXT_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/context/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/converter
        COMMAND ${CMAKE_COMMAND} -E copy ${CONVERTER_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/converter/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/logger
        COMMAND ${CMAKE_COMMAND} -E copy ${LOGGER_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/logger/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/dag
        COMMAND ${CMAKE_COMMAND} -E copy ${DAG_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/dag/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/executor
        COMMAND ${CMAKE_COMMAND} -E copy ${EXECUTOR_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/executor/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/model_infer
        COMMAND ${CMAKE_COMMAND} -E copy ${MODEL_INFER_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/model_infer/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/monitor
        COMMAND ${CMAKE_COMMAND} -E copy ${MONITOR_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/monitor/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/mem_manager
        COMMAND ${CMAKE_COMMAND} -E copy ${MEM_MANAGER_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/mem_manager/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/model
        COMMAND ${CMAKE_COMMAND} -E copy ${MODEL_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/model/
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/system_monitor
        COMMAND ${CMAKE_COMMAND} -E copy ${SYSTEM_MONITOR_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/system_monitor/
        COMMAND ${CMAKE_COMMAND} -E copy ${GRPC_HEADERS} ${CMAKE_INSTALL_PREFIX}/include/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/constant.h ${CMAKE_INSTALL_PREFIX}/include/
        DEPENDS grps-server-framework
)

add_custom_target(grps-server-framework-install ALL DEPENDS grps-server-framework-out)
