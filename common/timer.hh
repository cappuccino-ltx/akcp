#pragma once 

#include <cstdint>
#include <queue>
#include "common.hh"

namespace kcp{

class timer{
public:
    timer(asio::io_context& io_ctx,std::atomic_bool& stop);
    
    void push(uint64_t clock, uint32_t conv);
    void set_update_callback(void(* callback)(void*, uint32_t, uint64_t), void* ctx);
    void stop();
    void push_internal(uint64_t clock, uint32_t conv);

private:
    void handler(boost::system::error_code ec);
    void handler_push_internal();
    void handler_timeout(uint64_t now);
    void arm_to_top(uint64_t now);

private:
    struct Item{
        uint64_t next_time_;
        uint32_t conv;
    };
    struct Compare{
        bool operator()(const Item& i1,const Item& i2);
    };
    std::priority_queue<Item,std::vector<Item>,Compare> heap_;
    std::vector<Item> tasks_;
    void* update_ctx_;
    void(* update_callbacl_)(void*, uint32_t, uint64_t) {nullptr};
    asio::system_timer timer_;
    uint64_t next_timeout_ { MAX_TIMEOUT };
    std::atomic_bool& stop_;
};

} // namespace kcp