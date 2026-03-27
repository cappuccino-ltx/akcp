

add_executable(server_balance server.cc)
target_include_directories(server_balance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lockfreequeue/include)
target_link_libraries(server_balance PRIVATE akcp)

add_executable(client_balance client.cc)
target_include_directories(client_balance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lockfreequeue/include)
target_link_libraries(client_balance PRIVATE akcp)

