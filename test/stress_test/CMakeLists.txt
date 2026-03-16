

add_executable(server_stress server.cc)
target_link_libraries(server_stress PRIVATE akcp)

add_executable(client_stress client.cc)
find_package(jsoncpp CONFIG REQUIRED)
if(TARGET JsonCpp::JsonCpp)
    target_link_libraries(client_stress PRIVATE akcp JsonCpp::JsonCpp)
elseif(TARGET jsoncpp_lib)
    target_link_libraries(client_stress PRIVATE akcp jsoncpp_lib)
elseif(TARGET jsoncpp_static)
    target_link_libraries(client_stress PRIVATE akcp jsoncpp_static)
elseif(TARGET jsoncpp::jsoncpp)
    target_link_libraries(client_stress PRIVATE akcp jsoncpp::jsoncpp)
else()
    target_link_libraries(client_stress PRIVATE akcp jsoncpp)
    message(FATAL_INFO "jsoncpp target not found")
endif()
