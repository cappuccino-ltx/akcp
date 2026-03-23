
#include "packet.hh"

namespace kcp{

std::function<packet(size_t)> buffer_pool_interface::get_packet_;

void buffer_pool_interface::set_packet_get_callback(const std::function<packet(size_t)>& back){
    get_packet_ = back;
    return ;
}
packet buffer_pool_interface::get_packet(size_t size){
    if (get_packet_) {
        return get_packet_(size);
    }
    return std::make_shared<std::vector<uint8_t>>(size);
}

} // namespace kcp