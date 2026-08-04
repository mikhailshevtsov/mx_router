cmake_minimum_required(VERSION 3.28)
project(mx_router)

include(FetchContent)
enable_testing()

add_library(mx_router INTERFACE)
target_include_directories(mx_router INTERFACE ${PROJECT_SOURCE_DIR}/include/)

add_subdirectory(tests/)