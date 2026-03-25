#pragma once 

#include "channel_manager.hh"
#include "common.hh"
#include "kcp_thread.hh"

#include <atomic>
#include <functional>

namespace kcp{
    
class server{
public:
    server();

    void set_connect_callback(const std::function<void(channel_view,bool)>& callback);
    void set_message_callback(const std::function<void(channel_view,packet)>& callback);

    void enable_muliti_thread(int n = std::thread::hardware_concurrency());
    void set_connection_timeout(uint32_t second = 10);
    void set_buffer_pool(const std::function<packet(size_t)>& back);

    void start(int port, int core_begin = -1, int core_end = -1);
    void stop();
private:
    void stop_internal();

private:
    // muliti thread
    std::vector<std::unique_ptr<kcp_thread>> threads_;
    //  count = 0;
    // control variable
    std::atomic_bool stop_ {false};
    // callback function 
    std::function<void(channel_view,bool)> connect_callback_;
    std::function<void(channel_view,packet)> message_callback_;
}; // class server

}; // namespace kcp