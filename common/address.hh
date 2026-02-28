#pragma once 


#include "common.hh"
#include <cassert>


namespace kcp{
namespace util{
    class address{
    public:
        static void point_to_string(const udp::endpoint& point, std::string* host);
        static void host_to_string(const std::string& ip, int port, std::string* host);
        static void string_to_host(const std::string& host, std::string* ip, std::string* port);
    }; // class address
} // namespace util
} // namespace kcp