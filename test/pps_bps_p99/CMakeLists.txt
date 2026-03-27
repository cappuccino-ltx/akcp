

add_executable(server_pbp server.cc)
target_include_directories(server_pbp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lockfreequeue/include)
target_link_libraries(server_pbp PRIVATE akcp)

add_executable(client_pbp client.cc)
target_include_directories(client_pbp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lockfreequeue/include)
target_link_libraries(client_pbp PRIVATE akcp)

