#pragma once 
#include <cstdint>
#include <functional>
#include <unordered_map>
#include "kcp_timer.hh"
namespace kcp{

class timer_tasks{
public:
    timer_tasks(asio::io_context& io_ctx,std::atomic_bool& stop);
    uint32_t push(std::function<void()>&& task,uint32_t milliseconds);
    void cancel(uint32_t index);
    static void update_callback(void* self, uint32_t index ,uint64_t clock);
private:
    timer timer_;
    uint32_t index_ {0};
    std::unordered_map<uint32_t, std::function<void()>> tasks;
}; // class timer_tasks

} // namespace kcp