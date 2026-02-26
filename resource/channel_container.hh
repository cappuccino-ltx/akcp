#pragma once 

#include <cstdint>
#include <unordered_map>
#include <memory>
#include "channel.hh"
#include "timer.hh"


namespace kcp{

class channel_container{
public:
    // is_sharding is false, discord
    // is_sharding is ture , n > 0,
    channel_container(asio::io_context& io_ctx,std::atomic_bool& stop);
    std::shared_ptr<channel> find(uint32_t conv);
    void clear();
    std::shared_ptr<channel> insert(uint32_t conv, const udp::endpoint& peer, const std::weak_ptr<channel_manager>& manager);
    void remove(uint32_t conv);
    size_t size();
    void stop();
    static void remove_callback(void* self, uint32_t conv);
private:
    static void update_callback(void* self, uint32_t conv ,uint64_t clock);
    
private:
    std::unordered_map<uint32_t,std::shared_ptr<channel>> channels_;
    timer timer_;
}; // class connection_container

} // namespace kcp