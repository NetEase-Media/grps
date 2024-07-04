# build client binary

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(grps_client LANGUAGES C CXX)

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

string(REPLACE ";" " " CXXFLAGS "${CXXFLAGS}")
string(APPEND CMAKE_CXX_FLAGS " ")
string(APPEND CMAKE_CXX_FLAGS ${CXXFLAGS})
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})

include_directories(
        /usr/local/include
        /usr/include
)

set(DEPEND_LINK_DIRECTORIES
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib
)

add_executable(brpc_client brpc_client.cc)

target_link_directories(brpc_client BEFORE PUBLIC ${DEPEND_LINK_DIRECTORIES})
target_link_libraries(brpc_client
        libgrps_apis.a
        brpc
        protobuf
)

install(TARGETS brpc_client
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)

add_executable(grpc_client grpc_client.cc)

target_link_directories(grpc_client BEFORE PUBLIC ${DEPEND_LINK_DIRECTORIES})
target_link_libraries(grpc_client
        libgrps_apis.a
        grpc++_unsecure
        protobuf
        glog
        gflags
)

install(TARGETS grpc_client
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)
