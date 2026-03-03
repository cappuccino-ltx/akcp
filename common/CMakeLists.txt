

file(GLOB COMMON_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(kcp_common OBJECT ${COMMON_SRC})

target_link_libraries(kcp_common PUBLIC Boost::system Boost::asio)

target_include_directories(kcp_common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/"
)