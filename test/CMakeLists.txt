

add_executable(kcp_server server.cc)
target_link_libraries(kcp_server PRIVATE akcp)

add_executable(kcp_client client.cc)
target_link_libraries(kcp_client PRIVATE akcp)

add_executable(test_timer test_timer.cc)
target_link_libraries(test_timer PRIVATE akcp)