

add_executable(server_stress server.cc)
target_link_libraries(server_stress PRIVATE akcp)

add_executable(client_stress client.cc)
target_link_libraries(client_stress PRIVATE akcp jsoncpp)