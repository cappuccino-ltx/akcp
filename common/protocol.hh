#pragma once 

#include <common.hh>

namespace kcp{
class protocol{
public:
    static std::string format_connect_response(uint32_t conv);
    static std::string format_connect_response_ack(uint32_t conv);
    static uint32_t parse_conv_from_response(const char* data);
}; // class protocol
} // namespace kcp