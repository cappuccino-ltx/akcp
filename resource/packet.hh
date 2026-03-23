#pragma once 

#include "common.hh"

namespace kcp{

class buffer_pool_interface{
public:
    static void set_packet_get_callback(const std::function<packet(size_t)>& back);
    static packet get_packet(size_t size);
    
private:
    static std::function<packet(size_t)> get_packet_;
}; // class buffer_pool

} // namespace kcp