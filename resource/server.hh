#pragma once 

#include "channel_manager.hh"
#include "common.hh"
#include "io_loop.hh"
#include "kcp_thread.hh"
#include <protocol.hh>
#include <atomic>
#include <functional>

namespace kcp{
    
class server{
public:
    server();

    void set_connect_callback(const std::function<void(channel_view,bool)>& callback);
    void set_message_callback(const std::function<void(channel_view,packet)>& callback);

    void enable_muliti_thread(int n = 3);
    void set_update_interval(int val);

    void start(int port);
    void stop();
private:
    void start_single(int port);
    void start_muliti(int port);

    static void receive_callback(void* self,const udp::endpoint& point,const char* data,size_t size);
    static void receive_callback_single(void* self,const udp::endpoint& point,const char* data,size_t size);

private:
    // single thread
    std::shared_ptr<channel_manager> manager_;
    // muliti thread
    std::vector<kcp_thread> threads_;

    size_t count = 0;
    std::shared_ptr<io_loop> loop_;
    // control variable
    std::atomic_bool stop_ {false};
    // callback function 
    std::function<void(channel_view,bool)> connect_callback_;
    std::function<void(channel_view,packet)> message_callback_;
}; // class server

}; // namespace kcp