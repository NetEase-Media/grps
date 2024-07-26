# project cmake.

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

if (NOT DEFINED CUDA_ENABLE)
    set(CUDA_ENABLE 0)
endif ()
if (NOT DEFINED TF_ENABLE)
    set(TF_ENABLE 1)
endif ()
if (NOT DEFINED TORCH_ENABLE)
    set(TORCH_ENABLE 1)
endif ()
if (NOT DEFINED TRT_ENABLE)
    set(TRT_ENABLE 1)
endif ()

if (CUDA_ENABLE)
    project(grps_template LANGUAGES C CXX CUDA)
else ()
    project(grps_template LANGUAGES C CXX)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    # Release With Debug Info
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif ()

message(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

# --std=c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

set(CXXFLAGS
        -pipe
        -W
        -Wall
        -Wextra
        -m64
        -Wno-invalid-offsetof
        -Wno-deprecated
        -Wno-deprecated-declarations
        -Wno-unused-parameter

        # -Werror=unused-parameter
        -Wno-sign-compare
        -Wno-write-strings
        -Wno-unused-local-typedefs
        -Wno-literal-suffix
        -Wno-narrowing
        -Wno-parentheses
        -Wno-unused-but-set-variable
        -Wno-unused-variable
        -Wno-char-subscripts
        -Wno-implicit-fallthrough
        -Wno-register
        -ffast-math
        -fPIC
)

if (CUDA_ENABLE)
    list(APPEND CXXFLAGS -DGRPS_CUDA_ENABLE)
    find_package(CUDA REQUIRED)
    message(STATUS "CUDA_TOOLKIT_ROOT_DIR: ${CUDA_TOOLKIT_ROOT_DIR}")
    message(STATUS "CUDA_VERSION: ${CUDA_VERSION}")
    set(CUDA_INCLUDE_DIRS "${CUDA_TOOLKIT_ROOT_DIR}-${CUDA_VERSION}/include")
    message(STATUS "CUDA_INCLUDE_DIRS: ${CUDA_INCLUDE_DIRS}")
    message(STATUS "CUDA_LIBRARIES: ${CUDA_LIBRARIES}")
    message(STATUS "CUDA_CUBLAS_LIBRARIES: ${CUDA_CUBLAS_LIBRARIES}")
    set(CUDA_CUBLASLT_LIBRARIES "${CUDA_TOOLKIT_ROOT_DIR}/lib64/libcublasLt.so")
    message(STATUS "CUDA_CUBLASLT_LIBRARIES: ${CUDA_CUBLASLT_LIBRARIES}")
    set(CUDA_NVML_LIBRARIES "${CUDA_TOOLKIT_ROOT_DIR}/lib64/stubs/libnvidia-ml.so")
    include_directories(${CUDA_INCLUDE_DIRS})
    set(CUDA_NVCC_FLAGS "-std=c++17" "-Xcompiler=-fno-gnu-unique")
    set(CMAKE_CUDA_STANDARD 17)
    # CUDA targets
    if (DEFINED GPU_ARCHS)
        message(STATUS "GPU_ARCHS defined as ${GPU_ARCHS}. Generating CUDA code for SM ${GPU_ARCHS}")
        separate_arguments(GPU_ARCHS)
    else ()
        list(APPEND GPU_ARCHS 35 37 52 60 61 70 75)
        string(REGEX MATCH "aarch64" IS_ARM "${TRT_PLATFORM_ID}")
        if (IS_ARM)
            # Xavier (SM72) only supported for aarch64.
            list(APPEND GPU_ARCHS 72)
        endif ()

        if (CUDA_VERSION VERSION_GREATER_EQUAL 11.0)
            # Ampere GPU (SM80) support is only available in CUDA versions > 11.0
            list(APPEND GPU_ARCHS 80 86 89)
        else ()
            message(STATUS "Detected CUDA version is < 11.0. >= SM80 not supported.")
        endif ()

        message(STATUS "GPU_ARCHS is not defined. Generating CUDA code for default SMs: ${GPU_ARCHS}")
    endif ()
    # Generate SASS for each architecture
    foreach (arch ${GPU_ARCHS})
        set(GENCODES "${GENCODES}" "-gencode;arch=compute_${arch},code=sm_${arch}")
    endforeach ()
    set(CMAKE_CUDA_ARCHITECTURES ${GPU_ARCHS})
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler -Wno-deprecated-declarations")
    set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS}" "${GENCODES}")
    message(STATUS "CMAKE_CUDA_ARCHITECTURES: ${CMAKE_CUDA_ARCHITECTURES}")
    message(STATUS "CMAKE_CUDA_FLAGS: ${CMAKE_CUDA_FLAGS}")
    message(STATUS "CUDA_NVCC_FLAGS: ${CUDA_NVCC_FLAGS}")
