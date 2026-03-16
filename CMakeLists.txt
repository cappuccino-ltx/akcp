

cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(akcp)

set(CMAKE_CXX_STANDARD 17)

option(AKCP_BUILD_TESTS "Build akcp tests" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Boost REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS system)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/common/)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/resource/)
if(AKCP_BUILD_TESTS)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test/)
endif()

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/akcpConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/akcpConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/akcpConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/akcp
)

install(EXPORT akcpTargets
    NAMESPACE akcp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/akcp
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/akcpConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/akcpConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/akcp
)
