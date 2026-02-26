

add_executable(server_stress server.cc)
target_link_libraries(server_stress PRIVATE akcp)

add_executable(connection_number_stress connection_number.cc)
target_link_libraries(connection_number_stress PRIVATE akcp jsoncpp)

add_executable(rate_diff_stress rate_diff.cc)
target_link_libraries(rate_diff_stress PRIVATE akcp)

add_executable(data_size_stress data_size.cc)
target_link_libraries(data_size_stress PRIVATE akcp)