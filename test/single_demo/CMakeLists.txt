

add_executable(server_single server.cc)
target_link_libraries(server_single PRIVATE akcp)

add_executable(client_single client.cc)
target_link_libraries(client_single PRIVATE akcp)