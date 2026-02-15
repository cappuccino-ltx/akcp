

file(GLOB RESOURCE_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

add_library(akcp STATIC ${RESOURCE_SRC})

target_include_directories(akcp PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/"
)

target_link_libraries(akcp PUBLIC kcp_common)