endif ()

if (TF_ENABLE)
    list(APPEND CXXFLAGS -DGRPS_TF_ENABLE)
    if (NOT LIBTENSORFLOW_PATH)
        message("Not defined LIBTENSORFLOW_PATH, use default /usr/local/libtensorflow")
        set(LIBTENSORFLOW_PATH /usr/local/libtensorflow)
    endif ()
    set(TF_INCLUDE_DIR "${LIBTENSORFLOW_PATH}/include")
    set(TF_LIB_DIR "${LIBTENSORFLOW_PATH}/lib")
    include_directories(
            ${TF_INCLUDE_DIR}
    )
    set(TF_LIBRARIES ${TF_LIB_DIR}/libtensorflow_cc.so ${TF_LIB_DIR}/libtensorflow_framework.so)
    message(STATUS "TF_INCLUDE_DIR: ${TF_INCLUDE_DIR}")
    message(STATUS "TF_LIBRARIES: ${TF_LIBRARIES}")
endif ()

if (TORCH_ENABLE)
    list(APPEND CXXFLAGS -DGRPS_TORCH_ENABLE)
    if (NOT LIBTORCH_PATH)
        message("Not defined LIBTORCH_PATH, use default /usr/local/libtorch")
        set(LIBTORCH_PATH /usr/local/libtorch)
    endif ()
    set(Torch_DIR "${LIBTORCH_PATH}/share/cmake/Torch")
    find_package(Torch REQUIRED PATHS ${Torch_DIR})
    include_directories(
            ${TORCH_INCLUDE_DIRS}
    )
    message(STATUS "TORCH_INCLUDE_DIRS: ${TORCH_INCLUDE_DIRS}")
    message(STATUS "TORCH_LIBRARIES: ${TORCH_LIBRARIES}")
endif ()

if (TRT_ENABLE)
    list(APPEND CXXFLAGS -DGRPS_TRT_ENABLE)
    if (NOT LIBTENSORRT_PATH)
        message("Not defined LIBTENSORRT_PATH, use default /usr/local/libtensorrt")
        set(LIBTENSORRT_PATH /usr/local/libtensorrt)
    endif ()
    set(TRT_INCLUDE_DIR "${LIBTENSORRT_PATH}/include")
    set(TRT_LIB_DIR "${LIBTENSORRT_PATH}/lib")
    include_directories(
            ${TRT_INCLUDE_DIR}
    )
    set(TRT_LIBRARIES ${TRT_LIB_DIR}/libnvinfer.so ${TRT_LIB_DIR}/libnvinfer_plugin.so ${TRT_LIB_DIR}/libnvonnxparser.so)
    message(STATUS "TRT_INCLUDE_DIR: ${TRT_INCLUDE_DIR}")
    message(STATUS "TRT_LIBRARIES: ${TRT_LIBRARIES}")
endif ()

find_package(MPI REQUIRED)
message(STATUS "Using MPI_INCLUDE_PATH: ${MPI_INCLUDE_PATH}")
message(STATUS "Using MPI_LIBRARIES: ${MPI_LIBRARIES}")

string(REPLACE ";" " " CXXFLAGS "${CXXFLAGS}")
string(APPEND CMAKE_CXX_FLAGS " ")
string(APPEND CMAKE_CXX_FLAGS ${CXXFLAGS})

#message(STATUS "CMAKE_C_FLAGS_DEBUG: " ${CMAKE_C_FLAGS_DEBUG})
#message(STATUS "CMAKE_C_FLAGS_RELEASE: " ${CMAKE_C_FLAGS_RELEASE})
#message(STATUS "CMAKE_C_FLAGS_RELWITHDEBINFO: " ${CMAKE_C_FLAGS_RELWITHDEBINFO})
#message(STATUS "CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})
#message(STATUS "CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})

include_directories(
        /usr/local/include
        /usr/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/second_party/grps-server-framework/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include
        ${MPI_INCLUDE_PATH}
)

set(DEPEND_LINK_DIRECTORIES
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/second_party/grps-server-framework/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lib
)

add_subdirectory(src)

install(DIRECTORY conf/ DESTINATION conf)
install(DIRECTORY data/ DESTINATION data)