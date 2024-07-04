# @copyright 2023 netease. All rights reserved.
# @author zhaochaochao at corp netease dot com
# @date   2023-02
# @brief  build test
#
aux_source_directory(${CMAKE_SOURCE_DIR}/second_party/grps_apis/cpp_gens GRPS_APIS_SRCS)
add_executable(grps_brpc_client src/grps_brpc_client.cc ${GRPS_APIS_SRCS})

# -flto -fno-fat-lto-objects
# set_property(TARGET grps_brpc_client PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_directories(grps_brpc_client BEFORE PUBLIC ${DEPEND_LINK_DIRECTORIES})
target_link_libraries(grps_brpc_client
        brpc
        grpc++
        grpc
        gpr
        grpc_unsecure
        grpc++_unsecure
        protobuf
        pthread
)

target_link_options(grps_brpc_client BEFORE PUBLIC
)

install(TARGETS grps_brpc_client
        RUNTIME DESTINATION test/bin
        ARCHIVE DESTINATION test/lib
        LIBRARY DESTINATION test/lib
)

install(DIRECTORY conf/ DESTINATION test/conf)

aux_source_directory(${CMAKE_SOURCE_DIR}/second_party/grps_apis/cpp_gens GRPS_APIS_SRCS)
add_executable(grps_grpc_client src/grps_grpc_client.cc ${GRPS_APIS_SRCS})

# -flto -fno-fat-lto-objects
# set_property(TARGET grps_grpc_client PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_directories(grps_grpc_client BEFORE PUBLIC ${DEPEND_LINK_DIRECTORIES})
target_link_libraries(grps_grpc_client
        gpr
        grpc++_unsecure
        protobuf
        glog
        gflags
        pthread
)

target_link_options(grps_grpc_client BEFORE PUBLIC
)

install(TARGETS grps_grpc_client
        RUNTIME DESTINATION test/bin
        ARCHIVE DESTINATION test/lib
        LIBRARY DESTINATION test/lib
)

install(DIRECTORY conf/ DESTINATION test/conf)


add_executable(monitor_test src/monitor_test.cc ../src/monitor/monitor.cc ../src/logger/logger.cc)
target_link_directories(monitor_test BEFORE PUBLIC ${DEPEND_LINK_DIRECTORIES})
target_link_libraries(monitor_test
        gtest
        log4cxx.a
        aprutil-1
        apr-1
        expat
        pthread
        dl
        m
        unwind
        boost_system
        boost_thread
)

target_link_options(monitor_test BEFORE PUBLIC
)

install(TARGETS monitor_test
        RUNTIME DESTINATION test/bin
        ARCHIVE DESTINATION test/lib
        LIBRARY DESTINATION test/lib
)
