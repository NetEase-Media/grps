# build grps server user customized library and unit test binary.

######################## Build user customized lib [BEGIN] ########################
add_library(grps-server-customized SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/customized_converter.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/customized_inferer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/grps_server_customized.cc
)

set(THIRD_PARTY_LIB_DIR ${CMAKE_SOURCE_DIR}/third_party/lib/)
target_link_directories(grps-server-customized BEFORE PRIVATE ${DEPEND_LINK_DIRECTORIES} ${THIRD_PARTY_LIB_DIR})
target_link_libraries(grps-server-customized PRIVATE
        libgrps-server-framework.a
        ${MPI_LIBRARIES}
        brpc
        grpc++_unsecure
        gtest
        protobuf
        yaml-cpp
        log4cxx
        aprutil-1
        apr-1
        pthread
        boost_system
        boost_thread
        dl

        # add more third party lib.
)

if (CUDA_ENABLE)
    target_link_libraries(grps-server-customized PRIVATE ${CUDA_LIBRARIES} ${CUDA_NVML_LIBRARIES})
    set_target_properties(grps-server-customized PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif ()
if (TF_ENABLE)
    target_link_directories(grps-server-customized BEFORE PUBLIC ${TF_LIB_DIR})
    target_link_libraries(grps-server-customized PRIVATE ${TF_LIBRARIES})
endif ()
if (TORCH_ENABLE)
    target_link_libraries(grps-server-customized PRIVATE ${TORCH_LIBRARIES})
endif ()
if (TRT_ENABLE)
    target_link_libraries(grps-server-customized PRIVATE ${TRT_LIBRARIES})
endif ()

set_target_properties(
        grps-server-customized
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        OUTPUT_NAME grps-server-customized
        LINK_FLAGS
        "-Wl,--no-undefined"
)

# install grps-server-customized lib.
install(TARGETS grps-server-customized
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)

# install third party lib.
install(DIRECTORY ${THIRD_PARTY_LIB_DIR}
        DESTINATION lib
        FILES_MATCHING PATTERN "*.so*"
)
######################## Build user customized lib [END] ########################

######################## Build unittest [BEGIN] ########################
add_executable(unit_test
        main.cc
)

# -flto -fno-fat-lto-objects
# set_property(TARGET unit_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_link_directories(unit_test BEFORE PUBLIC ${DEPEND_LINK_DIRECTORIES})
target_link_libraries(unit_test PUBLIC
        "-Wl,--whole-archive"
        grps-server-customized
        libgrps-server-framework.a
        "-Wl,--no-whole-archive"
        ${MPI_LIBRARIES}
        brpc
        grpc++_unsecure
        gtest
        protobuf
        yaml-cpp
        log4cxx
        aprutil-1
        apr-1
        pthread
        boost_system
        boost_thread
        dl
)
if (CUDA_ENABLE)
    target_link_libraries(unit_test PUBLIC ${CUDA_LIBRARIES} ${CUDA_NVML_LIBRARIES})
endif ()
if (TF_ENABLE)
    target_link_directories(unit_test BEFORE PUBLIC ${TF_LIB_DIR})
    target_link_libraries(unit_test PUBLIC ${TF_LIBRARIES})
endif ()
if (TORCH_ENABLE)
    target_link_libraries(unit_test PUBLIC ${TORCH_LIBRARIES})
endif ()
if (TRT_ENABLE)
    target_link_libraries(unit_test PUBLIC ${TRT_LIBRARIES})
endif ()


target_link_options(unit_test BEFORE PUBLIC
)

install(TARGETS unit_test
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)
######################## Build unittest [END] ########################
