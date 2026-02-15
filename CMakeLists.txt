

cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(akcp)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/common/)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/resource/)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test/)
