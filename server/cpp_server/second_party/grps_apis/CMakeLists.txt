cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(grps_apis LANGUAGES C CXX)

if (NOT CMAKE_BUILD_TYPE)
    # Release With Debug Info
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif ()
message(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

# --std=c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

string(APPEND CMAKE_CXX_FLAGS " -fPIC")

file(GLOB PROTO_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/cpp_gens/*.h)
file(GLOB PROTO_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/cpp_gens/*.cc)
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp_gens/
)

add_library(grps_apis STATIC ${PROTO_SRCS})

# install headers
install(FILES ${PROTO_HEADERS} DESTINATION include/grps_apis)

# install lib
install(TARGETS grps_apis
        LIBRARY DESTINATION lib)