

add_executable(server_multi server.cc)
target_link_libraries(server_multi PRIVATE akcp)

add_executable(client_multi client.cc)
target_link_libraries(client_multi PRIVATE akcp)

add_custom_command(
    TARGET client_multi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/test.sh"
        "${CMAKE_CURRENT_BINARY_DIR}/test.sh"
)