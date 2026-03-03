

add_executable(server_multi server.cc)
target_link_libraries(server_multi PRIVATE akcp)

add_executable(client_multi client.cc)
target_link_libraries(client_multi PRIVATE akcp)
